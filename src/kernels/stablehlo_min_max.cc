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

#include "core/c/common.h"
#include "kernels/stablehlo_elementwise.h"

namespace tflite::ops::builtin {

TfLiteRegistration* Register_STABLEHLO_MAXIMUM() {
  static TfLiteRegistration r = {nullptr, nullptr, ElementwisePrepare,
                                 ElementwiseEval<ComputationType::kMax>};
  return &r;
}
TfLiteRegistration* Register_STABLEHLO_MINIMUM() {
  static TfLiteRegistration r = {nullptr, nullptr, ElementwisePrepare,
                                 ElementwiseEval<ComputationType::kMin>};
  return &r;
}
}  // namespace tflite::ops::builtin
