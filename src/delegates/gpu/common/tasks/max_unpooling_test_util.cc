/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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

#include "delegates/gpu/common/tasks/max_unpooling_test_util.h"

#include <memory>
#include <vector>

#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/status.h"
#include "delegates/gpu/common/task/testing_util.h"
#include "delegates/gpu/common/tasks/max_unpooling.h"

namespace tflite {
namespace gpu {

absl::Status MaxUnpoolingTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 2, 1);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};
  TensorFloat32 src_ind_tensor;
  src_ind_tensor.shape = BHWC(1, 2, 2, 1);
  src_ind_tensor.data = {0.1f, 1.1f, 2.1f, 3.1f};

  MaxUnpooling2DAttributes attr;
  attr.padding.prepended = HW(0, 0);
  attr.padding.appended = HW(0, 0);
  attr.strides = HW(2, 2);
  attr.kernel = HW(2, 2);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation =
          CreateMaxUnpooling(env->GetGpuInfo(), op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          {src_tensor, src_ind_tensor},
          std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 4, 4, 1), &dst_tensor));
      RETURN_IF_ERROR(
          PointWiseNear({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 3.0f},
                        dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

}  // namespace gpu
}  // namespace tflite
