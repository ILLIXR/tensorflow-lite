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
#include "delegates/utils/utils.h"

#include <algorithm>
#include <memory>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "array.h"
#include "minimal_logging.h"

namespace tflite::delegates::utils {

TfLiteStatus ConvertToTfLiteStatus(absl::Status status) {
  if (!status.ok()) {
    TFLITE_LOG_PROD(
        TFLITE_LOG_ERROR, "%s",
        status.ToString(absl::StatusToStringMode::kWithEverything).c_str());
    return kTfLiteError;
  }
  return kTfLiteOk;
}

}  // namespace tflite::delegates::utils
