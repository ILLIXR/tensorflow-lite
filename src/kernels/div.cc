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
#include <stddef.h>
#include <stdint.h>

#include "core/c/builtin_op_data.h"
#include "core/c/common.h"
#include "kernels/internal/compatibility.h"
#include "kernels/internal/optimized/cpu_check.h"
#include "kernels/internal/optimized/neon_check.h"
#include "kernels/internal/optimized/optimized_ops.h"
#include "kernels/internal/quantization_util.h"
#include "kernels/internal/reference/process_broadcast_shapes.h"
#include "kernels/internal/reference/reference_ops.h"
#include "kernels/internal/tensor.h"
#include "kernels/internal/tensor_ctypes.h"
#include "kernels/internal/types.h"
#include "kernels/kernel_util.h"

#ifdef TFLITE_KERNEL_USE_XNNPACK
#include <algorithm>
#include <array>
#include <limits>

#include "tfl-xnnpack.h"  // from @XNNPACK
#include "kernels/cpu_backend_context.h"
#include "minimal_logging.h"
#endif  // TFLITE_KERNEL_USE_XNNPACK

namespace tflite {
namespace ops {
namespace builtin {
namespace div {

// This file has three implementation of Div.
enum KernelType {
  kReference,
  kGenericOptimized,  // Neon-free
  kNeonOptimized,
};

constexpr int kInputTensor1 = 0;
constexpr int kInputTensor2 = 1;
constexpr int kOutputTensor = 0;

struct OpData {
  bool requires_broadcast;

  // Parameters used in the quantized paths where the output is 8bit
  int32 output_activation_min;
  int32 output_activation_max;

  // Parameters used in all quantized paths
  int32_t output_multiplier;
  int output_shift;
};

void* Init(TfLiteContext* context, const char* buffer, size_t length) {
  auto* data = new OpData;
  data->requires_broadcast = false;
  return data;
}

void Free(TfLiteContext* context, void* buffer) {
  delete reinterpret_cast<OpData*>(buffer);
}

TfLiteStatus Prepare(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteDivParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  TF_LITE_ENSURE_EQ(context, NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, NumOutputs(node), 1);

  const TfLiteTensor* input1;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor1, &input1));
  const TfLiteTensor* input2;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor2, &input2));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));

  TF_LITE_ENSURE_TYPES_EQ(context, input1->type, input2->type);
  output->type = input2->type;

  data->requires_broadcast = !HaveSameShapes(input1, input2);

  TfLiteIntArray* output_size = nullptr;
  if (data->requires_broadcast) {
    TF_LITE_ENSURE_OK(context, CalculateShapeForBroadcast(
                                   context, input1, input2, &output_size));
  } else {
    output_size = TfLiteIntArrayCopy(input1->dims);
  }

  if (output->type == kTfLiteUInt8) {
    TF_LITE_ENSURE_STATUS(CalculateActivationRangeQuantized(
        context, params->activation, output, &data->output_activation_min,
        &data->output_activation_max));
    const double real_multiplier =
        input1->params.scale / (input2->params.scale * output->params.scale);
    QuantizeMultiplier(real_multiplier, &data->output_multiplier,
                       &data->output_shift);
  }

  return context->ResizeTensor(context, output, output_size);
}

