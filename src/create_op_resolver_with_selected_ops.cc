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

#include <memory>

#include "core/create_op_resolver.h"
#include "mutable_op_resolver.h"

// This method is generated by `gen_selected_ops`.
extern void RegisterSelectedOps(::tflite::MutableOpResolver* resolver);

namespace tflite {
// This interface is the unified entry point for creating op resolver
// regardless if selective registration is being used. C++ client will call
// this method directly and Java client will call this method indirectly via
// JNI code in interpreter_jni.cc.
std::unique_ptr<MutableOpResolver> CreateOpResolver() {
  std::unique_ptr<MutableOpResolver> resolver =
      std::make_unique<MutableOpResolver>();
  RegisterSelectedOps(resolver.get());
  return resolver;
}

}  // namespace tflite
