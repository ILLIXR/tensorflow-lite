/* Copyright 2024 The TensorFlow Authors. All Rights Reserved.

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

#include "experimental/shlo/ops/logistic.h"

#include <cmath>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "experimental/shlo/bf16.h"
#include "experimental/shlo/f16.h"
#include "experimental/shlo/ops/test_util.h"
#include "experimental/shlo/ops/unary_elementwise_test_util.h"
#include "experimental/shlo/quantize.h"
#include "experimental/shlo/quantized_tensor_element_type.h"
#include "experimental/shlo/shape.h"
#include "experimental/shlo/status_matcher.h"
#include "experimental/shlo/tensor.h"

using testing::ElementsAreArray;
using testing::NanSensitiveFloatEq;
using testing::Pointwise;

namespace shlo_ref {

template <>
struct ParamName<LogisticOp> {
  static std::string Get() { return "Logistic"; }
};

namespace {

struct Logistic {
  template <class T>
  T operator()(T v) const {
    constexpr T one = static_cast<T>(1);
    return one / (one + std::exp(-v));
  }
} logistic_ref;

template <>
F16 Logistic::operator()(F16 v) const {
  return F16(operator()(static_cast<float>(v)));
}

template <>
BF16 Logistic::operator()(BF16 v) const {
  return BF16(operator()(static_cast<float>(v)));
}

INSTANTIATE_TYPED_TEST_SUITE_P(Logistic, UnaryElementwiseOpShapePropagationTest,
                               LogisticOp, TestParamNames);

INSTANTIATE_TYPED_TEST_SUITE_P(
    Logistic, UnaryElementwiseSameBaselineElementTypeConstraintTest,
    UnaryElementwiseConstraint1Types<LogisticOp>, TestParamNames);

using UnsupportedTypes =
    WithOpTypes<LogisticOp, ConcatTypes<BoolTestType, IntTestTypes,
                                        PerAxisQuantizedTestTypes>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Logistic, UnaryElementwiseUnsupportedTypeTest,
                               UnsupportedTypes, TestParamNames);

template <class T>
struct LogisticTest : ::testing::Test {};

TYPED_TEST_SUITE(LogisticTest, FloatTestTypes, TestParamNames);

TYPED_TEST(LogisticTest, FloatTensorsWork) {
  using StorageT = typename TypeParam::StorageT;

  const Shape shape({2, 3, 4});
  Vector<StorageT> input_data = RandomBuffer<TypeParam::kStorage>(shape);
  Vector<StorageT> output_data(shape.NumElements());

  Tensor input_tensor{
      .type = TensorType{.shape = shape, .element_type = TypeParam::kStorage},
      .data = input_data.data()};
  Tensor output_tensor{
      .type = TensorType{.shape = shape, .element_type = TypeParam::kStorage},
      .data = output_data.data()};

  Vector<StorageT> expected_data(shape.NumElements());
  absl::c_transform(input_data, expected_data.begin(), logistic_ref);

  auto op = Create(LogisticOp::Attributes{});
  ASSERT_OK(Prepare(op, input_tensor, output_tensor));
  ASSERT_OK(Evaluate(op, input_tensor, output_tensor));
  EXPECT_THAT(output_data, Pointwise(NanSensitiveFloatEq(), expected_data));
}

template <class T>
struct QuantizedLogisticTest : ::testing::Test {};

TYPED_TEST_SUITE(QuantizedLogisticTest, QuantizedTestTypes, TestParamNames);

TYPED_TEST(QuantizedLogisticTest, PerTensorWorks) {
  using StorageT = typename TypeParam::StorageT;
  using ExpressedT = typename TypeParam::ExpressedT;

  const Shape shape({2, 3, 4});
  Vector<StorageT> input_data = RandomBuffer<TypeParam::kStorage>(shape);
  Vector<StorageT> output_data(shape.NumElements());
  const ExpressedT scale = static_cast<ExpressedT>(1.5);
  const StorageT zero_point = static_cast<StorageT>(5);
  const QuantizedElementTypePerTensor tensor_type =
      QuantizedElementTypePerTensor(TypeParam::kStorage, zero_point,
                                    TypeParam::kExpressed, scale);
  Tensor input_tensor{
      .type = QuantizedPerTensorTensorType{.shape = shape,
                                           .element_type = tensor_type},
      .data = input_data.data()};
  Tensor output_tensor{
      .type = QuantizedPerTensorTensorType{.shape = shape,
                                           .element_type = tensor_type},
      .data = output_data.data()};

  Vector<StorageT> expected_data(shape.NumElements());
  absl::c_transform(
      input_data, expected_data.begin(), [zero_point, scale](auto v) {
        const ExpressedT dequantized_input = Dequantize(v, zero_point, scale);
        const ExpressedT dequantized_res = logistic_ref(dequantized_input);
        return Quantize<TypeParam::kStorage, TypeParam::kExpressed>(
            dequantized_res, zero_point, static_cast<ExpressedT>(1.) / scale);
      });

  auto op = Create(LogisticOp::Attributes{});
  ASSERT_OK(Prepare(op, input_tensor, output_tensor));
  ASSERT_OK(Evaluate(op, input_tensor, output_tensor));
  EXPECT_THAT(output_data, ElementsAreArray(expected_data));
}

}  // namespace
}  // namespace shlo_ref
