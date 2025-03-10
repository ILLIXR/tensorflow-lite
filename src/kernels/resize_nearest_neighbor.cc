/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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
#include "kernels/internal/reference/resize_nearest_neighbor.h"

#include <stdint.h>

#include "core/c/builtin_op_data.h"
#include "core/c/common.h"
#include "kernels/internal/compatibility.h"
#include "kernels/internal/optimized/neon_check.h"
#include "kernels/internal/optimized/optimized_ops.h"
#include "kernels/internal/reference/reference_ops.h"
#include "kernels/internal/tensor.h"
#include "kernels/internal/tensor_ctypes.h"
#include "kernels/internal/types.h"
#include "kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace resize_nearest_neighbor {

// This file has three implementations of RESIZE_NEAREST_NEIGHBOR.
enum KernelType {
  kReference,
  kGenericOptimized,
  kNeonOptimized,
};

constexpr int kInputTensor = 0;
constexpr int kSizeTensor = 1;
constexpr int kOutputTensor = 0;

TfLiteStatus ResizeOutputTensor(TfLiteContext* context,
                                const TfLiteTensor* input,
                                const TfLiteTensor* size,
                                TfLiteTensor* output) {
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(4);
  output_size->data[0] = input->dims->data[0];
  const int32* size_data = GetTensorData<int32>(size);
  output_size->data[1] = size_data[0];
  output_size->data[2] = size_data[1];
  output_size->data[3] = input->dims->data[3];
  return context->ResizeTensor(context, output, output_size);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  const TfLiteTensor* size;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kSizeTensor, &size));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  // Our current implementations relies on the input being 4D,
  // and the size being 1D tensor with exactly 2 elements.
  TF_LITE_ENSURE_EQ(context, NumDimensions(input), 4);
  TF_LITE_ENSURE_EQ(context, NumDimensions(size), 1);
  TF_LITE_ENSURE_TYPES_EQ(context, size->type, kTfLiteInt32);
  TF_LITE_ENSURE_EQ(context, size->dims->data[0], 2);

  output->type = input->type;

  if (!IsConstantOrPersistentTensor(size)) {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
  return ResizeOutputTensor(context, input, size, output);
}

template <KernelType kernel_type>
TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params =
      reinterpret_cast<TfLiteResizeNearestNeighborParams*>(node->builtin_data);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInputTensor, &input));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));
  const TfLiteTensor* size;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kSizeTensor, &size));

  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputTensor(context, input, size, output));
  }

  tflite::ResizeNearestNeighborParams op_params;
  op_params.align_corners = params->align_corners;
  op_params.half_pixel_centers = params->half_pixel_centers;

  if (output->type == kTfLiteFloat32) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int32>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int32>(output));
  } else if (output->type == kTfLiteUInt8) {
    if (kernel_type == kReference) {
      reference_ops::ResizeNearestNeighbor(
          op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
          GetTensorShape(size), GetTensorData<int32>(size),
          GetTensorShape(output), GetTensorData<uint8_t>(output));
    }
    if (kernel_type == kGenericOptimized || kernel_type == kNeonOptimized) {
      optimized_ops::ResizeNearestNeighbor(
          op_params, GetTensorShape(input), GetTensorData<uint8_t>(input),
          GetTensorShape(size), GetTensorData<int32>(size),
          GetTensorShape(output), GetTensorData<uint8_t>(output));
    }
  } else if (output->type == kTfLiteInt8) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int8_t>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int8_t>(output));
  } else if (output->type == kTfLiteInt16) {
    reference_ops::ResizeNearestNeighbor(
        op_params, GetTensorShape(input), GetTensorData<int16_t>(input),
        GetTensorShape(size), GetTensorData<int32>(size),
        GetTensorShape(output), GetTensorData<int16_t>(output));
  } else {
    TF_LITE_KERNEL_LOG(
        context, "Output type is %s, requires float, uint8, int8 or int16.",
        TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }

  return kTfLiteOk;
}

}  // namespace resize_nearest_neighbor

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR_REF() {
  static TfLiteRegistration r = {
      nullptr, nullptr, resize_nearest_neighbor::Prepare,
      resize_nearest_neighbor::Eval<resize_nearest_neighbor::kReference>};
  return &r;
}

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR_GENERIC_OPT() {
  static TfLiteRegistration r = {
      nullptr, nullptr, resize_nearest_neighbor::Prepare,
      resize_nearest_neighbor::Eval<
          resize_nearest_neighbor::kGenericOptimized>};
  return &r;
}

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR_NEON_OPT() {
  static TfLiteRegistration r = {
      nullptr, nullptr, resize_nearest_neighbor::Prepare,
      resize_nearest_neighbor::Eval<resize_nearest_neighbor::kNeonOptimized>};
  return &r;
}

TfLiteRegistration* Register_RESIZE_NEAREST_NEIGHBOR() {
#ifdef USE_NEON
  return Register_RESIZE_NEAREST_NEIGHBOR_NEON_OPT();
#else
  return Register_RESIZE_NEAREST_NEIGHBOR_GENERIC_OPT();
#endif
}

}  // namespace builtin
}  // namespace ops
}  // namespace tflite
