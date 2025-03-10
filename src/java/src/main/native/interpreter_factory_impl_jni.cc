/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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

#include <jni.h>
#include <stdio.h>

#include "c/c_api.h"
#include "java/src/main/native/jni_utils.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

JNIEXPORT jstring JNICALL
Java_org_tensorflow_lite_InterpreterFactoryImpl_nativeRuntimeVersion(
    JNIEnv* env, jclass /*clazz*/) {
  if (!tflite::jni::CheckJniInitializedOrThrow(env)) return nullptr;

  return env->NewStringUTF(TfLiteVersion());
}

JNIEXPORT jstring JNICALL
Java_org_tensorflow_lite_InterpreterFactoryImpl_nativeSchemaVersion(
    JNIEnv* env, jclass /*clazz*/) {
  char buf[64];
  snprintf(buf, sizeof(buf), "%d", TfLiteSchemaVersion());
  return env->NewStringUTF(buf);
}

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus
