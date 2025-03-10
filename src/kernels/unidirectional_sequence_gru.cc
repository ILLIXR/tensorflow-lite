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

#include <limits>

#include "core/c/common.h"
#include "kernels/cpu_backend_context.h"
#include "kernels/gru_cell.h"
#include "kernels/internal/tensor.h"
#include "kernels/kernel_util.h"

// Unidirectional_sequence_gru is the fused version of GRU:
// https://www.tensorflow.org/api_docs/python/tf/keras/layers/GRU.
namespace tflite {
namespace ops {
namespace custom {
namespace unidirectional_sequence_gru {
namespace {

void GruImpl(const TfLiteTensor* input, const TfLiteTensor* input_state,
             const TfLiteTensor* gate_weight, const TfLiteTensor* gate_bias,
             const TfLiteTensor* candidate_weight,
             const TfLiteTensor* candidate_bias, TfLiteTensor* output,
             TfLiteTensor* output_state, TfLiteTensor* activation,
             TfLiteTensor* concat,
             tflite::CpuBackendContext* cpu_backend_context) {
  const int n_time = input->dims->data[0];
  const int n_batch = input->dims->data[1];
  const int n_input = input->dims->data[2];
  const int n_output = output->dims->data[2];
  const int n_batch_input = n_batch * n_input;
  const int n_batch_output = n_batch * n_output;
  const RuntimeShape input_shape({n_batch, n_input});
  const float* input_data = GetTensorData<float>(input);
  const RuntimeShape state_shape = GetTensorShape(input_state);
  const float* input_state_data = GetTensorData<float>(input_state);
  const RuntimeShape gate_weight_shape = GetTensorShape(gate_weight);
  const float* gate_weight_data = GetTensorData<float>(gate_weight);
  const RuntimeShape gate_bias_shape = GetTensorShape(gate_bias);
  const float* gate_bias_data = GetTensorData<float>(gate_bias);
  const RuntimeShape candidate_weight_shape = GetTensorShape(candidate_weight);
  const float* candidate_weight_data = GetTensorData<float>(candidate_weight);
  const RuntimeShape candidate_bias_shape = GetTensorShape(candidate_bias);
  const float* candidate_bias_data = GetTensorData<float>(candidate_bias);
  const RuntimeShape activation_shape = GetTensorShape(activation);
  const RuntimeShape output_shape = RuntimeShape({n_batch, n_output});
  float* output_data = GetTensorData<float>(output);
  float* output_state_data = GetTensorData<float>(output_state);
  float* activation_data = GetTensorData<float>(activation);
  const RuntimeShape concat_shape = GetTensorShape(concat);
  float* concat_data = GetTensorData<float>(concat);
  tflite::FullyConnectedParams fc_params;
  fc_params.float_activation_min = std::numeric_limits<float>::lowest();
  fc_params.float_activation_max = std::numeric_limits<float>::max();

  // The lhs is cacheable only when both gate weight & candidate weight are both
  // constants.
  fc_params.lhs_cacheable =
      IsConstantTensor(gate_weight) && IsConstantTensor(candidate_weight);
  fc_params.rhs_cacheable = false;
  for (int i = 0; i < n_time; ++i) {
    gru_cell::GruCell(
        input_shape, input_data, state_shape, input_state_data,
        gate_weight_shape, gate_weight_data, gate_bias_shape, gate_bias_data,
        candidate_weight_shape, candidate_weight_data, candidate_bias_shape,
        candidate_bias_data, output_shape, output_data, output_state_data,
        activation_shape, activation_data, concat_shape, concat_data, fc_params,
        cpu_backend_context);
    input_data += n_batch_input;
    output_data += n_batch_output;
    input_state_data = output_state_data;
  }
}

}  // namespace

enum InputTensor {
  // Input tensor of size [n_time, n_batch, n_input]
  kInput = 0,
  // Input state tensor of size [n_batch, n_output]
  kInputState = 1,
  // Gate weight tensor of size [2*n_output, n_input+n_output]
  kGateWeight = 2,
  // Gate bias tensor of size [2*n_output]
  kGateBias = 3,
  // Candidate weight tensor of size [n_output, n_input+n_output]
  kCandidateWeight = 4,
  // Candidate bias tensor of size [n_output]
  kCandidateBias = 5,
  kInputNum = 6
};

enum OutputTensor {
  // Input tensor of size [n_time, n_batch, n_output]
  kOutput = 0,
  // Output state tensor of size [n_batch, n_output]
  kOutputState = 1,
  kOutputNum = 2
};

enum TemporaryTensor {
  // Scratch buffer for activation of size [n_batch, 2*n_output]
  kActivation = 0,
  // Scratch buffer for activation of size [n_batch, n_input+n_output]
  kConcat = 1,
  kTemporaryNum = 2
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  auto* scratch_tensor_index = new int;
  context->AddTensors(context, kTemporaryNum, scratch_tensor_index);
  return scratch_tensor_index;
}

void Free(TfLiteContext* context, void* buffer) {
  delete reinterpret_cast<int*>(buffer);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  int* scratch_tensor_index = reinterpret_cast<int*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, node->inputs->size, kInputNum);
  TF_LITE_ENSURE_EQ(context, node->outputs->size, kOutputNum);

  // input's dim = [n_time, n_batch, n_input]
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInput, &input));
  TF_LITE_ENSURE_EQ(context, input->dims->size, 3);
  const int n_time = input->dims->data[0];
  const int n_batch = input->dims->data[1];
  const int n_input = input->dims->data[2];

