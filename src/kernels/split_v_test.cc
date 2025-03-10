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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "flatbuffers/flatbuffers.h"  // from @flatbuffers
#include "kernels/test_util.h"
#include "schema/schema_generated.h"

namespace tflite {
namespace {

using ::testing::ElementsAreArray;

constexpr int kAxisIsATensor = -1000;

enum class TestType {
  kDynamic = 0,      // Both split_sizes and axis are dynamic
  kConstAxis = 1,    // split_sizes is dynamic and axis is constant
  kConstSplits = 2,  // Both split_sizes and axis are constant
};

class SplitVOpModel : public SingleOpModel {
 public:
  SplitVOpModel(const TensorData& input, const TensorData& size_splits,
                int num_splits, int axis,
                std::initializer_list<int> size_splits_data) {
    input_ = AddInput(input);
    if (size_splits_data.size() == 0) {
      size_splits_ = AddInput(size_splits);
    } else {
      size_splits_ = AddConstInput(size_splits, size_splits_data);
    }
    if (axis == kAxisIsATensor) {
      axis_ = AddInput({TensorType_INT32, {1}});
    } else {
      axis_ = AddConstInput(TensorType_INT32, {axis}, {1});
    }
    for (int i = 0; i < num_splits; ++i) {
      outputs_.push_back(AddOutput(input.type));
    }
    SetBuiltinOp(BuiltinOperator_SPLIT_V, BuiltinOptions_SplitVOptions,
                 CreateSplitVOptions(builder_, num_splits).Union());
    if (axis == kAxisIsATensor) {
      BuildInterpreter(
          {GetShape(input_), GetShape(size_splits_), GetShape(axis_)});
    } else {
      BuildInterpreter({GetShape(input_), GetShape(size_splits_), {}});
    }
  }

  template <typename T>
  void SetInput(std::initializer_list<T> data) {
    PopulateTensor<T>(input_, data);
  }
  void SetSizeSplits(std::initializer_list<int> data) {
    PopulateTensor(size_splits_, data);
  }
  void SetAxis(int axis) { PopulateTensor(axis_, {axis}); }

  template <typename T>
  std::vector<T> GetOutput(int i) {
    return ExtractVector<T>(outputs_[i]);
  }
  std::vector<int> GetOutputShape(int i) { return GetTensorShape(outputs_[i]); }

