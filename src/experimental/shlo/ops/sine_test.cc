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

#include "experimental/shlo/ops/sine.h"

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
struct ParamName<SineOp> {
  static std::string Get() { return "Sine"; }
};

namespace {

struct Sine {
  template <class T>
  T operator()(T v) const {
    return std::sin(v);
  }
} sine_ref;

template <>
F16 Sine::operator()<F16>(F16 val) const {
  return F16(operator()(static_cast<float>(val)));
}

template <>
BF16 Sine::operator()<BF16>(BF16 val) const {
  return BF16(operator()(static_cast<float>(val)));
}

INSTANTIATE_TYPED_TEST_SUITE_P(Sine, UnaryElementwiseOpShapePropagationTest,
                               SineOp, TestParamNames);

INSTANTIATE_TYPED_TEST_SUITE_P(
    Sine, UnaryElementwiseSameBaselineElementTypeConstraintTest,
    UnaryElementwiseConstraint1Types<SineOp>, TestParamNames);

using UnsupportedTypes = WithOpTypes<
    SineOp, ConcatTypes<BoolTestType, IntTestTypes, PerAxisQuantizedTestTypes>>;

INSTANTIATE_TYPED_TEST_SUITE_P(Sine, UnaryElementwiseUnsupportedTypeTest,
                               UnsupportedTypes, TestParamNames);

template <class T>
struct FloatSineTest : ::testing::Test {};

TYPED_TEST_SUITE(FloatSineTest, FloatTestTypes, TestParamNames);

TYPED_TEST(FloatSineTest, FloatTensorsWork) {
  using StorageT = typename TypeParam::StorageT;

  const Shape shape({2, 3, 4});
  Vector<StorageT> input_data = RandomBuffer<TypeParam::kStorage>(shape);
  Vector<StorageT> output_data(shape.NumElements());
  const TensorType tensor_type =
      TensorType{.shape = shape, .element_type = TypeParam::kStorage};
  Tensor input_tensor{.type = tensor_type, .data = input_data.data()};
  Tensor output_tensor{.type = tensor_type, .data = output_data.data()};

  Vector<StorageT> expected_data(shape.NumElements());
  absl::c_transform(input_data, expected_data.begin(), sine_ref);

  auto op = Create(SineOp::Attributes{});
  ASSERT_OK(Prepare(op, input_tensor, output_tensor));
  ASSERT_OK(Evaluate(op, input_tensor, output_tensor));
  EXPECT_THAT(output_data, Pointwise(NanSensitiveFloatEq(), expected_data));
}

template <class T>
struct QuantizedSineTest : ::testing::Test {};

TYPED_TEST_SUITE(QuantizedSineTest, QuantizedTestTypes, TestParamNames);

TYPED_TEST(QuantizedSineTest, PerTensorWorks) {
  using StorageT = typename TypeParam::StorageT;
  using ExpressedT = typename TypeParam::ExpressedT;

  const Shape shape({2, 3, 4});
  const ExpressedT scale = static_cast<ExpressedT>(1.5);
  const StorageT zero_point = static_cast<StorageT>(5);
  Vector<StorageT> input_data = RandomBuffer<TypeParam::kStorage>(shape);
  Vector<StorageT> output_data(shape.NumElements());
  const QuantizedPerTensorTensorType tensor_type = {
      .shape = shape,
      .element_type = QuantizedElementTypePerTensor(
          TypeParam::kStorage, zero_point, TypeParam::kExpressed, scale)};
  Tensor input_tensor{.type = tensor_type, .data = input_data.data()};
  Tensor output_tensor{.type = tensor_type, .data = output_data.data()};

  Vector<StorageT> expected_data(shape.NumElements());
  absl::c_transform(
      input_data, expected_data.begin(), [zero_point, scale](auto v) {
        const ExpressedT dequantized_input = Dequantize(v, zero_point, scale);
        const ExpressedT dequantized_res = sine_ref(dequantized_input);
        return Quantize<TypeParam::kStorage, TypeParam::kExpressed>(
            dequantized_res, zero_point, static_cast<ExpressedT>(1.) / scale);
      });

  auto op = Create(SineOp::Attributes{});
  ASSERT_OK(Prepare(op, input_tensor, output_tensor));
  ASSERT_OK(Evaluate(op, input_tensor, output_tensor));
  EXPECT_THAT(output_data, ElementsAreArray(expected_data));
}

}  // namespace
}  // namespace shlo_ref
