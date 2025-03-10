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
#include <stdint.h>
#include <string.h>

#include "core/c/common.h"
#include "kernels/internal/reference/reference_ops.h"
#include "kernels/internal/tensor.h"
#include "kernels/internal/tensor_ctypes.h"
#include "kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace expand_dims {

// Input indices
enum { kInput = 0, kAxis };

namespace {
TfLiteStatus ExpandTensorDim(TfLiteContext* context, const TfLiteTensor& input,
                             int axis, TfLiteTensor* output) {
  const TfLiteIntArray& input_dims = *input.dims;
  if (axis < 0) {
    axis = input_dims.size + 1 + axis;
  }
  TF_LITE_ENSURE(context, axis <= input_dims.size);
  TF_LITE_ENSURE(context, axis >= 0);

  TfLiteIntArray* output_dims = TfLiteIntArrayCreate(input_dims.size + 1);
  for (int i = 0; i < output_dims->size; ++i) {
    if (i < axis) {
      output_dims->data[i] = input_dims.data[i];
    } else if (i == axis) {
      output_dims->data[i] = 1;
    } else {
      output_dims->data[i] = input_dims.data[i - 1];
    }
  }

  return context->ResizeTensor(context, output, output_dims);
}

TfLiteStatus GetAxisValueFromTensor(TfLiteContext* context,
                                    const TfLiteTensor& axis, int* axis_value) {
  TF_LITE_ENSURE_EQ(context, NumElements(&axis), 1);
  switch (axis.type) {
    case kTfLiteInt32:
      *axis_value = *GetTensorData<int32_t>(&axis);
      return kTfLiteOk;
    case kTfLiteInt64:
      *axis_value = *GetTensorData<int64_t>(&axis);
      return kTfLiteOk;
    default:
      return kTfLiteError;
  }
}

}  // namespace

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInput, &input));
  const TfLiteTensor* axis;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kAxis, &axis));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context, GetOutputSafe(context, node, 0, &output));

  output->type = input->type;
  TF_LITE_ENSURE_EQ(context, input->params.scale, output->params.scale);
  TF_LITE_ENSURE_EQ(context, input->params.zero_point,
                    output->params.zero_point);
  if (input->type == kTfLiteInt16) {
    TF_LITE_ENSURE_EQ(context, input->params.zero_point, 0);
  }

  if (IsConstantOrPersistentTensor(axis)) {
    int axis_value;
    TF_LITE_ENSURE_OK(context,
                      GetAxisValueFromTensor(context, *axis, &axis_value));
    return ExpandTensorDim(context, *input, axis_value, output);
  }
  SetTensorToDynamic(output);

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  // Just copy input to output.
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInput, &input));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context, GetOutputSafe(context, node, 0, &output));
  const TfLiteTensor* axis;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kAxis, &axis));
  if (IsDynamicTensor(output)) {
    int axis_value;
    TF_LITE_ENSURE_OK(context,
                      GetAxisValueFromTensor(context, *axis, &axis_value));
    TF_LITE_ENSURE_OK(context,
                      ExpandTensorDim(context, *input, axis_value, output));
  }
  if (output->type == kTfLiteString) {
    TfLiteTensorRealloc(input->bytes, output);
  }
  // Only copy data if input and output do not share a buffer.
  if (output->data.data != input->data.data) {
    memcpy(output->data.data, input->data.data, input->bytes);
  }
  return kTfLiteOk;
}

}  // namespace expand_dims
TfLiteRegistration* Register_EXPAND_DIMS() {
  static TfLiteRegistration r = {
      nullptr,
      nullptr,
      expand_dims::Prepare,
      expand_dims::Eval,
      /*profiling_string=*/nullptr,
      /*builtin_code=*/0,
      /*custom_name=*/nullptr,
      /*version=*/0,
      /*registration_external=*/nullptr,
      /*async_kernel=*/nullptr,
      kTfLiteInplaceOpInput0Shared | kTfLiteInplaceOpDataUnmodified};
  return &r;
}
}  // namespace builtin
}  // namespace ops
}  // namespace tflite