 private:
  int input_;
  int size_splits_;
  int axis_;
  std::vector<int> outputs_;
};

template <typename T>
void Check(TestType test_type, int axis, std::initializer_list<int> input_shape,
           std::initializer_list<int> size_splits_shape,
           std::vector<std::initializer_list<int>> output_shapes,
           const std::initializer_list<T>& input_data,
           const std::initializer_list<int>& size_splits_data,
           const std::vector<std::initializer_list<T>>& output_data) {
  int num_splits = size_splits_data.size();

  switch (test_type) {
    case TestType::kDynamic: {
      SplitVOpModel m({GetTensorType<T>(), input_shape},
                      {TensorType_INT32, size_splits_shape}, num_splits,
                      kAxisIsATensor, {/*size_splits is a tensor*/});
      m.SetInput<T>(input_data);
      m.SetSizeSplits(size_splits_data);
      m.SetAxis(axis);
      ASSERT_EQ(m.Invoke(), kTfLiteOk);
      for (int i = 0; i < num_splits; ++i) {
        EXPECT_THAT(m.GetOutput<T>(i), ElementsAreArray(output_data[i]));
        EXPECT_THAT(m.GetOutputShape(i), ElementsAreArray(output_shapes[i]));
      }
    } break;
    case TestType::kConstAxis: {
      SplitVOpModel m({GetTensorType<T>(), input_shape},
                      {TensorType_INT32, size_splits_shape}, num_splits, axis,
                      {/*size_splits is a tensor*/});
      m.SetInput<T>(input_data);
      m.SetSizeSplits(size_splits_data);
      ASSERT_EQ(m.Invoke(), kTfLiteOk);
      for (int i = 0; i < num_splits; ++i) {
        EXPECT_THAT(m.GetOutput<T>(i), ElementsAreArray(output_data[i]));
        EXPECT_THAT(m.GetOutputShape(i), ElementsAreArray(output_shapes[i]));
      }
    } break;
    case TestType::kConstSplits: {
      SplitVOpModel m({GetTensorType<T>(), input_shape},
                      {TensorType_INT32, size_splits_shape}, num_splits, axis,
                      size_splits_data);
      m.SetInput<T>(input_data);
      ASSERT_EQ(m.Invoke(), kTfLiteOk);
      for (int i = 0; i < num_splits; ++i) {
        EXPECT_THAT(m.GetOutputShape(i), ElementsAreArray(output_shapes[i]));
        if (output_data[i].size() != 0) {
          EXPECT_THAT(m.GetOutput<T>(i), ElementsAreArray(output_data[i]));
        }
      }
    } break;
  }
}

template <typename T>
class SplitVOpTypedTest : public ::testing::Test {};

using DataTypes = ::testing::Types<float, uint8_t, int8_t, int16_t, int32_t>;
TYPED_TEST_SUITE(SplitVOpTypedTest, DataTypes);

#define TYPED_SPLIT_V_TEST(TestSuiteName, CaseName)                    \
  template <typename TypeParam>                                        \
  void Check##TestSuiteName##CaseName(TestType test_type);             \
                                                                       \
  TYPED_TEST(TestSuiteName, Dynamic##CaseName) {                       \
    Check##TestSuiteName##CaseName<TypeParam>(TestType::kDynamic);     \
  }                                                                    \
  TYPED_TEST(TestSuiteName, ConstAxis##CaseName) {                     \
    Check##TestSuiteName##CaseName<TypeParam>(TestType::kConstAxis);   \
  }                                                                    \
  TYPED_TEST(TestSuiteName, ConstSplits##CaseName) {                   \
    Check##TestSuiteName##CaseName<TypeParam>(TestType::kConstSplits); \
  }                                                                    \
                                                                       \
  template <typename TypeParam>                                        \
  void Check##TestSuiteName##CaseName(TestType test_type)

TYPED_SPLIT_V_TEST(SplitVOpTypedTest, TwoDimensional) {
  // Input shape: {4, 3}
  // size_splits: {1, 1, 2}
  // axis: 0
  // We should have 3 outpus with shapes respectively:
  //  output 1 : {1, 3}
  //  output 2 : {1, 3}
  //  output 3 : {2, 3}
  Check<TypeParam>(test_type,
                   /*axis=*/0, {4, 3}, {3}, {{1, 3}, {1, 3}, {2, 3}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12}, {1, 1, 2},
                   {{1, 2, 3}, {4, 5, 6}, {7, 8, 9, 10, 11, 12}});
}

TYPED_SPLIT_V_TEST(SplitVOpTypedTest, FourDimensional) {
  Check<TypeParam>(test_type,
                   /*axis=*/0, {2, 2, 2, 2}, {2}, {{1, 2, 2, 2}, {1, 2, 2, 2}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   {1, 1},
                   {
                       {1, 2, 3, 4, 5, 6, 7, 8},
                       {9, 10, 11, 12, 13, 14, 15, 16},
                   });
  Check<TypeParam>(test_type,
                   /*axis=*/1, {2, 2, 2, 2}, {2}, {{2, 1, 2, 2}, {2, 1, 2, 2}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   {1, -1},
                   {
                       {1, 2, 3, 4, 9, 10, 11, 12},
                       {5, 6, 7, 8, 13, 14, 15, 16},
                   });
  Check<TypeParam>(test_type,
                   /*axis=*/2, {2, 2, 2, 2}, {2}, {{2, 2, 1, 2}, {2, 2, 1, 2}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   {1, 1},
                   {
                       {1, 2, 5, 6, 9, 10, 13, 14},
                       {3, 4, 7, 8, 11, 12, 15, 16},
                   });
  Check<TypeParam>(test_type,
                   /*axis=*/3, {2, 2, 2, 2}, {2}, {{2, 2, 2, 1}, {2, 2, 2, 1}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   {1, 1},
                   {
                       {1, 3, 5, 7, 9, 11, 13, 15},
                       {2, 4, 6, 8, 10, 12, 14, 16},
                   });
}

TYPED_SPLIT_V_TEST(SplitVOpTypedTest, OneDimensional) {
  Check<TypeParam>(test_type,
                   /*axis=*/0, {8}, {8},
                   {{1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}},
                   {1, 2, 3, 4, 5, 6, 7, 8}, {1, 1, 1, 1, 1, 1, 1, 1},
                   {{1}, {2}, {3}, {4}, {5}, {6}, {7}, {8}});
}

TYPED_SPLIT_V_TEST(SplitVOpTypedTest, OneDimensional2) {
  Check<TypeParam>(test_type,
                   /*axis=*/0, {8}, {8},
                   {{1}, {1}, {1}, {1}, {1}, {1}, {2}, {0}},
                   {1, 2, 3, 4, 5, 6, 7, 8}, {1, 1, 1, 1, 1, 1, 2, -1},
                   {{1}, {2}, {3}, {4}, {5}, {6}, {7, 8}, {}});
}

TYPED_SPLIT_V_TEST(SplitVOpTypedTest, NegativeAxis) {
  Check<TypeParam>(test_type,
                   /*axis=*/-4, {2, 2, 2, 2}, {2}, {{1, 2, 2, 2}, {1, 2, 2, 2}},
                   {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16},
                   {1, 1},
                   {
                       {1, 2, 3, 4, 5, 6, 7, 8},
                       {9, 10, 11, 12, 13, 14, 15, 16},
                   });
}

}  // namespace
}  // namespace tflite
