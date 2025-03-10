/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

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

#include "delegates/gpu/gl/kernels/registry.h"

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/status.h"
#include "delegates/gpu/gl/kernels/add.h"
#include "delegates/gpu/gl/kernels/concat.h"
#include "delegates/gpu/gl/kernels/conv.h"
#include "delegates/gpu/gl/kernels/custom_registry.h"
#include "delegates/gpu/gl/kernels/depthwise_conv.h"
#include "delegates/gpu/gl/kernels/elementwise.h"
#include "delegates/gpu/gl/kernels/fully_connected.h"
#include "delegates/gpu/gl/kernels/lstm.h"
#include "delegates/gpu/gl/kernels/mean.h"
#include "delegates/gpu/gl/kernels/mul.h"
#include "delegates/gpu/gl/kernels/pad.h"
#include "delegates/gpu/gl/kernels/pooling.h"
#include "delegates/gpu/gl/kernels/prelu.h"
#include "delegates/gpu/gl/kernels/quantize_and_dequantize.h"
#include "delegates/gpu/gl/kernels/relu.h"
#include "delegates/gpu/gl/kernels/resampler.h"
#include "delegates/gpu/gl/kernels/reshape.h"
#include "delegates/gpu/gl/kernels/resize.h"
#include "delegates/gpu/gl/kernels/slice.h"
#include "delegates/gpu/gl/kernels/softmax.h"
#include "delegates/gpu/gl/kernels/space_to_depth.h"
#include "delegates/gpu/gl/kernels/tile.h"
#include "delegates/gpu/gl/kernels/transpose_conv.h"

#ifndef TFLITE_GPU_BINARY_RELEASE
#include "delegates/gpu/gl/kernels/max_unpooling.h"
#endif  // TFLITE_GPU_BINARY_RELEASE

namespace tflite {
namespace gpu {
namespace gl {
namespace {

class Registry : public NodeShader {
 public:
  Registry() {
    using Type = OperationType;
    using NewShaderFunc = std::function<std::unique_ptr<NodeShader>()>;

    const auto insert_op = [&](Type type, NewShaderFunc func) {
      shaders_[ToString(type)].push_back(func());
    };
    const auto insert_elementwise_op = [&](Type operation_type) {
      shaders_[ToString(operation_type)].push_back(
          NewElementwiseNodeShader(operation_type));
    };

    insert_op(Type::ADD, NewAddNodeShader);
    insert_op(Type::CONCAT, NewAlignedConcatNodeShader);
    insert_op(Type::CONCAT, NewFlatConcatNodeShader);
    insert_op(Type::CONCAT, NewConcatNodeShader);
    insert_op(Type::CONVOLUTION_2D, NewConvolution1x1NodeShader);
    insert_op(Type::CONVOLUTION_2D, NewConvolutionNodeShader);
    insert_op(Type::CONVOLUTION_TRANSPOSED, NewConvolutionTransposedNodeShader);
    insert_op(Type::DEPTHWISE_CONVOLUTION, NewDepthwiseConvolutionNodeShader);
    insert_op(Type::DEPTH_TO_SPACE, NewDepthToSpaceNodeShader);
    insert_op(Type::FULLY_CONNECTED, NewFullyConnectedNodeShader);
    insert_op(Type::LSTM, NewLstmNodeShader);
    insert_op(Type::MEAN, NewMeanNodeShader);
    // TODO(b/162763635): implement MeanStddevNormalization for OpenGL.
    insert_op(Type::MUL, NewMultiplyNodeShader);
    insert_op(Type::PAD, NewPadNodeShader);
    insert_op(Type::POOLING_2D, NewPoolingNodeShader);
    insert_op(Type::PRELU, NewPReLUNodeShader);
    insert_op(Type::QUANTIZE_AND_DEQUANTIZE,
              NewQuantizeAndDequantizeNodeShader);
    insert_op(Type::RELU, NewReLUNodeShader);
    insert_op(Type::RESAMPLER, NewResamplerNodeShader);
    insert_op(Type::RESIZE, NewResizeNodeShader);
    insert_op(Type::RESHAPE, NewReshapeNodeShader);
    insert_op(Type::SLICE, NewSliceNodeShader);
    insert_op(Type::SOFTMAX, NewSoftmaxNodeShader);
    insert_op(Type::SPACE_TO_DEPTH, NewSpaceToDepthNodeShader);
    insert_op(Type::TILE, NewTileNodeShader);

    insert_elementwise_op(Type::ABS);
    insert_elementwise_op(Type::COPY);
    insert_elementwise_op(Type::COS);
    insert_elementwise_op(Type::DIV);
    insert_elementwise_op(Type::ELU);
    insert_elementwise_op(Type::EXP);
    insert_elementwise_op(Type::FLOOR);
    insert_elementwise_op(Type::FLOOR_DIV);
    insert_elementwise_op(Type::FLOOR_MOD);
    insert_elementwise_op(Type::GELU);
    insert_elementwise_op(Type::HARD_SWISH);
    insert_elementwise_op(Type::LOG);
    insert_elementwise_op(Type::NEG);
    insert_elementwise_op(Type::MAXIMUM);
    insert_elementwise_op(Type::MINIMUM);
    insert_elementwise_op(Type::POW);
    insert_elementwise_op(Type::RSQRT);
    insert_elementwise_op(Type::SIGMOID);
    insert_elementwise_op(Type::SIN);
    insert_elementwise_op(Type::SQRT);
    insert_elementwise_op(Type::SQUARE);
    insert_elementwise_op(Type::SQUARED_DIFF);
    insert_elementwise_op(Type::SUB);
    insert_elementwise_op(Type::TANH);

#ifndef TFLITE_GPU_BINARY_RELEASE
    insert_op(Type::MAX_UNPOOLING_2D, NewMaxUnpoolingNodeShader);
    RegisterCustomOps(&shaders_);
#endif  // TFLITE_GPU_BINARY_RELEASE
  }

  ~Registry() final = default;

  absl::Status GenerateCode(const GenerationContext& ctx,
                            GeneratedCode* generated_code) const final {
    auto it = shaders_.find(ctx.op_type);
    if (it == shaders_.end()) {
      return absl::NotFoundError(
          absl::StrCat("No shader implementation for ", ctx.op_type));
    }
    std::vector<std::string> errors;
    for (const auto& shader : it->second) {
      const auto status = shader->GenerateCode(ctx, generated_code);
      // Return the first suitable shader.
      if (status.ok()) return absl::OkStatus();
      errors.push_back(std::string(status.message()));
    }
    return errors.empty() ? absl::OkStatus()
                          : absl::UnknownError(absl::StrJoin(errors, ", "));
  }

 private:
  absl::flat_hash_map<std::string, std::vector<std::unique_ptr<NodeShader>>>
      shaders_;
};

}  // namespace

std::unique_ptr<NodeShader> NewNodeShaderRegistry() {
  return std::make_unique<Registry>();
}

}  // namespace gl
}  // namespace gpu
}  // namespace tflite
