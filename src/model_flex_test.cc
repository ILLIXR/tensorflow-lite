/* Copyright 2018 The TensorFlow Authors. All Rights Reserved.

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

#include <gtest/gtest.h>
#include "core/interpreter.h"
#include "core/interpreter_builder.h"
#include "core/kernels/register.h"
#include "core/model_builder.h"
#include "testing/util.h"

namespace tflite {

// Ensures that a model with TensorFlow ops can be imported as long as the
// appropriate delegate is linked into the client.
TEST(FlexModel, WithFlexDelegate) {
  auto model = FlatBufferModel::BuildFromFile(
      "testdata/multi_add_flex.bin");
  ASSERT_TRUE(model);

  std::unique_ptr<Interpreter> interpreter;
  ASSERT_EQ(InterpreterBuilder(*model,
                               ops::builtin::BuiltinOpResolver{})(&interpreter),
            kTfLiteOk);
  ASSERT_TRUE(interpreter);

  ASSERT_EQ(interpreter->AllocateTensors(), kTfLiteOk);
}

}  // namespace tflite
