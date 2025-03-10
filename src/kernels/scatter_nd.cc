/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include "core/c/common.h"
#include "kernels/internal/optimized/optimized_ops.h"
#include "kernels/internal/reference/reference_ops.h"
#include "kernels/internal/tensor.h"
#include "kernels/internal/tensor_ctypes.h"
#include "kernels/internal/types.h"
#include "kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace scatter_nd {
constexpr int kIndices = 0;
constexpr int kUpdates = 1;
constexpr int kShape = 2;
constexpr int kOutputTensor = 0;

template <typename IndicesT>
TfLiteStatus ResizeOutputTensor(TfLiteContext* context,
                                const TfLiteTensor* shape,
                                TfLiteTensor* output) {
  const int shape_rank = SizeOfDimension(shape, 0);
  TfLiteIntArray* output_shape = TfLiteIntArrayCreate(shape_rank);
  const auto* shape_data = GetTensorData<IndicesT>(shape);

  for (int i = 0; i < shape_rank; i++) {
    output_shape->data[i] = shape_data[i];
  }
  return context->ResizeTensor(context, output, output_shape);
}

template <typename IndicesT>
TfLiteStatus CheckShapes(TfLiteContext* context, const RuntimeShape& indices,
                         const RuntimeShape& updates,
                         const RuntimeShape& shape_shape,
                         const IndicesT* shape_data) {
  TF_LITE_ENSURE(context, (indices.DimensionsCount() >= 1) &&
                              (updates.DimensionsCount() >= 1) &&
                              (shape_shape.DimensionsCount() == 1));

  const int outer_dims = indices.DimensionsCount() - 1;
  for (int i = 0; i < outer_dims; ++i) {
    TF_LITE_ENSURE_EQ(context, indices.Dims(i), updates.Dims(i));
  }

  const int ix = indices.Dims(outer_dims);
  TF_LITE_ENSURE_EQ(context, updates.DimensionsCount() - outer_dims,
                    shape_shape.Dims(0) - ix);
  for (int i = 0; i + outer_dims < updates.DimensionsCount(); ++i) {
    TF_LITE_ENSURE_EQ(context, updates.Dims(i + outer_dims),
                      shape_data[ix + i]);
  }
  return kTfLiteOk;
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  TF_LITE_ENSURE_EQ(context, NumInputs(node), 3);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* indices;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kIndices, &indices));
  const TfLiteTensor* updates;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kUpdates, &updates));
  const TfLiteTensor* shape;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kShape, &shape));

  switch (updates->type) {
    case kTfLiteFloat32:
    case kTfLiteUInt8:
    case kTfLiteBool:
    case kTfLiteInt8:
    case kTfLiteInt64:
    case kTfLiteInt32:
      break;
    default:
      TF_LITE_KERNEL_LOG(
          context, "Updates of type '%s' are not supported by scatter_nd.",
          TfLiteTypeGetName(updates->type));
      return kTfLiteError;
  }
  if (indices->type != shape->type) {
    TF_LITE_KERNEL_LOG(context, "Indices and shape must have the same type.");
    return kTfLiteError;
  }

  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));
  output->type = updates->type;

  if (IsConstantOrPersistentTensor(shape)) {
    switch (indices->type) {
      case kTfLiteInt32:
        TF_LITE_ENSURE_OK(
            context,
            CheckShapes<int32_t>(context, GetTensorShape(indices),
                                 GetTensorShape(updates), GetTensorShape(shape),
                                 GetTensorData<int32_t>(shape)));
        return ResizeOutputTensor<int32_t>(context, shape, output);
      default:
        TF_LITE_KERNEL_LOG(
            context, "Indices of type '%s' are not supported by scatter_nd.",
            TfLiteTypeGetName(indices->type));
        return kTfLiteError;
    }
  } else {
    SetTensorToDynamic(output);
    return kTfLiteOk;
  }
}

template <typename IndicesT, typename UpdatesT>
TfLiteStatus ScatterNd(const TfLiteTensor* indices, const TfLiteTensor* updates,
                       TfLiteTensor* output) {
  return reference_ops::ScatterNd(
      GetTensorShape(indices), GetTensorData<IndicesT>(indices),
      GetTensorShape(updates), GetTensorData<UpdatesT>(updates),
      GetTensorShape(output), GetTensorData<UpdatesT>(output));
}

template <typename IndicesT>
TfLiteStatus EvalScatterNd(TfLiteContext* context, const TfLiteTensor* indices,
                           const TfLiteTensor* updates,
                           const TfLiteTensor* shape, TfLiteTensor* output) {
  if (IsDynamicTensor(output)) {
    TF_LITE_ENSURE_OK(
        context, CheckShapes<IndicesT>(
                     context, GetTensorShape(indices), GetTensorShape(updates),
                     GetTensorShape(shape), GetTensorData<IndicesT>(shape)));
    TF_LITE_ENSURE_OK(context,
                      ResizeOutputTensor<IndicesT>(context, shape, output));
  }

  TfLiteStatus status = kTfLiteError;
  switch (updates->type) {
    case kTfLiteFloat32:
      status = ScatterNd<IndicesT, float>(indices, updates, output);
      break;
    case kTfLiteUInt8:
      status = ScatterNd<IndicesT, uint8_t>(indices, updates, output);
      break;
    case kTfLiteBool:
      status = ScatterNd<IndicesT, bool>(indices, updates, output);
      break;
    case kTfLiteInt8:
      status = ScatterNd<IndicesT, int8_t>(indices, updates, output);
      break;
    case kTfLiteInt32:
      status = ScatterNd<IndicesT, int32_t>(indices, updates, output);
      break;
    case kTfLiteInt64:
      status = ScatterNd<IndicesT, int64_t>(indices, updates, output);
      break;
    default:
      TF_LITE_KERNEL_LOG(
          context, "Updates of type '%s' are not supported by scatter_nd.",
          TfLiteTypeGetName(updates->type));
      return kTfLiteError;
  }
  if (status != kTfLiteOk) {
    TF_LITE_KERNEL_LOG(context, "scatter_nd index out of bounds");
  }
  return status;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* indices;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kIndices, &indices));
  const TfLiteTensor* updates;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kUpdates, &updates));
  const TfLiteTensor* shape;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kShape, &shape));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  switch (indices->type) {
    case kTfLiteInt32:
      return EvalScatterNd<int32_t>(context, indices, updates, shape, output);
    default:
      TF_LITE_KERNEL_LOG(
          context, "Indices of type '%s' are not supported by scatter_nd.",
          TfLiteTypeGetName(indices->type));
      return kTfLiteError;
  }
}

}  // namespace scatter_nd

TfLiteRegistration* Register_SCATTER_ND() {
  static TfLiteRegistration r = {/*init*/ nullptr, /*free*/ nullptr,
                                 scatter_nd::Prepare, scatter_nd::Eval};
  return &r;
}
}  // namespace builtin
}  // namespace ops
}  // namespace tflite
