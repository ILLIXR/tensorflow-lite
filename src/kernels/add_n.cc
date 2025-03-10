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

#include <algorithm>

#include "core/c/common.h"
#include "kernels/cpu_backend_threadpool.h"
#include "kernels/internal/optimized/optimized_ops.h"
#include "kernels/internal/tensor.h"
#include "kernels/internal/tensor_ctypes.h"
#include "kernels/kernel_util.h"

namespace tflite {
namespace ops {
namespace builtin {
namespace add_n {

constexpr int kInputTensor1 = 0;
constexpr int kOutputTensor = 0;

struct OpData {
  // The index of the temporary tensor where temporary accumulations are kept.
  int scratch_tensor_index;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  auto* op_data = new OpData();
  context->AddTensors(context, 1, &op_data->scratch_tensor_index);
  return op_data;
}

void Free(TfLiteContext* context, void* buffer) {
  delete reinterpret_cast<OpData*>(buffer);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  int num_inputs = NumInputs(node);
  TF_LITE_ENSURE(context, num_inputs >= 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor1, &input1));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));
  output->type = input1->type;

  OpData* op_data = reinterpret_cast<OpData*>(node->user_data);
  TfLiteIntArrayFree(node->temporaries);
  node->temporaries = TfLiteIntArrayCreate(1);
  node->temporaries->data[0] = op_data->scratch_tensor_index;
  TfLiteTensor* scratch_tensor;
  TF_LITE_ENSURE_OK(
      context, GetTemporarySafe(context, node, /*index=*/0, &scratch_tensor));
  scratch_tensor->type = input1->type;
  scratch_tensor->allocation_type = kTfLiteArenaRw;

  CpuBackendContext* cpu_backend_context =
      CpuBackendContext::GetFromContext(context);
  // Choose the proper number of thread so that:
  // (1) Each thread gets at least two tensors (1 if we only have 1 input
  // tensor).
  // (2) Total thread_count should be bounded by the maximimum allowed threads.
  // (3) Tensors are distributed evenly across different threads.
  const int thread_count =
      std::min(std::max(1, static_cast<int>(num_inputs) / 2),
               cpu_backend_context->max_num_threads());

  TfLiteIntArray* scratch_shape = TfLiteIntArrayCreate(1);
  scratch_shape->data[0] = thread_count * NumElements(input1);
  TF_LITE_ENSURE_OK(
      context, context->ResizeTensor(context, scratch_tensor, scratch_shape));

  // Check that all input tensors have the same shape and type.
  for (int i = kInputTensor1 + 1; i < num_inputs; ++i) {
    const TfLiteTensor* input;
    TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, i, &input));
    TF_LITE_ENSURE(context, HaveSameShapes(input1, input));
    TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input->type);
  }

  // Use the first input node's dimension to be the dimension of the output
  // node.
  TfLiteIntArray* input1_dims = input1->dims;
  TfLiteIntArray* output_dims = TfLiteIntArrayCopy(input1_dims);
  return context->ResizeTensor(context, output, output_dims);
}

template <typename T>
TfLiteStatus EvalAddN(TfLiteContext* context, TfLiteNode* node) {
  // TODO(haoliang): Initialize all_inputs only once during init.
  VectorOfTensors<T> all_inputs(*context, *node->inputs);
  // Safe to use unchecked since caller checks that tensor is valid
  TfLiteTensor* output = GetOutput(context, node, kOutputTensor);
  int num_inputs = NumInputs(node);
  // Safe to use unchecked since caller checks that tensor is valid
  const TfLiteTensor* input1 = GetInput(context, node, kInputTensor1);

  // Fetch backend context and number of threads.
  CpuBackendContext* cpu_backend_context =
      CpuBackendContext::GetFromContext(context);
  TfLiteTensor* scratch_tensor;
  TF_LITE_ENSURE_OK(context,
                    GetTemporarySafe(context, node, 0, &scratch_tensor));
  optimized_ops::AddN<T>(GetTensorShape(input1), num_inputs, all_inputs.data(),
                         GetTensorData<T>(output),
                         GetTensorData<T>(scratch_tensor), cpu_backend_context);
  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input1;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor1, &input1));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));
  if (output->type == kTfLiteFloat32) {
    TF_LITE_ENSURE_OK(context, EvalAddN<float>(context, node));
  } else if (output->type == kTfLiteInt32) {
    TF_LITE_ENSURE_OK(context, EvalAddN<int32_t>(context, node));
  } else {
    TF_LITE_KERNEL_LOG(context, "AddN only supports FLOAT32|INT32 now, got %s.",
                       TfLiteTypeGetName(output->type));
    return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace add_n

TfLiteRegistration* Register_ADD_N() {
  static TfLiteRegistration r = {add_n::Init, add_n::Free, add_n::Prepare,
                                 add_n::Eval};
  return &r;
}

}  // namespace builtin
}  // namespace ops
}  // namespace tflite