template <KernelType kernel_type>
void EvalDiv(TfLiteContext* context, TfLiteNode* node, TfLiteDivParams* params,
             const OpData* data, const TfLiteTensor* input1,
             const TfLiteTensor* input2, TfLiteTensor* output) {
#define TF_LITE_DIV(type, opname, data_type)                             \
  tflite::ArithmeticParams op_params;                                    \
  data_type output_activation_min, output_activation_max;                \
  CalculateActivationRange(params->activation, &output_activation_min,   \
                           &output_activation_max);                      \
  SetActivationParams(output_activation_min, output_activation_max,      \
                      &op_params);                                       \
  type::opname(op_params, GetTensorShape(input1),                        \
               GetTensorData<data_type>(input1), GetTensorShape(input2), \
               GetTensorData<data_type>(input2), GetTensorShape(output), \
               GetTensorData<data_type>(output))
  if (output->type == kTfLiteInt32) {
    if (kernel_type == kReference) {
      if (data->requires_broadcast) {
        TF_LITE_DIV(reference_ops, BroadcastDivSlow, int32_t);
      } else {
        TF_LITE_DIV(reference_ops, Div, int32_t);
      }
    } else {
      if (data->requires_broadcast) {
        TF_LITE_DIV(optimized_ops, BroadcastDivSlow, int32_t);
      } else {
        TF_LITE_DIV(optimized_ops, Div, int32_t);
      }
    }
  } else if (output->type == kTfLiteFloat32) {
    if (kernel_type == kReference) {
      if (data->requires_broadcast) {
        TF_LITE_DIV(reference_ops, BroadcastDivSlow, float);
      } else {
        TF_LITE_DIV(reference_ops, Div, float);
      }
    } else {
#ifdef TFLITE_KERNEL_USE_XNNPACK
      size_t num_input1_dims =
          static_cast<size_t>(GetTensorShape(input1).DimensionsCount());
      size_t num_input2_dims =
          static_cast<size_t>(GetTensorShape(input2).DimensionsCount());
      if (std::max(num_input1_dims, num_input2_dims) <= XNN_MAX_TENSOR_DIMS) {
        std::array<size_t, XNN_MAX_TENSOR_DIMS> input1_shape;
        std::array<size_t, XNN_MAX_TENSOR_DIMS> input2_shape;
        for (size_t i = 0; i < num_input1_dims; ++i) {
          input1_shape[i] = GetTensorShape(input1).Dims(i);
        }
        for (size_t i = 0; i < num_input2_dims; ++i) {
          input2_shape[i] = GetTensorShape(input2).Dims(i);
        }
        CpuBackendContext* cpu_backend_context =
            CpuBackendContext::GetFromContext(context);
        pthreadpool_t threadpool =
            cpu_backend_context->get_xnnpack_threadpool();
        float output_min = -std::numeric_limits<float>::infinity();
        float output_max = std::numeric_limits<float>::infinity();
        CalculateActivationRange(params->activation, &output_min, &output_max);
        // NOTE: In the case of NaN inputs the behavior is platform dependent.
        // Passing in NaN values should not cause hardware exceptions, but NaN
        // propagation is not guaranteed.
        const enum xnn_status status = xnn_run_divide_nd_f32(
            num_input1_dims, input1_shape.data(), num_input2_dims,
            input2_shape.data(), GetTensorData<float>(input1),
            GetTensorData<float>(input2), GetTensorData<float>(output),
            output_min, output_max,
            /*flags=*/XNN_FLAG_YIELD_WORKERS, threadpool);
        if (status == xnn_status_success) {
          return;
        }
        TFLITE_LOG(
            TFLITE_LOG_INFO,
            "Failed to run xnnpack xnn_run_divide_nd_f32. Error code: %d",
            status);
      }
#endif  // TFLITE_KERNEL_USE_XNNPACK
      if (data->requires_broadcast) {
        TF_LITE_DIV(optimized_ops, BroadcastDivSlow, float);
      } else {
        TF_LITE_DIV(optimized_ops, Div, float);
      }
    }
  }
#undef TF_LITE_DIV
}

template <KernelType kernel_type>
TfLiteStatus EvalQuantized(TfLiteContext* context, TfLiteNode* node,
                           TfLiteDivParams* params, const OpData* data,
                           const TfLiteTensor* input1,
                           const TfLiteTensor* input2, TfLiteTensor* output) {
  if (input1->type == kTfLiteUInt8 && input2->type == kTfLiteUInt8 &&
      output->type == kTfLiteUInt8) {
    tflite::ArithmeticParams op_params;
    SetActivationParams(data->output_activation_min,
                        data->output_activation_max, &op_params);
    op_params.input1_offset = -input1->params.zero_point;
    op_params.input2_offset = -input2->params.zero_point;
    op_params.output_offset = output->params.zero_point;
    op_params.output_multiplier = data->output_multiplier;
    op_params.output_shift = data->output_shift;
    bool need_broadcast = optimized_ops::ProcessBroadcastShapes(
        GetTensorShape(input1), GetTensorShape(input2), &op_params);
#define TF_LITE_DIV(type, opname, dtype)                             \
  type::opname(op_params, GetTensorShape(input1),                    \
               GetTensorData<dtype>(input1), GetTensorShape(input2), \
               GetTensorData<dtype>(input2), GetTensorShape(output), \
               GetTensorData<dtype>(output))
    if (kernel_type == kReference) {
      if (need_broadcast) {
        TF_LITE_DIV(reference_ops, BroadcastDivSlow, uint8_t);
      } else {
        TF_LITE_DIV(reference_ops, Div, uint8_t);
      }
    } else {
      if (need_broadcast) {
        TF_LITE_DIV(optimized_ops, BroadcastDivSlow, uint8_t);
      } else {
        TF_LITE_DIV(optimized_ops, Div, uint8_t);
      }
    }
#undef TF_LITE_DIV
  } else {
    TF_LITE_KERNEL_LOG(
        context, "Unsupported combination of input and output types in Div.");
    return kTfLiteError;
  }
  return kTfLiteOk;
}