  // input_state's dim = [n_batch, n_output]
  const TfLiteTensor* input_state;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputState, &input_state));
  TF_LITE_ENSURE_EQ(context, input_state->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, input_state->dims->data[0], n_batch);
  const int n_output = input_state->dims->data[1];

  // gate_weight' dim = [2 * n_output, n_input + n_output]
  const TfLiteTensor* gate_weight;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kGateWeight, &gate_weight));
  TF_LITE_ENSURE_EQ(context, gate_weight->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, gate_weight->dims->data[0], 2 * n_output);
  TF_LITE_ENSURE_EQ(context, gate_weight->dims->data[1], n_input + n_output);

  // gate_bias' dim = [2 * n_output]
  const TfLiteTensor* gate_bias;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kGateBias, &gate_bias));
  TF_LITE_ENSURE_EQ(context, gate_bias->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, gate_bias->dims->data[0], 2 * n_output);

  // candidate_weight' dim = [n_output, n_input + n_output]
  const TfLiteTensor* candidate_weight;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kCandidateWeight,
                                          &candidate_weight));
  TF_LITE_ENSURE_EQ(context, candidate_weight->dims->size, 2);
  TF_LITE_ENSURE_EQ(context, candidate_weight->dims->data[0], n_output);
  TF_LITE_ENSURE_EQ(context, candidate_weight->dims->data[1],
                    n_input + n_output);

  // candidate_bias' dim = [n_output]
  const TfLiteTensor* candidate_bias;
  TF_LITE_ENSURE_OK(
      context, GetInputSafe(context, node, kCandidateBias, &candidate_bias));
  TF_LITE_ENSURE_EQ(context, candidate_bias->dims->size, 1);
  TF_LITE_ENSURE_EQ(context, candidate_bias->dims->data[0], n_output);

  // output's dim = [n_time, n_batch, n_output]
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context, GetOutputSafe(context, node, kOutput, &output));
  TfLiteIntArray* output_size = TfLiteIntArrayCreate(3);
  output_size->data[0] = n_time;
  output_size->data[1] = n_batch;
  output_size->data[2] = n_output;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, output, output_size));

  // output_state's dim = [n_batch, n_output]
  TfLiteTensor* output_state;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputState, &output_state));
  TF_LITE_ENSURE_OK(
      context, context->ResizeTensor(context, output_state,
                                     TfLiteIntArrayCopy(input_state->dims)));

  TfLiteIntArrayFree(node->temporaries);
  node->temporaries = TfLiteIntArrayCreate(kTemporaryNum);

  // activation's dim = [n_batch, 2 * n_output]
  node->temporaries->data[kActivation] = *scratch_tensor_index;
  TfLiteTensor* activation;
  TF_LITE_ENSURE_OK(context,
                    GetTemporarySafe(context, node, kActivation, &activation));
  activation->type = input->type;
  activation->allocation_type = kTfLiteArenaRw;
  TfLiteIntArray* activation_size = TfLiteIntArrayCreate(2);
  activation_size->data[0] = n_batch;
  activation_size->data[1] = 2 * n_output;
  TF_LITE_ENSURE_OK(
      context, context->ResizeTensor(context, activation, activation_size));

  // concat's dim  = [n_batch, n_input + n_output]
  node->temporaries->data[kConcat] = (*scratch_tensor_index) + kConcat;
  TfLiteTensor* concat;
  TF_LITE_ENSURE_OK(context, GetTemporarySafe(context, node, kConcat, &concat));
  concat->type = input->type;
  concat->allocation_type = kTfLiteArenaRw;
  TfLiteIntArray* concat_size = TfLiteIntArrayCreate(2);
  concat_size->data[0] = n_batch;
  concat_size->data[1] = n_input + n_output;
  TF_LITE_ENSURE_OK(context,
                    context->ResizeTensor(context, concat, concat_size));

  return kTfLiteOk;
}

TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  const TfLiteTensor* input;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kInput, &input));
  const TfLiteTensor* input_state;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputState, &input_state));
  const TfLiteTensor* gate_weight;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kGateWeight, &gate_weight));
  const TfLiteTensor* gate_bias;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kGateBias, &gate_bias));
  const TfLiteTensor* candidate_weight;
  TF_LITE_ENSURE_OK(context, GetInputSafe(context, node, kCandidateWeight,
                                          &candidate_weight));
  const TfLiteTensor* candidate_bias;
  TF_LITE_ENSURE_OK(
      context, GetInputSafe(context, node, kCandidateBias, &candidate_bias));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context, GetOutputSafe(context, node, kOutput, &output));
  TfLiteTensor* output_state;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputState, &output_state));
  TfLiteTensor* activation;
  TF_LITE_ENSURE_OK(context,
                    GetTemporarySafe(context, node, kActivation, &activation));
  TfLiteTensor* concat;
  TF_LITE_ENSURE_OK(context, GetTemporarySafe(context, node, kConcat, &concat));
  auto cpu_backend_context = CpuBackendContext::GetFromContext(context);

  if (gate_weight->type == kTfLiteFloat32) {
    GruImpl(input, input_state, gate_weight, gate_bias, candidate_weight,
            candidate_bias, output, output_state, activation, concat,
            cpu_backend_context);
  } else {
    TF_LITE_KERNEL_LOG(context,
                       "Unsupported combination of data types for GruCell");
    return kTfLiteError;
  }

  return kTfLiteOk;
}

}  // namespace unidirectional_sequence_gru

TfLiteRegistration* Register_UNIDIRECTIONAL_SEQUENCE_GRU() {
  static TfLiteRegistration r = {
      unidirectional_sequence_gru::Init, unidirectional_sequence_gru::Free,
      unidirectional_sequence_gru::Prepare, unidirectional_sequence_gru::Eval};
  return &r;
}

}  // namespace custom
}  // namespace ops
}  // namespace tflite
