/* Copyright 2023 The TensorFlow Authors. All Rights Reserved.

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

#include "kernels/internal/opaque_tensor_ctypes.h"

#include "c/c_api_opaque.h"
#include "kernels/internal/runtime_shape.h"
#include "namespace.h"

namespace tflite {
namespace TFLITE_CONDITIONAL_NAMESPACE {

RuntimeShape GetTensorShape(const TfLiteOpaqueTensor* tensor) {
  if (tensor == nullptr) {
    return RuntimeShape();
  }
  const int dims_size = TfLiteOpaqueTensorNumDims(tensor);
  RuntimeShape shape(dims_size);
  for (int i = 0; i < dims_size; ++i) {
    shape.SetDim(i, TfLiteOpaqueTensorDim(tensor, i));
  }
  return shape;
}

}  // namespace TFLITE_CONDITIONAL_NAMESPACE
}  // namespace tflite