template <typename T>
TfLiteStatus CheckNonZero(TfLiteContext* context, const TfLiteTensor* tensor) {
  const auto* data = GetTensorData<T>(tensor);
  const size_t number_elements = tensor->bytes / sizeof(T);
  for (size_t i = 0; i < number_elements; i++) {
    TF_LITE_ENSURE(context, data[i] != 0);
  }
  return kTfLiteOk;
}

template <KernelType kernel_type>
TfLiteStatus Eval(TfLiteContext* context, TfLiteNode* node) {
  auto* params = reinterpret_cast<TfLiteDivParams*>(node->builtin_data);
  OpData* data = reinterpret_cast<OpData*>(node->user_data);

  const TfLiteTensor* input1;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor1, &input1));
  const TfLiteTensor* input2;
  TF_LITE_ENSURE_OK(context,
                    GetInputSafe(context, node, kInputTensor2, &input2));
  TfLiteTensor* output;
  TF_LITE_ENSURE_OK(context,
                    GetOutputSafe(context, node, kOutputTensor, &output));


  if (output->type == kTfLiteFloat32) {
    // Div by zero seems ok in this case, we don't do a check at this point.
    // However, unlike in TF where infinities are returned, here we return an
    // activation min/max value if any or std::numeric_limits<float>::min/max.
    EvalDiv<kernel_type>(context, node, params, data, input1, input2, output);
  } else if (output->type == kTfLiteInt32) {
    CheckNonZero<int32_t>(context, input2);
    EvalDiv<kernel_type>(context, node, params, data, input1, input2, output);
  } else if (output->type == kTfLiteUInt8) {
    CheckNonZero<uint8_t>(context, input2);
    TF_LITE_ENSURE_OK(
        context, EvalQuantized<kernel_type>(context, node, params, data, input1,
                                            input2, output));
  } else {
    TF_LITE_KERNEL_LOG(
        context,
        "Div only supports FLOAT32, INT32 and quantized UINT8 now, got %d.",
        output->type);
    return kTfLiteError;
  }

  return kTfLiteOk;
}

}  // namespace div

TfLiteRegistration* Register_DIV_REF() {
  static TfLiteRegistration r = {
      div::Init,
      div::Free,
      div::Prepare,
      div::Eval<div::kReference>,
      /*profiling_string=*/nullptr,
      /*builtin_code=*/0,
      /*custom_name=*/nullptr,
      /*version=*/0,
      /*registration_external=*/nullptr,
      /*async_kernel=*/nullptr,
      kTfLiteInplaceOpInput0Shared | kTfLiteInplaceOpInput1Shared};
  return &r;
}

TfLiteRegistration* Register_DIV_GENERIC_OPT() {
  static TfLiteRegistration r = {
      div::Init,
      div::Free,
      div::Prepare,
      div::Eval<div::kGenericOptimized>,
      /*profiling_string=*/nullptr,
      /*builtin_code=*/0,
      /*custom_name=*/nullptr,
      /*version=*/0,
      /*registration_external=*/nullptr,
      /*async_kernel=*/nullptr,
      kTfLiteInplaceOpInput0Shared | kTfLiteInplaceOpInput1Shared};
  return &r;
}

TfLiteRegistration* Register_DIV_NEON_OPT() {
  static TfLiteRegistration r = {
      div::Init,
      div::Free,
      div::Prepare,
      div::Eval<div::kNeonOptimized>,
      /*profiling_string=*/nullptr,
      /*builtin_code=*/0,
      /*custom_name=*/nullptr,
      /*version=*/0,
      /*registration_external=*/nullptr,
      /*async_kernel=*/nullptr,
      kTfLiteInplaceOpInput0Shared | kTfLiteInplaceOpInput1Shared};
  return &r;
}

TfLiteRegistration* Register_DIV() {
#ifdef USE_NEON
  return Register_DIV_NEON_OPT();
#else
  return Register_DIV_GENERIC_OPT();
#endif
}

}  // namespace builtin
}  // namespace ops
}  // namespace tflite
