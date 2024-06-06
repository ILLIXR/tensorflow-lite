/* Copyright 2023 The OpenXLA Authors.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "xla/service/gpu/runtime/nccl_p2p_thunk_common.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "absl/synchronization/mutex.h"
#include "mlir/IR/BuiltinAttributes.h"  // from @llvm-project
#include "xla/executable_run_options.h"
#include "xla/hlo/ir/hlo_instructions.h"
#include "xla/service/collective_ops_utils.h"
#include "xla/service/gpu/runtime/nccl_clique_key.h"
#include "xla/service/hlo_parser.h"
#include "xla/shape.h"
#include "xla/status_macros.h"
#include "xla/stream_executor/stream_executor.h"
#include "xla/xla_data.pb.h"
#include "tsl/platform/statusor.h"

namespace xla {
namespace gpu {

absl::Status ExecutionCounters::Initialize(se::StreamExecutor* executor,
                                           RunId run_id) {
  absl::MutexLock lock(&mu_);
  CounterKey key = {executor, run_id};
  if (counters_.contains(key)) return absl::OkStatus();
  counters_.emplace(key, 0);
  return absl::OkStatus();
}

absl::StatusOr<int64_t*> ExecutionCounters::GetCounter(
    se::StreamExecutor* executor, RunId run_id) {
  absl::MutexLock lock(&mu_);
  CounterKey key = {executor, run_id};
  auto counter = counters_.find(key);
  if (counter == counters_.end()) {
    return absl::InternalError("Execution counter not initialized");
  }

  return &counter->second;
}

absl::StatusOr<std::vector<std::pair<int64_t, int64_t>>> GetSourceTargetPairs(
    mlir::DictionaryAttr frontend_attributes) {
  mlir::StringAttr src_dst_string = frontend_attributes.getAs<mlir::StringAttr>(
      kSendRecvSourceTargetPairsAttr);
  if (!src_dst_string) {
    return absl::AbortedError(
        absl::StrCat("expecting send/recv op with string attribute ",
                     kSendRecvSourceTargetPairsAttr));
  }
  TF_ASSIGN_OR_RETURN(std::vector<ReplicaGroup> replica_groups,
                      ParseReplicaGroupsOnly(src_dst_string.str()));
  std::vector<std::pair<int64_t, int64_t>> source_target_pairs;
  source_target_pairs.reserve(replica_groups.size());
  for (const ReplicaGroup& replica_group : replica_groups) {
    TF_RET_CHECK(replica_group.replica_ids_size() == 2);
    source_target_pairs.emplace_back(replica_group.replica_ids(0),
                                     replica_group.replica_ids(1));
  }
  return source_target_pairs;
}

NcclP2PConfig GetNcclP2PConfigForSendRecv(const HloSendRecvInstruction* instr,
                                          const Shape& shape,
                                          int64_t replica_count,
                                          int64_t partition_count) {
  NcclP2PConfig p2p_config;
  auto& config = p2p_config.config;

  config.operand_count = 1;
  config.operand_element_type.push_back(shape.element_type());
  config.SetCollectiveOpKindAndID(instr);
  config.group_mode = GetCollectiveOpGroupMode(
                          instr->channel_id().value_or(0) > 0, std::nullopt)
                          .value();

  // All execution instances of a Send/Recv together form a replica group.
  const int64_t num_participants =
      config.group_mode == CollectiveOpGroupMode::kCrossReplica
          ? replica_count
          : partition_count;
  config.replica_groups.emplace_back();
  ReplicaGroup& replica_group = config.replica_groups.front();
  for (int i = 0; i < num_participants; ++i) {
    replica_group.add_replica_ids(i);
  }

  std::optional<std::string> source_target_pairs_string =
      instr->frontend_attributes().map().at(kSendRecvSourceTargetPairsAttr);

  // We currently ignore problems related to the source-target-pair string to
  // avoid using absl::StatusOr for the return type. This should be ok as
  // Send/Recv are generated by the compiler.
  if (!source_target_pairs_string.has_value()) {
    return p2p_config;
  }
  auto statusor = ParseReplicaGroupsOnly(*source_target_pairs_string);
  if (!statusor.ok()) {
    return p2p_config;
  }

  std::vector<ReplicaGroup> replica_groups = statusor.value();
  auto validation_it =
      instr->frontend_attributes().map().find(kSendRecvValidationAttr);
  NcclP2PConfig::ValidationKind validation_kind =
      NcclP2PConfig::ValidationKind::kValid;
  std::vector<ReplicaGroup> bounds;
  if (validation_it != instr->frontend_attributes().map().end()) {
    if (validation_it->second == "invalid") {
      validation_kind = NcclP2PConfig::ValidationKind::kInvalid;
    } else {
      auto statusor_bounds = ParseReplicaGroupsOnly(validation_it->second);
      if (!statusor_bounds.ok() ||
          statusor_bounds.value().size() != replica_groups.size()) {
        // Ignore problems related to the source-target-pair string to avoid
        // using absl::StatusOr for the return type.
        return p2p_config;
      }
      validation_kind = NcclP2PConfig::ValidationKind::kConditional;
      bounds = statusor_bounds.value();
    }
  }

  int i = 0;
  p2p_config.validation_kind = validation_kind;
  NcclP2PConfig::SourceTargetToBounds& source_target_to_bounds =
      p2p_config.source_target_to_bounds;
  for (const ReplicaGroup& replica_group : replica_groups) {
    int64_t source = replica_group.replica_ids(0);
    int64_t target = replica_group.replica_ids(1);

    p2p_config.id_to_source_target.insert({target, {}}).first->second.source =
        source;
    p2p_config.id_to_source_target.insert({source, {}}).first->second.target =
        target;

    if (validation_kind == NcclP2PConfig::ValidationKind::kConditional) {
      const ReplicaGroup& bound = bounds[i];
      int64_t lower = bound.replica_ids(0);
      int64_t upper = bound.replica_ids(1);
      source_target_to_bounds[std::make_pair(source, target)] =
          std::make_pair(lower, upper);
      i++;
    }
  }

  return p2p_config;
}

AsyncStreamKind GetStreamKindForSendRecv(const HloSendRecvInstruction* instr) {
  auto it = instr->frontend_attributes().map().find(kSendRecvPipelineAttr);
  if (it != instr->frontend_attributes().map().end() && it->second == "1") {
    return AsyncStreamKind::kP2P1;
  }
  return AsyncStreamKind::kP2P0;
}

}  // namespace gpu
}  // namespace xla
