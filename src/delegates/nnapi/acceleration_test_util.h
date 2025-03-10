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
#ifndef TENSORFLOW_LITE_DELEGATES_NNAPI_ACCELERATION_TEST_UTIL_H_
#define TENSORFLOW_LITE_DELEGATES_NNAPI_ACCELERATION_TEST_UTIL_H_

#include <optional>
#include <string>

#include <gtest/gtest.h>
#include "absl/types/optional.h"
#include "delegates/nnapi/nnapi_delegate_kernel.h"

namespace tflite {

// NNAPI specific configuration for the validation allowlist.
class NnapiAccelerationTestParams {
 public:
  static const char* AccelerationTestConfig() {
    return acceleration_test_config_;
  }

  static NnapiAccelerationTestParams ParseConfigurationLine(
      const std::string& conf_line);

  explicit NnapiAccelerationTestParams(int min_android_sdk_version)
      : min_android_sdk_version_{min_android_sdk_version} {};

  NnapiAccelerationTestParams()
      : min_android_sdk_version_{delegate::nnapi::kMinSdkVersionForNNAPI} {};

  // Minimum SDK version to apply the acceleration validation to.
  int MinAndroidSdkVersion() { return min_android_sdk_version_; }

 private:
  // Content in nnapi_acceleration_test_list.cc.
  static const char* const acceleration_test_config_;
  int min_android_sdk_version_;
};

// Returns the NNAPI acceleration test configuration for the given test id.
std::optional<NnapiAccelerationTestParams> GetNnapiAccelerationTestParam(
    std::string test_id);

}  // namespace tflite

#endif  // TENSORFLOW_LITE_DELEGATES_NNAPI_ACCELERATION_TEST_UTIL_H_
