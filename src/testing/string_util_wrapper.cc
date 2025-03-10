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

#include "pybind11/pybind11.h"  // from @pybind11
#include "pybind11/pytypes.h"  // from @pybind11
#include "python/interpreter_wrapper/numpy.h"
#include "testing/string_util.h"
#include "tensorflow/python/lib/core/pybind11_lib.h"

namespace py = pybind11;

PYBIND11_MODULE(_pywrap_string_util, m) {
  tflite::python::ImportNumpy();

  m.doc() = R"pbdoc(
    _pywrap_string_util
    -----
  )pbdoc";
  m.def(
      "SerializeAsHexString",
      [](py::handle& string_tensor) {
        return tensorflow::PyoOrThrow(
            tflite::testing::python::SerializeAsHexString(string_tensor.ptr()));
      },
      R"pbdoc(
      Serializes TF Lite dynamic buffer format as a HexString.
    )pbdoc");
}
