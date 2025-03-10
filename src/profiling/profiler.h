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
#ifndef TENSORFLOW_LITE_PROFILING_PROFILER_H_
#define TENSORFLOW_LITE_PROFILING_PROFILER_H_

#include "profiling/buffered_profiler.h"
#include "profiling/noop_profiler.h"

namespace tflite {
namespace profiling {

// TODO(b/131688504): Remove this and use runtime flags for profiler selection.
#ifdef TFLITE_PROFILING_ENABLED
using Profiler = BufferedProfiler;
#else
using Profiler = NoopProfiler;
#endif  // TFLITE_PROFILING_ENABLED

}  // namespace profiling
}  // namespace tflite

#define SCOPED_TAGGED_OPERATOR_PROFILE TFLITE_SCOPED_TAGGED_OPERATOR_PROFILE

#endif  // TENSORFLOW_LITE_PROFILING_PROFILER_H_
