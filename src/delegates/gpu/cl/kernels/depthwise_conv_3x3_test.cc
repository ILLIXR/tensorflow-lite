/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

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

#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "delegates/gpu/cl/kernels/cl_test.h"
#include "delegates/gpu/common/operations.h"
#include "delegates/gpu/common/status.h"
#include "delegates/gpu/common/tasks/depthwise_conv_3x3_stride_h2_test_util.h"
#include "delegates/gpu/common/tasks/depthwise_conv_3x3_test_util.h"

namespace tflite {
namespace gpu {
namespace cl {
namespace {

TEST_F(OpenCLOperationTest, DepthwiseConv3x3SimpleWeights) {
  auto status = DepthwiseConv3x3SimpleWeightsTest(&exec_env_);
  ASSERT_TRUE(status.ok()) << status.message();
}

TEST_F(OpenCLOperationTest, DepthwiseConv3x3) {
  auto status = DepthwiseConv3x3Test(&exec_env_);
  ASSERT_TRUE(status.ok()) << status.message();
}

TEST_F(OpenCLOperationTest, DepthWiseConv3x3StrideH2SimpleWeights) {
  auto status = DepthWiseConv3x3StrideH2SimpleWeightsTest(&exec_env_);
  ASSERT_TRUE(status.ok()) << status.message();
}

}  // namespace
}  // namespace cl
}  // namespace gpu
}  // namespace tflite
