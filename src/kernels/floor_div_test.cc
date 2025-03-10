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
#include <stdint.h>

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "kernels/test_util.h"
#include "schema/schema_generated.h"

namespace tflite {
namespace {

using ::testing::ElementsAre;

template <typename T>
class FloorDivModel : public SingleOpModel {
 public:
  FloorDivModel(const TensorData& input1, const TensorData& input2,
                const TensorData& output) {
    input1_ = AddInput(input1);
    input2_ = AddInput(input2);
    output_ = AddOutput(output);
    SetBuiltinOp(BuiltinOperator_FLOOR_DIV, BuiltinOptions_FloorDivOptions,
                 CreateFloorDivOptions(builder_).Union());
    BuildInterpreter({GetShape(input1_), GetShape(input2_)});
  }

  int input1() { return input1_; }
  int input2() { return input2_; }

  std::vector<T> GetOutput() { return ExtractVector<T>(output_); }
  std::vector<int> GetOutputShape() { return GetTensorShape(output_); }

 private:
  int input1_;
  int input2_;
  int output_;
};

TEST(FloorDivModel, Simple) {
  FloorDivModel<int32_t> model({TensorType_INT32, {1, 2, 2, 1}},
                               {TensorType_INT32, {1, 2, 2, 1}},
                               {TensorType_INT32, {}});
  model.PopulateTensor<int32_t>(model.input1(), {10, 9, 11, 3});
  model.PopulateTensor<int32_t>(model.input2(), {2, 2, 3, 4});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(5, 4, 3, 0));
}

TEST(FloorDivModel, NegativeValue) {
  FloorDivModel<int32_t> model({TensorType_INT32, {1, 2, 2, 1}},
                               {TensorType_INT32, {1, 2, 2, 1}},
                               {TensorType_INT32, {}});
  model.PopulateTensor<int32_t>(model.input1(), {10, -9, -11, 7});
  model.PopulateTensor<int32_t>(model.input2(), {2, 2, -3, -4});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(5, -5, 3, -2));
}

TEST(FloorDivModel, BroadcastFloorDiv) {
  FloorDivModel<int32_t> model({TensorType_INT32, {1, 2, 2, 1}},
                               {TensorType_INT32, {1}}, {TensorType_INT32, {}});
  model.PopulateTensor<int32_t>(model.input1(), {10, -9, -11, 7});
  model.PopulateTensor<int32_t>(model.input2(), {-3});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(-4, 3, 3, -3));
}

TEST(FloorDivModel, SimpleFloat) {
  FloorDivModel<float> model({TensorType_FLOAT32, {1, 2, 2, 1}},
                             {TensorType_FLOAT32, {1, 2, 2, 1}},
                             {TensorType_FLOAT32, {}});
  model.PopulateTensor<float>(model.input1(), {10.05, 9.09, 11.9, 3.01});
  model.PopulateTensor<float>(model.input2(), {2.05, 2.03, 3.03, 4.03});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(4.0, 4.0, 3.0, 0.0));
}

TEST(FloorDivModel, NegativeValueFloat) {
  FloorDivModel<float> model({TensorType_FLOAT32, {1, 2, 2, 1}},
                             {TensorType_FLOAT32, {1, 2, 2, 1}},
                             {TensorType_FLOAT32, {}});
  model.PopulateTensor<float>(model.input1(), {10.03, -9.9, -11.0, 7.0});
  model.PopulateTensor<float>(model.input2(), {2.0, 2.3, -3.0, -4.1});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(5.0, -5.0, 3.0, -2.0));
}

TEST(FloorDivModel, BroadcastFloorDivFloat) {
  FloorDivModel<float> model({TensorType_FLOAT32, {1, 2, 2, 1}},
                             {TensorType_FLOAT32, {1}},
                             {TensorType_FLOAT32, {}});
  model.PopulateTensor<float>(model.input1(), {10.03, -9.9, -11.0, 7.0});
  model.PopulateTensor<float>(model.input2(), {-3.3});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(-4.0, 2.0, 3.0, -3.0));
}

TEST(FloorDivModel, SimpleInt16) {
  FloorDivModel<int16_t> model({TensorType_INT16, {1, 2, 2, 1}},
                               {TensorType_INT16, {1, 2, 2, 1}},
                               {TensorType_INT16, {}});
  model.PopulateTensor<int16_t>(model.input1(), {10, 9, 11, 3});
  model.PopulateTensor<int16_t>(model.input2(), {2, 2, 3, 4});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(5, 4, 3, 0));
}

TEST(FloorDivModel, NegativeValueInt16) {
  FloorDivModel<int16_t> model({TensorType_INT16, {1, 2, 2, 1}},
                               {TensorType_INT16, {1, 2, 2, 1}},
                               {TensorType_INT16, {}});
  model.PopulateTensor<int16_t>(model.input1(), {10, -9, -11, 7});
  model.PopulateTensor<int16_t>(model.input2(), {2, 2, -3, -4});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(5, -5, 3, -2));
}

TEST(FloorDivModel, BroadcastFloorDivInt16) {
  FloorDivModel<int16_t> model({TensorType_INT16, {1, 2, 2, 1}},
                               {TensorType_INT16, {1}}, {TensorType_INT16, {}});
  model.PopulateTensor<int16_t>(model.input1(), {10, -9, -11, 7});
  model.PopulateTensor<int16_t>(model.input2(), {-3});
  ASSERT_EQ(model.Invoke(), kTfLiteOk);
  EXPECT_THAT(model.GetOutputShape(), ElementsAre(1, 2, 2, 1));
  EXPECT_THAT(model.GetOutput(), ElementsAre(-4, 3, 3, -3));
}
}  // namespace
}  // namespace tflite
