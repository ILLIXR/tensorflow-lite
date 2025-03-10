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

#include <cstdint>
#include <functional>
#include <memory>
#include <random>

#include <gtest/gtest.h>
#include "delegates/xnnpack/reduce_tester.h"
#include "delegates/xnnpack/xnnpack_delegate.h"

namespace tflite {
namespace xnnpack {

TEST(Sum, DISABLED_4DReduceBatchSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({0})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_4DReduceBatchKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({0})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_4DReduceHeightSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({1})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_4DReduceHeightKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({1})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, 4DReduceWidthSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({2})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, 4DReduceWidthKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({2})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, 4DReduceHeightWidthSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({1, 2})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({2, 1})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, 4DReduceHeightWidthKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({1, 2})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({2, 1})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_4DReduceChannelsSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({3})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_4DReduceChannelsKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({3})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceBatchSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({0})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceBatchKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({0})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceWidthSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({1})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceWidthKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({1})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceChannelsSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({2})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_3DReduceChannelsKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, width, channels})
      .Axes({2})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_2DReduceBatchSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, channels})
      .Axes({0})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_2DReduceBatchKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, channels})
      .Axes({0})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_2DReduceChannelsSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, channels})
      .Axes({1})
      .KeepDims(false)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_2DReduceChannelsKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, channels})
      .Axes({1})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_1DSqueezeDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();

  ReduceTester().InputShape({batch}).Axes({0}).KeepDims(false).Test(
      BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, DISABLED_1DKeepDims) {
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(nullptr),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();

  ReduceTester().InputShape({batch}).Axes({0}).KeepDims(true).Test(
      BuiltinOperator_SUM, xnnpack_delegate.get());
}

TEST(Sum, MultiThreading) {
  TfLiteXNNPackDelegateOptions delegate_options =
      TfLiteXNNPackDelegateOptionsDefault();
  delegate_options.num_threads = 2;
  std::unique_ptr<TfLiteDelegate, decltype(&TfLiteXNNPackDelegateDelete)>
      xnnpack_delegate(TfLiteXNNPackDelegateCreate(&delegate_options),
                       TfLiteXNNPackDelegateDelete);

  std::random_device random_device;
  auto rng = std::mt19937(random_device());
  auto shape_rng =
      std::bind(std::uniform_int_distribution<int32_t>(2, 5), std::ref(rng));
  const auto batch = shape_rng();
  const auto height = shape_rng();
  const auto width = shape_rng();
  const auto channels = shape_rng();

  ReduceTester()
      .InputShape({batch, height, width, channels})
      .Axes({1, 2})
      .KeepDims(true)
      .Test(BuiltinOperator_SUM, xnnpack_delegate.get());
}

}  // namespace xnnpack
}  // namespace tflite
