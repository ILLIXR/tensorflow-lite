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

#ifndef TENSORFLOW_LITE_DELEGATES_GPU_COMMON_TASKS_SPLIT_H_
#define TENSORFLOW_LITE_DELEGATES_GPU_COMMON_TASKS_SPLIT_H_

#include <string>

#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/task/gpu_operation.h"
#include "delegates/gpu/common/types.h"

namespace tflite {
namespace gpu {

class Split : public GPUOperation {
 public:
  Split(const GpuInfo& gpu_info, const OperationDef& definition,
        const SplitAttributes& attr, const std::vector<int>& channels);
  int3 GetGridSize() const override;

  // Move only
  Split(Split&& operation) = default;
  Split& operator=(Split&& operation) = default;
  Split(const Split&) = delete;
  Split& operator=(const Split&) = delete;

 private:
  std::string GetSplitCode();
  std::string GetSplitChannelsCode(const GpuInfo& gpu_info,
                                   const std::vector<int>& channels);

  SplitAttributes attr_;
};

Split CreateSplit(const GpuInfo& gpu_info, const OperationDef& definition,
                  const SplitAttributes& attr,
                  const std::vector<int>& channels);

}  // namespace gpu
}  // namespace tflite

#endif  // TENSORFLOW_LITE_DELEGATES_GPU_COMMON_TASKS_SPLIT_H_
