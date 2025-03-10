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

#include "delegates/gpu/common/tasks/padding_test_util.h"

#include <memory>
#include <vector>

#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/status.h"
#include "delegates/gpu/common/task/testing_util.h"
#include "delegates/gpu/common/tasks/padding.h"

namespace tflite {
namespace gpu {

absl::Status PaddingAppendWidthTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 0);
  attr.appended = BHWC(0, 0, 1, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 2, 2, 2), &dst_tensor));
      RETURN_IF_ERROR(
          PointWiseNear({0.0f, 1.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 0.0f},
                        dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingAppendWidthConstValuesTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 0);
  attr.appended = BHWC(0, 0, 1, 0);
  attr.constant_values = 5;

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 2, 2, 2), &dst_tensor));
      RETURN_IF_ERROR(
          PointWiseNear({0.0f, 1.0f, 5.0f, 5.0f, 2.0f, 3.0f, 5.0f, 5.0f},
                        dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingPrependWidthTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 1, 0);
  attr.appended = BHWC(0, 0, 0, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 2, 2, 2), &dst_tensor));
      RETURN_IF_ERROR(
          PointWiseNear({0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 2.0f, 3.0f},
                        dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingAppendHeightTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 0);
  attr.appended = BHWC(0, 1, 0, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 3, 1, 2), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({0.0f, 1.0f, 2.0f, 3.0f, 0.0f, 0.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingPrependHeightTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 1, 0, 0);
  attr.appended = BHWC(0, 0, 0, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 3, 1, 2), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingAppendChannelsTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 0);
  attr.appended = BHWC(0, 0, 0, 1);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 2, 1, 3), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({0.0f, 1.0f, 0.0f, 2.0f, 3.0f, 0.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingPrependChannelsTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 1);
  attr.appended = BHWC(0, 0, 0, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 2, 1, 3), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 3.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingPrependChannelsX4Test(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 1, 1, 2);
  src_tensor.data = {1.0f, 2.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 4);
  attr.appended = BHWC(0, 0, 0, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 1, 1, 6), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 2.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingComplexTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 2, 1, 2);
  src_tensor.data = {0.0f, 1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 1, 1);
  attr.appended = BHWC(0, 1, 1, 0);

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 3, 3, 3), &dst_tensor));
      RETURN_IF_ERROR(
          PointWiseNear({0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f},
                        dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingReflectWidthTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 1, 3, 1);
  src_tensor.data = {1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 2, 0);
  attr.appended = BHWC(0, 0, 2, 0);
  attr.type = PaddingContentType::REFLECT;

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 1, 7, 1), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({3.0f, 2.0f, 1.0f, 2.0f, 3.0f, 2.0f, 1.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

absl::Status PaddingReflectChannelsTest(TestExecutionEnvironment* env) {
  TensorFloat32 src_tensor;
  src_tensor.shape = BHWC(1, 1, 1, 3);
  src_tensor.data = {1.0f, 2.0f, 3.0f};

  PadAttributes attr;
  attr.prepended = BHWC(0, 0, 0, 2);
  attr.appended = BHWC(0, 0, 0, 2);
  attr.type = PaddingContentType::REFLECT;

  for (auto precision : env->GetSupportedPrecisions()) {
    auto data_type = DeduceDataTypeFromPrecision(precision);
    for (auto storage : env->GetSupportedStorages(data_type)) {
      const float eps = precision == CalculationsPrecision::F32 ? 1e-6f : 1e-3f;
      OperationDef op_def;
      op_def.precision = precision;
      op_def.src_tensors.push_back({data_type, storage, Layout::HWC});
      op_def.dst_tensors.push_back({data_type, storage, Layout::HWC});
      TensorFloat32 dst_tensor;
      GPUOperation operation = CreatePadding(op_def, attr);
      RETURN_IF_ERROR(env->ExecuteGPUOperation(
          src_tensor, std::make_unique<GPUOperation>(std::move(operation)),
          BHWC(1, 1, 1, 7), &dst_tensor));
      RETURN_IF_ERROR(PointWiseNear({3.0f, 2.0f, 1.0f, 2.0f, 3.0f, 2.0f, 1.0f},
                                    dst_tensor.data, eps));
    }
  }
  return absl::OkStatus();
}

}  // namespace gpu
}  // namespace tflite
