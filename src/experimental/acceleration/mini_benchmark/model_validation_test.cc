/* Copyright 2021 The TensorFlow Authors. All Rights Reserved.

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
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <fstream>
#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "flatbuffers/flatbuffers.h"  // from @flatbuffers
#include "acceleration/configuration/configuration_generated.h"
#include "experimental/acceleration/compatibility/android_info.h"
#include "experimental/acceleration/mini_benchmark/big_little_affinity.h"
#include "experimental/acceleration/mini_benchmark/validator.h"
#include "tools/model_loader.h"
#ifdef ENABLE_NNAPI_SL_TEST
#include "nnapi/sl/include/SupportLibrary.h"
#endif /* ENABLE_NNAPI_SL_TEST */

extern const unsigned char TENSORFLOW_ACCELERATION_MODEL_DATA_VARIABLE[];
extern const int TENSORFLOW_ACCELERATION_MODEL_LENGTH_VARIABLE;

namespace tflite {
namespace acceleration {
namespace {

class LocalizerValidationRegressionTest : public ::testing::Test {
 protected:
  static void SetUpTestSuite() {
    int error = -1;
#if defined(__ANDROID__)
    AndroidInfo android_info;
    auto status = RequestAndroidInfo(&android_info);
    ASSERT_TRUE(status.ok());
    if (android_info.is_emulator) {
      std::cerr << "Running on an emulator, skipping processor affinity\n";
    } else {
      std::cerr << "Running on hardware, setting processor affinity\n";
      BigLittleAffinity affinity = GetAffinity();
      cpu_set_t set;
      CPU_ZERO(&set);
      for (int i = 0; i < 16; i++) {
        if (affinity.big_core_affinity & (0x1 << i)) {
          CPU_SET(i, &set);
        }
      }
      error = sched_setaffinity(getpid(), sizeof(set), &set);
      if (error == -1) {
        perror("sched_setaffinity failed");
      }
    }
#endif
    std::string dir = GetTestTmpDir();
    error = mkdir(dir.c_str(), 0777);
    if (error == -1) {
      if (errno != EEXIST) {
        perror("mkdir failed");
        ASSERT_TRUE(false);
      }
    }

    std::string path = ModelPath();
    (void)unlink(path.c_str());
    std::string contents(reinterpret_cast<const char*>(
                             TENSORFLOW_ACCELERATION_MODEL_DATA_VARIABLE),
                         TENSORFLOW_ACCELERATION_MODEL_LENGTH_VARIABLE);
    std::ofstream f(path, std::ios::binary);
    ASSERT_TRUE(f.is_open());
    f << contents;
    f.close();
    ASSERT_EQ(chmod(path.c_str(), 0500), 0);
  }
  static std::string ModelPath() { return GetTestTmpDir() + "/model.tflite"; }
  static std::string GetTestTmpDir() {
    const char* from_env = getenv("TEST_TMPDIR");
    if (from_env) {
      return from_env;
    }
#ifdef __ANDROID__
    return "/data/local/tmp";
#else
    return "/tmp";
#endif
  }
  void CheckValidation(const std::string& accelerator_name) {
    std::string path = ModelPath();
    const ComputeSettings* settings =
        flatbuffers::GetRoot<ComputeSettings>(fbb_.GetBufferPointer());
    int fd = open(path.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    struct stat stat_buf = {0};
    ASSERT_EQ(fstat(fd, &stat_buf), 0);
    auto validator =
        std::make_unique<Validator>(std::make_unique<tools::MmapModelLoader>(
                                        fd, /*offset=*/0, stat_buf.st_size),
                                    settings);
    close(fd);

    Validator::Results results;
    Validator::Status validation_run = validator->RunValidation(&results);
    EXPECT_EQ(validation_run.status, kMinibenchmarkSuccess);
    EXPECT_EQ(validation_run.stage, BenchmarkStage_UNKNOWN);
    EXPECT_TRUE(results.ok);
    EXPECT_EQ(results.delegate_error, 0);
    if (accelerator_name != "CPU") {
      // For any non-CPU delegate, we validate that model execution was at least
      // partially delegated by expecting non-zero number of delegated kernels
      EXPECT_NE(results.delegated_kernels, 0);
    }

    for (const auto& metric : results.metrics) {
      int test_case = 0;
      std::cerr << "Metric " << metric.first;
      for (float v : metric.second) {
        std::cerr << " " << v;
        RecordProperty("[" + std::to_string(test_case++) + "] " +
                           accelerator_name + " " + metric.first,
                       std::to_string(v));
      }
      std::cerr << "\n";
    }
    std::cerr << "Delegate prep time us " << results.delegate_prep_time_us
              << std::endl;
    RecordProperty(accelerator_name + " Delegate prep time us",
                   results.delegate_prep_time_us);
    std::cerr << "Execution time us";
    int test_case = 0;
    int64_t total_execution_time_us = 0;
    for (int64_t t : results.execution_time_us) {
      std::cerr << " " << t;
      RecordProperty("[" + std::to_string(test_case++) + "] " +
                         accelerator_name + " Execution time us",
                     t);
      total_execution_time_us += t;
    }
    std::cerr << "\n";
    int64_t average_execution_time_us = total_execution_time_us / test_case;
    std::cerr << "Avg execution time us " << average_execution_time_us
              << std::endl;
    RecordProperty(accelerator_name + " Avg execution time us",
                   average_execution_time_us);
    std::cerr << std::endl;
  }
  flatbuffers::FlatBufferBuilder fbb_;
};

TEST_F(LocalizerValidationRegressionTest, Cpu) {
  fbb_.Finish(CreateComputeSettings(fbb_, ExecutionPreference_ANY,
                                    CreateTFLiteSettings(fbb_)));
  CheckValidation("CPU");
}

#ifndef DISABLE_PLATFORM_NNAPI_TEST
TEST_F(LocalizerValidationRegressionTest, Nnapi) {
  fbb_.Finish(
      CreateComputeSettings(fbb_, ExecutionPreference_ANY,
                            CreateTFLiteSettings(fbb_, Delegate_NNAPI)));
  AndroidInfo android_info;
  auto status = RequestAndroidInfo(&android_info);
  ASSERT_TRUE(status.ok());
  if (android_info.android_sdk_version >= "28") {
    CheckValidation("NNAPI");
  }
}
#endif  // DISABLE_PLATFORM_NNAPI_TEST

#ifdef ENABLE_NNAPI_SL_TEST
TEST_F(LocalizerValidationRegressionTest, NnapiSl) {
  const char* accelerator_name = getenv("TEST_ACCELERATOR_NAME");

  auto nnapi_sl_handle = nnapi::loadNnApiSupportLibrary(
      GetTestTmpDir() + "/libnnapi_sl_driver.so");

  ASSERT_NE(nnapi_sl_handle.get(), nullptr);
  int res;
  uint32_t count;
  res = nnapi_sl_handle->getFL5()->ANeuralNetworks_getDeviceCount(&count);
  ASSERT_EQ(res, ANEURALNETWORKS_NO_ERROR);
  ASSERT_GE(count, 1);

  fbb_.Finish(CreateComputeSettings(
      fbb_, ExecutionPreference_ANY,
      CreateTFLiteSettings(
          fbb_, Delegate_NNAPI,
          CreateNNAPISettings(
              fbb_, fbb_.CreateString(accelerator_name ? accelerator_name : ""),
              /* cache_directory */ 0,
              /* model_token */ 0, NNAPIExecutionPreference_UNDEFINED,
              /* no_of_nnapi_instances_to_cache */ 0,
              /* fallback_settings */ 0,
              /* allow_nnapi_cpu_on_android_10_plus */ false,
              /* execution_priority */
              NNAPIExecutionPriority_NNAPI_PRIORITY_UNDEFINED,
              /* allow_dynamic_dimensions */ false,
              /* allow_fp16_precision_for_fp32 */ true,
              /* use_burst_computation */ false,
              reinterpret_cast<uint64_t>(nnapi_sl_handle->getFL5())),
          /* gpu_settings */ 0,
          /* hexagon_settings */ 0,
          /* xnnpack_settings */ 0,
          /* coreml_settings */ 0,
          /* cpu_settings */ 0,
          /* max_delegated_partitions */ 1)));
  AndroidInfo android_info;
  auto status = RequestAndroidInfo(&android_info);
  ASSERT_TRUE(status.ok());
  if (android_info.android_sdk_version >= "30") {
    CheckValidation("NNAPISL");
  }
}
#endif  // ENABLE_NNAPI_SL_TEST

TEST_F(LocalizerValidationRegressionTest, GpuAny) {
  AndroidInfo android_info;
  auto status = RequestAndroidInfo(&android_info);
  ASSERT_TRUE(status.ok());
  if (android_info.is_emulator) {
    std::cerr << "Skipping GPU on emulator\n";
    return;
  }
  fbb_.Finish(CreateComputeSettings(fbb_, ExecutionPreference_ANY,
                                    CreateTFLiteSettings(fbb_, Delegate_GPU)));
#ifdef __ANDROID__
  CheckValidation("GPUANY");
#endif  // __ANDROID__
}

TEST_F(LocalizerValidationRegressionTest, GpuOpenGL) {
  AndroidInfo android_info;
  auto status = RequestAndroidInfo(&android_info);
  ASSERT_TRUE(status.ok());
  if (android_info.is_emulator) {
    std::cerr << "Skipping GPU on emulator\n";
    return;
  }
  fbb_.Finish(CreateComputeSettings(
      fbb_, ExecutionPreference_ANY,
      CreateTFLiteSettings(
          fbb_, Delegate_GPU, 0,
          CreateGPUSettings(fbb_, /* allow_precision_loss */ false,
                            /* allow_quantized_inference */ true,
                            GPUBackend_OPENGL))));
#ifdef __ANDROID__
  CheckValidation("GPUOPENGL");
#endif  // __ANDROID__
}

TEST_F(LocalizerValidationRegressionTest, GpuOpenCL) {
  AndroidInfo android_info;
  auto status = RequestAndroidInfo(&android_info);
  ASSERT_TRUE(status.ok());
  if (android_info.is_emulator) {
    std::cerr << "Skipping GPU on emulator\n";
    return;
  }
  fbb_.Finish(CreateComputeSettings(
      fbb_, ExecutionPreference_ANY,
      CreateTFLiteSettings(
          fbb_, Delegate_GPU, 0,
          CreateGPUSettings(fbb_, /* allow_precision_loss */ false,
                            /* allow_quantized_inference */ true,
                            GPUBackend_OPENCL))));
#ifdef __ANDROID__
  CheckValidation("GPUOPENCL");
#endif  // __ANDROID__
}

}  // namespace
}  // namespace acceleration
}  // namespace tflite
