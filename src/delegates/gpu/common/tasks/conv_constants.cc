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

#include "delegates/gpu/common/tasks/conv_constants.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>

#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"

namespace tflite {
namespace gpu {

namespace {
// Adreno can provide up to ~3-4KB of constant memory, but in some cases even
// 3KB can have very bad performance.
int GetAdrenoOptimalMaxConstantSize(const AdrenoInfo& adreno_info) {
  if (adreno_info.IsAdreno3xx() || adreno_info.IsAdreno4xx() ||
      adreno_info.IsAdreno5xx()) {
    return 256 * 10;  // 2.5KB
  } else {
    return 256 * 14;  // 3.5KB
  }
}

int GetOptimalMaxConstantSize(const GpuInfo& gpu_info) {
  if (gpu_info.IsAdreno()) {
    return GetAdrenoOptimalMaxConstantSize(gpu_info.adreno_info);
  } else if (gpu_info.IsAMD()) {
    return 4096;
  } else {
    return 1024;  // 1KB
  }
}

void AppendConditionally(const std::string& value, const std::string& delimeter,
                         std::string* result) {
  if (!result->empty()) {
    *result += delimeter;
  }
  *result += value;
}

// src_size and dst_size must be <= 4;
std::string GenerateConv(int src_size, int dst_size, bool use_dot_conv,
                         int const_mem_offset, CalculationsPrecision precision,
                         const std::string& dst, const std::string& src) {
  std::string result;
  const std::string postfixes[] = {".x", ".y", ".z", ".w"};
  if (use_dot_conv) {
    const std::string src_postfixes[] = {".x", ".xy", ".xyz", ""};
    const std::string src_postfix = src_postfixes[src_size - 1];
    for (int i = 0; i < dst_size; ++i) {
      result += "      " + dst + postfixes[i] + " += dot(" + src +
                ", args.weights.Read(" + std::to_string(const_mem_offset + i) +
                ")" + src_postfix + ");\n";
    }
  } else {
    const std::string dst_postfixes[] = {".x", ".xy", ".xyz", ""};
    const std::string dst_postfix = dst_postfixes[dst_size - 1];
    if (precision == CalculationsPrecision::F32_F16) {
      for (int i = 0; i < src_size; ++i) {
        if (i != 0) {
          result += " + ";
        }
        std::string src_name = src;
        if (src_size != 1) {
          src_name += postfixes[i];
        }
        result += src_name + " * args.weights.Read(" +
                  std::to_string(const_mem_offset + i) + ")" + dst_postfix;
      }
      std::string size = dst_size == 1 ? "" : std::to_string(dst_size);
      result = "      " + dst + dst_postfix + " += TO_ACCUM_FLT" + size + "(" +
               result + ");\n";
    } else {
      for (int i = 0; i < src_size; ++i) {
        std::string src_name = src;
        if (src_size != 1) {
          src_name += postfixes[i];
        }
        result += "      " + dst + dst_postfix + " += " + src_name +
                  " * args.weights.Read(" +
                  std::to_string(const_mem_offset + i) + ")" + dst_postfix +
                  ";\n";
      }
    }
  }
  return result;
}

std::string GenerateConvolutionConstantCode(const GpuInfo& gpu_info,
                                            const OperationDef& op_def,
                                            const OHWI& weights_shape,
                                            bool x_oob_reads, bool y_oob_reads,
                                            bool use_dot_conv,
                                            GPUOperation* op) {
  auto src_desc = op_def.src_tensors[0];
  op->AddSrcTensor("src_tensor", src_desc);
  op->AddDstTensor("dst_tensor", op_def.dst_tensors[0]);

  const int out_z = DivideRoundUp(weights_shape.o, 4);
  const std::string kOutZ = std::to_string(out_z);
  const int src_depth = DivideRoundUp(weights_shape.i, 4);

  const std::string postfixes[] = {".x", ".xy", ".xyz", ""};

  std::string c;
  c += "MAIN_FUNCTION($0) {\n";
  if (src_desc.HasAxis(Axis::BATCH)) {
    c += "  int linear_id = GLOBAL_ID_0;\n";
    c += "  int X = linear_id / args.dst_tensor.Batch();\n";
    c += "  int B = linear_id % args.dst_tensor.Batch();\n";
    c += "  args.src_tensor.SetBatchRef(B);\n";
    c += "  args.dst_tensor.SetBatchRef(B);\n";
  } else {
    c += "  int X = GLOBAL_ID_0;\n";
  }
  c += "  int Y = GLOBAL_ID_1;\n";
  c += "  if (X >= args.dst_tensor.Width() || Y >= args.dst_tensor.Height()) "
       "return;\n";
  c += "  int start_x = X * args.stride_x + args.padding_x;\n";
  c += "  int start_y = Y * args.stride_y + args.padding_y;\n";
  for (int i = 0; i < out_z; ++i) {
    c += "  ACCUM_FLT4 r" + std::to_string(i) + " = INIT_ACCUM_FLT4(0.0f);\n";
  }
  std::string check;
  if (y_oob_reads && !src_desc.SupportsZeroClamp(Axis::HEIGHT, gpu_info)) {
    AppendConditionally("inside_y", " && ", &check);
  }
  if (x_oob_reads && !src_desc.SupportsZeroClamp(Axis::WIDTH, gpu_info)) {
    AppendConditionally("inside_x", " && ", &check);
  }
  int filters_counter = 0;
  for (int s = 0; s < src_depth; ++s) {
    const int src_ch_count = std::min(4, weights_shape.i - s * 4);
    const std::string s_count =
        src_ch_count == 1 ? "" : std::to_string(src_ch_count);
    const std::string s_type = absl::StrCat("FLT", s_count);
    const std::string s_postfix = postfixes[src_ch_count - 1];
    for (int ky = 0; ky < weights_shape.h; ++ky) {
      std::string s_y = absl::StrCat("(start_y + ", ky, " * args.dilation_y)");
      c += "  {\n";
      c += "    int y_c = start_y + " + std::to_string(ky) +
           " * args.dilation_y;\n";
      if (y_oob_reads && !src_desc.SupportsZeroClamp(Axis::HEIGHT, gpu_info)) {
        c +=
            "    bool inside_y = y_c >= 0 && y_c < args.src_tensor.Height();\n";
        c += "    y_c = clamp(y_c, 0, args.src_tensor.Height() - 1);\n";
      }
      for (int kx = 0; kx < weights_shape.w; ++kx) {
        c += "    {\n";
        c += "      int x_c = start_x + " + std::to_string(kx) +
             " * args.dilation_x;\n";
        if (x_oob_reads && !src_desc.SupportsZeroClamp(Axis::WIDTH, gpu_info)) {
          c += "      bool inside_x = x_c >= 0 && x_c < "
               "args.src_tensor.Width();\n";
          c += "      x_c = clamp(x_c, 0, args.src_tensor.Width() - 1);\n";
        }
        c += "      " + s_type + " src = args.src_tensor.Read(x_c, y_c, " +
             std::to_string(s) + ")" + s_postfix + ";\n";
        if (!check.empty()) {
          c += "      src *= INIT_FLT(" + check + ");\n";
        }
        for (int d = 0; d < out_z; ++d) {
          const int dst_ch_count = std::min(4, weights_shape.o - d * 4);
          c += GenerateConv(src_ch_count, dst_ch_count, use_dot_conv,
                            filters_counter, op_def.precision,
                            "r" + std::to_string(d), "src");
          filters_counter += use_dot_conv ? dst_ch_count : src_ch_count;
        }
        c += "    }\n";
      }
      c += "  }\n";
    }
  }
  for (int i = 0; i < out_z; ++i) {
    std::string s_i = std::to_string(i);
    c += "  {\n";
    c += "    FLT4 res = TO_FLT4(r" + s_i + ") + args.biases.Read(" + s_i +
         ");\n";
    c += "    args.dst_tensor.Write(res, X, Y, " + s_i + ");\n";
    c += "  }\n";
  }
  c += "}\n";
  return c;
}

bool IsDotConvBetter(int src_channels, int dst_channels) {
  if (dst_channels % 4 == 0) {
    return false;
  }

  // dst_channels % 4 != 0
  if (src_channels % 4 == 0) {
    return true;
  }

  // dst_channels % 4 != 0 && src_channels % 4 != 0
  const int src_depth = DivideRoundUp(src_channels, 4);
  const int dst_depth = DivideRoundUp(dst_channels, 4);
  return dst_channels * src_depth < src_channels * dst_depth;
}

}  // namespace

bool IsConvConstantsSupported(const GpuInfo& gpu_info,
                              const OperationDef& definition,
                              const Convolution2DAttributes& attr) {
  if (gpu_info.IsApiOpenCl() && gpu_info.IsAdreno()) {
    const std::string kBadDriver =
        "OpenCL 2.0 QUALCOMM build: commit #7ff4f54 changeid #I4460aa6217 "
        "Date: 12/30/18";
    if (absl::StrContains(gpu_info.opencl_info.platform_version, kBadDriver)) {
      return false;
    }
  }

  if (attr.groups != 1) {
    return false;
  }

  const bool use_dot_conv =
      IsDotConvBetter(attr.weights.shape.i, attr.weights.shape.o);
  const auto& w_shape = attr.weights.shape;
  const int src_depth = DivideRoundUp(w_shape.i, 4);
  const int dst_depth = DivideRoundUp(w_shape.o, 4);
  const int aligned_ch_count =
      use_dot_conv ? w_shape.o * src_depth * 4 : w_shape.i * dst_depth * 4;
  const int filters_count = aligned_ch_count * w_shape.h * w_shape.w;
  const int float_size = definition.precision == CalculationsPrecision::F32
                             ? sizeof(float)
                             : sizeof(half);
  const int filters_buffer_size = filters_count * float_size;
  const int kConstantMaxSize = GetOptimalMaxConstantSize(gpu_info);
  const int flt4_registers = DivideRoundUp(w_shape.o, 4);
  return filters_buffer_size <= kConstantMaxSize && flt4_registers <= 8;
}

GPUOperation CreateConvConstants(const GpuInfo& gpu_info,
                                 const OperationDef& definition,
                                 const Convolution2DAttributes& attr) {
  const bool use_dot_conv =
      IsDotConvBetter(attr.weights.shape.i, attr.weights.shape.o);
  GPUOperation op(definition);
  UploadWeightsForConvConstants(attr.weights, gpu_info, definition.precision,
                                use_dot_conv, &op);
  op.args_.AddInt("stride_x", attr.strides.w);
  op.args_.AddInt("stride_y", attr.strides.h);
  op.args_.AddInt("padding_x", -attr.padding.prepended.w);
  op.args_.AddInt("padding_y", -attr.padding.prepended.h);
  op.args_.AddInt("dilation_x", attr.dilations.w);
  op.args_.AddInt("dilation_y", attr.dilations.h);
  op.tensor_to_grid_ = TensorToGrid::kWBToX_HDToY_ZIs1;

  bool x_oob_reads =
      attr.padding.appended.w != 0 || attr.padding.prepended.w != 0;
  bool y_oob_reads =
      attr.padding.appended.h != 0 || attr.padding.prepended.h != 0;
  op.code_ = GenerateConvolutionConstantCode(gpu_info, definition,
                                             attr.weights.shape, x_oob_reads,
                                             y_oob_reads, use_dot_conv, &op);
  if (definition.precision == CalculationsPrecision::F16 &&
      gpu_info.IsAdreno() && gpu_info.adreno_info.IsAdreno3xx()) {
    op.compiler_options_.push_back(CompilerOptions::kAdrenoFullSimd);
  }
  if (definition.precision != CalculationsPrecision::F32 &&
      gpu_info.IsPowerVR()) {
    // BUG, some PowerVRs (GE8320) produce incorrect result without it
    op.compiler_options_.push_back(CompilerOptions::kClDisableOptimizations);
  }

  TensorDescriptor bias_tensor_desc = CreateConstantLinearTensorDescriptor(
      gpu_info, definition.src_tensors[0].GetDataType(), attr.bias);
  op.args_.AddObject("biases", std::make_unique<TensorDescriptor>(
                                   std::move(bias_tensor_desc)));
  return op;
}

}  // namespace gpu
}  // namespace tflite
