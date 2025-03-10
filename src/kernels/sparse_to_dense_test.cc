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

#include <initializer_list>
#include <vector>

#include <gtest/gtest.h>
#include "kernels/test_util.h"
#include "schema/schema_generated.h"

namespace tflite {
namespace {

using ::testing::ElementsAreArray;

enum class TestType {
  kPersistentRo = 0,
  kConstant = 1,
  kDynamic = 2,
};

template <typename T>
class SparseToDenseOpModel : public SingleOpModel {
 public:
  SparseToDenseOpModel(std::initializer_list<int> indices_shape,
                       std::initializer_list<int> output_shape_shape,
                       std::initializer_list<int> values_shape, T default_value,
                       TensorType tensor_index_type,
                       TensorType tensor_input_type,
                       std::initializer_list<int> output_shape_data,
                       TestType test_type)
      : test_type_(test_type) {
    indices_ = AddInput(tensor_index_type);
    output_shape_ = test_type == TestType::kConstant
                        ? AddConstInput(TensorType_INT32, output_shape_data,
                                        output_shape_shape)
                        : AddInput(TensorType_INT32);
    values_ = AddInput(tensor_input_type);
    default_value_ = AddInput(tensor_input_type);
    output_ = AddOutput(tensor_input_type);

    SetBuiltinOp(BuiltinOperator_SPARSE_TO_DENSE,
                 BuiltinOptions_SparseToDenseOptions,
                 CreateSparseToDenseOptions(builder_, false).Union());
    BuildInterpreter({indices_shape, output_shape_shape, values_shape, {1}},
                     /*num_threads=*/-1,
                     /*allow_fp32_relax_to_fp16=*/false,
                     /*apply_delegate=*/true, /*allocate_and_delegate=*/false);
    if (test_type == TestType::kPersistentRo) {
      interpreter_->tensor(output_shape_)->allocation_type =
          kTfLitePersistentRo;
      interpreter_->ResizeInputTensorStrict(output_shape_, output_shape_shape);
      PopulateTensor<int32_t>(output_shape_, output_shape_data);
    }
    AllocateAndDelegate(true);
    PopulateTensor<T>(default_value_, {default_value});
  }

  int indices() { return indices_; }
  int output_shape() { return output_shape_; }
  int values() { return values_; }

  bool IsDynamicOutput() {
    const TfLiteTensor* tensor = interpreter_->tensor(output_);
    return tensor->allocation_type == kTfLiteDynamic;
  }

  std::vector<T> GetOutput() { return ExtractVector<T>(output_); }
  std::vector<int> GetOutputShape() { return GetTensorShape(output_); }

 private:
  int indices_;
  int output_shape_;
  int values_;
  int default_value_;
  int output_;
  TestType test_type_;
};

class SparseToDenseOpModelTest : public ::testing::TestWithParam<TestType> {};

TEST_P(SparseToDenseOpModelTest, ZeroDimensionTest) {
  SparseToDenseOpModel<float> m({1}, {1}, {1}, 0, TensorType_INT32,
                                TensorType_FLOAT32, {5}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {3});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {5});
  }
  m.PopulateTensor<float>(m.values(), {7});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({0, 0, 0, 7, 0}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({5}));
}

TEST_P(SparseToDenseOpModelTest, OneDimensionTest) {
  SparseToDenseOpModel<float> m({3}, {1}, {3}, 0, TensorType_INT32,
                                TensorType_FLOAT32, {7}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {1, 3, 5});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {7});
  }
  m.PopulateTensor<float>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(m.GetOutput(), ElementsAreArray({0, 2, 0, 4, 0, 6, 0}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({7}));
}

TEST_P(SparseToDenseOpModelTest, TwoDimensionsTest) {
  SparseToDenseOpModel<float> m({3, 3}, {3}, {3}, 0, TensorType_INT32,
                                TensorType_FLOAT32, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<float>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(m.GetOutput(),
              ElementsAreArray({2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 4, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, Int64IndexTest) {
  SparseToDenseOpModel<float> m({3, 3}, {3}, {3}, -1, TensorType_INT64,
                                TensorType_FLOAT32, {3, 3, 3}, GetParam());
  m.PopulateTensor<int64_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<float>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(
      m.GetOutput(),
      ElementsAreArray({2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 4,  -1, -1, 6,  -1, -1, -1, -1, -1, -1, -1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, DefaultValueTest) {
  SparseToDenseOpModel<float> m({3, 3}, {3}, {3}, -1, TensorType_INT32,
                                TensorType_FLOAT32, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<float>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(
      m.GetOutput(),
      ElementsAreArray({2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 4,  -1, -1, 6,  -1, -1, -1, -1, -1, -1, -1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, Int32ValueTest) {
  SparseToDenseOpModel<int32_t> m({3, 3}, {3}, {3}, -1, TensorType_INT32,
                                  TensorType_INT32, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<int32_t>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(
      m.GetOutput(),
      ElementsAreArray({2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 4,  -1, -1, 6,  -1, -1, -1, -1, -1, -1, -1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, Int64ValueTest) {
  SparseToDenseOpModel<int64_t> m({3, 3}, {3}, {3}, -1, TensorType_INT32,
                                  TensorType_INT64, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<int64_t>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(
      m.GetOutput(),
      ElementsAreArray({2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 4,  -1, -1, 6,  -1, -1, -1, -1, -1, -1, -1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, Int8ValueTest) {
  SparseToDenseOpModel<int8_t> m({3, 3}, {3}, {3}, -1, TensorType_INT32,
                                 TensorType_INT8, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<int8_t>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(
      m.GetOutput(),
      ElementsAreArray({2,  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
                        -1, -1, 4,  -1, -1, 6,  -1, -1, -1, -1, -1, -1, -1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

TEST_P(SparseToDenseOpModelTest, UInt8ValueTest) {
  SparseToDenseOpModel<uint8_t> m({3, 3}, {3}, {3}, 1, TensorType_INT32,
                                  TensorType_UINT8, {3, 3, 3}, GetParam());
  m.PopulateTensor<int32_t>(m.indices(), {0, 0, 0, 1, 2, 1, 2, 0, 1});
  if (GetParam() != TestType::kConstant) {
    m.PopulateTensor<int32_t>(m.output_shape(), {3, 3, 3});
  }
  m.PopulateTensor<uint8_t>(m.values(), {2, 4, 6});
  ASSERT_EQ(m.Invoke(), kTfLiteOk);
  ASSERT_EQ(m.IsDynamicOutput(), GetParam() == TestType::kDynamic);
  EXPECT_THAT(m.GetOutput(),
              ElementsAreArray({2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                1, 1, 4, 1, 1, 6, 1, 1, 1, 1, 1, 1, 1}));
  EXPECT_THAT(m.GetOutputShape(), ElementsAreArray({3, 3, 3}));
}

INSTANTIATE_TEST_SUITE_P(SparseToDenseOpModelTest, SparseToDenseOpModelTest,
                         ::testing::Values(TestType::kPersistentRo,
                                           TestType::kConstant,
                                           TestType::kDynamic));
}  // namespace
}  // namespace tflite
