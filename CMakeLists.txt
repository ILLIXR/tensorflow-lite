#
# Copyright 2020 The TensorFlow Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


# Builds the Tensorflow Lite runtime.
#
# WARNING: This is an experimental that is subject to change.
# This has only been tested on Windows, Linux and macOS.
#
# The following are not currently supported:
# - iOS
# - Micro backend
# - Tests
# - Many features in experimental
# - Host Tools (i.e conversion / analysis tools etc.)

# This build has been modified to remove the building of third party code to another module
# for inclusion in ILLIXR. This was done to alleviate the issue of naming conflicts within
# components.

# 2024
# Modified for use with ILLIXR hand tracking plugin
# changed what components are built when ENABLE_GPU is set to ON to match Bazel build

cmake_minimum_required(VERSION 3.16)
if(NOT CMAKE_BUILD_TYPE)
    message(STATUS "Setting build type to Release, for debug builds use"
            "'-DCMAKE_BUILD_TYPE=Debug'.")
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Double colon in target name means ALIAS or IMPORTED target.
cmake_policy(SET CMP0028 NEW)
# Enable MACOSX_RPATH (@rpath) for built dynamic libraries.
cmake_policy(SET CMP0042 NEW)
project(tensorflow-lite C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(TENSORFLOW_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH
    "Directory that contains the TensorFlow project"
)
#set(TENSORFLOW_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "")
set(TSL_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xla/third_party/tsl")
set(XLA_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/xla/")
set(TFLITE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src")
set(CMAKE_MODULE_PATH
    ${CMAKE_INSTALL_PREFIX}/lib/cmake
    "${TFLITE_SOURCE_DIR}/tools/cmake/modules"
    ${CMAKE_MODULE_PATH}
)
set(CMAKE_PREFIX_PATH
    ${CMAKE_INSTALL_PREFIX}/lib/cmake
    "${TFLITE_SOURCE_DIR}/tools/cmake/modules"
    ${CMAKE_PREFIX_PATH}
)
include(GNUInstallDirs)
include(CMakeDependentOption)
option(TFLITE_ENABLE_INSTALL "Enable install rule" OFF)
option(TFLITE_ENABLE_LABEL_IMAGE "Enable label_image example" OFF)
option(TFLITE_ENABLE_RUY "Enable experimental RUY integration" OFF)
option(TFLITE_ENABLE_RESOURCE "Enable experimental support for resources" ON)
option(TFLITE_ENABLE_NNAPI "Enable NNAPI (Android only)." ON)
cmake_dependent_option(TFLITE_ENABLE_NNAPI_VERBOSE_VALIDATION "Enable NNAPI verbose validation." OFF
                       "TFLITE_ENABLE_NNAPI" ON)
option(TFLITE_ENABLE_MMAP "Enable MMAP (unsupported on Windows)" ON)
option(TFLITE_ENABLE_GPU "Enable GPU" OFF)
option(TFLITE_ENABLE_METAL "Enable Metal delegate (iOS only)" OFF)
option(TFLITE_ENABLE_XNNPACK "Enable XNNPACK backend" ON)
option(TFLITE_ENABLE_EXTERNAL_DELEGATE "Enable External Delegate backend" ON)
option(LIBRARY_POSTFIX "Postfix for the library" "")
if(LIBRARY_POSTFIX)
    if(NOT ((LIBRARY_POSTFIX MATCHES "^-") OR (LIBRARY_POSTFIX MATCHES "^_")))
        set(LIBRARY_POSTFIX "-${LIBRARY_POSTFIX}")
    endif()
else()
    set(LIBRARY_POSTFIX "")
endif()
option(TFLITE_KERNEL_TEST "Enable tflite kernel unit test" OFF)
option(BUILD_SHARED_LIBS "Build a shared library instead of a static one" OFF)
if(TFLITE_KERNEL_TEST AND ${CMAKE_CROSSCOMPILING})
    set(TFLITE_HOST_TOOLS_DIR "" CACHE PATH "Host tools directory")
    if(TFLITE_HOST_TOOLS_DIR STREQUAL "")
        message(FATAL_ERROR "Path to the natively compiled 'flatc' compiler binary has not been provided!\
                        Please specify it using -DTFLITE_HOST_TOOLS_DIR=<flatc_dir_path> launch argument.")
    endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(_TFLITE_ENABLE_RUY "${TFLITE_ENABLE_RUY}")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    set(_TFLITE_ENABLE_RUY ON)
endif()
set(_TFLITE_ENABLE_NNAPI "${TFLITE_ENABLE_NNAPI}")
#if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
#    set(_TFLITE_ENABLE_NNAPI OFF)
#endif()
set(_TFLITE_ENABLE_MMAP "${TFLITE_ENABLE_MMAP}")
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # See https://github.com/tensorflow/tensorflow/blob/\
    # 2b96f3662bd776e277f86997659e61046b56c315/tensorflow/lite/tools/make/\
    # Makefile#L157
    set(_TFLITE_ENABLE_MMAP OFF)
endif()
# Simplifies inclusion of non-test sources and headers from a directory.
# SOURCE_DIR: Directory to search for files.
# SOURCES_VAR: Variable to append with all matching *.cc and *.h files.
# [FILTER expression0 .. expressionN]:
#   Additional regular expressions to filter the set of matching
#   files. By default, all files ending in "(_test|test_util)\\.(cc|h)" are
#   removed.
# [RECURSE]: Whether to recursively search SOURCE_DIR.
macro(populate_source_vars SOURCE_DIR SOURCES_VAR)
    cmake_parse_arguments(ARGS "RECURSE" "" "FILTER" ${ARGN})
    if(ARGS_RECURSE)
        set(GLOB_OP GLOB_RECURSE)
    else()
        set(GLOB_OP GLOB)
    endif()
    set(DEFAULT_FILE_FILTER ".*(_test|test_util)\\.(c|cc|h)$")
    file(${GLOB_OP} FOUND_SOURCES "${SOURCE_DIR}/*.*")
    list(FILTER FOUND_SOURCES INCLUDE REGEX ".*\\.(c|cc|h)$")
    list(FILTER FOUND_SOURCES EXCLUDE REGEX "${DEFAULT_FILE_FILTER}")
    foreach(FILE_FILTER ${ARGS_FILTER})
        list(FILTER FOUND_SOURCES EXCLUDE REGEX "${FILE_FILTER}")
    endforeach()
    list(APPEND ${SOURCES_VAR} ${FOUND_SOURCES})
endmacro()
# Simplifies inclusion of non-test sources and headers from a directory
# relative to TFLITE_SOURCE_DIR. See populate_source_vars() for the
# description of arguments including and following SOURCES_VAR.
macro(populate_tflite_source_vars RELATIVE_DIR SOURCES_VAR)
    populate_source_vars(
            "${TFLITE_SOURCE_DIR}/${RELATIVE_DIR}" ${SOURCES_VAR} ${ARGN}
    )
endmacro()
# Make sure all repos have licenses.
set(OVERRIDABLE_FETCH_CONTENT_LICENSE_CHECK ON)
# Additional library dependencies based upon enabled features.
set(TFLITE_TARGET_DEPENDENCIES "")
# Find TensorFlow Lite dependencies.
find_package(absl REQUIRED)

find_package(Eigen3 REQUIRED CONFIG)
add_compile_definitions(EIGEN_MPL2_ONLY)

find_package(farmhash REQUIRED CONFIG)
find_package(fft2d REQUIRED CONFIG)
find_package(flatbuffers REQUIRED CONFIG)
find_package(gemmlowp REQUIRED CONFIG)
if (NOT CMAKE_SYSTEM_PROCESSOR OR CMAKE_SYSTEM_PROCESSOR MATCHES "x86")
    find_package(NEON_2_SSE REQUIRED CONFIG)
    list(APPEND TFLITE_TARGET_DEPENDENCIES NEON_2_SSE::NEON_2_SSE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(cpuinfo REQUIRED libcpuinfo)
include_directories(AFTER ${cpuinfo_INCLUDE_DIR}) #CPUINFO is used by XNNPACK and RUY library

find_package(ruy REQUIRED CONFIG)
# Include TSL, which is in tensorflow/third_party
include_directories(
        ${TSL_SOURCE_DIR}
        ${XLA_SOURCE_DIR}
)
find_package(pthreadpool REQUIRED CONFIG)

set(TF_TARGET_PRIVATE_OPTIONS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    # TensorFlow uses a heap of deprecated proto fields so surpress these
    # warnings until they're fixed.
    list(APPEND TF_TARGET_PRIVATE_OPTIONS "-Wno-deprecated-declarations")
endif()
# Additional compiler flags used when compiling TF Lite.
set(TFLITE_TARGET_PUBLIC_OPTIONS "-DEIGEN_NEON_GEBP_NR=4")
set(TFLITE_TARGET_PRIVATE_OPTIONS "")
set(TFLITE_TARGET_PRIVATE_DEFINITIONS "")
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang$")
    # TFLite uses deprecated methods in neon2sse which generates a huge number of
    # warnings so surpress these until they're fixed.
    list(APPEND TFLITE_TARGET_PRIVATE_OPTIONS "-Wno-deprecated-declarations")
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    # Use NOMINMAX to disable the min / max macros in windows.h as they break
    # use of std::min std::max.
    # Use NOGDI to ERROR macro which breaks TensorFlow logging.
    # Disable mmap, which is not available on Windows.
    list(APPEND TFLITE_TARGET_PRIVATE_OPTIONS "-DNOMINMAX" "-DNOGDI" "-DTFLITE_MMAP_DISABLED")
    # lite/kernels/conv.cc has more than 64k sections so enable /bigobj to
    # support compilation with MSVC2015.
    if(MSVC)
        list(APPEND TFLITE_TARGET_PRIVATE_OPTIONS "/bigobj")
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            message("Disabling MSVC /O2 optimization for Win32")
            set(CompFlags
                CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL
                CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL
                CMAKE_C_FLAGS_RELWITHDEBINFO
            )
            foreach (CompFlag ${CompFlags})
                string(REGEX REPLACE "(\/Ob. )" "" ${CompFlag} "${${CompFlag}}")
                string(REPLACE "/O2" "/O1" ${CompFlag} "${${CompFlag}}")
                list(REMOVE_DUPLICATES ${CompFlag})
                set(${CompFlag} "${${CompFlag}}" CACHE INTERNAL "")
            endforeach()
        endif()
        list(APPEND TFLITE_TARGET_PRIVATE_DEFINITIONS "_USE_MATH_DEFINES")
    elseif(CMAKE_COMPILER_IS_GNUCXX)
        list(APPEND TFLITE_TARGET_PRIVATE_OPTIONS "-Wa,-mbig-obj")
    endif()
endif()
if(CMAKE_SYSTEM_NAME MATCHES "Android")
    find_library(ANDROID_LOG_LIB log)
    list(APPEND TFLITE_TARGET_DEPENDENCIES
         log
    )
endif()
# Build a list of source files to compile into the TF Lite library.
populate_tflite_source_vars("." TFLITE_SRCS)
if((CMAKE_SYSTEM_NAME MATCHES "Windows" AND BUILD_SHARED_LIBS) OR CMAKE_BUILD_TYPE MATCHES "Reslease")
    list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*simple_memory_arena_debug_dump\\.cc$")
endif()

# This particular file is excluded because the more explicit approach to enable
# XNNPACK delegate is preferred to the weak-symbol one.
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*tflite_with_xnnpack\\.cc$")

# Exclude Flex related files.
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*with_selected_ops\\.cc$")

# Exclude tensorflow_profiler_logger files.
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*tensorflow_profiler_logger\\.cc$")

if(_TFLITE_ENABLE_MMAP)
    list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*mmap_allocation_disabled\\.cc$")
else()
    list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*mmap_allocation\\.cc$")
endif()

# Handle TFLite logging source.
list(FILTER TFLITE_SRCS EXCLUDE REGEX ".*minimal_logging_.*\\.cc$")
if("${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
    list(APPEND TFLITE_SRCS ${TFLITE_SOURCE_DIR}/minimal_logging_android.cc)
elseif("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS")
    list(APPEND TFLITE_SRCS ${TFLITE_SOURCE_DIR}/minimal_logging_ios.cc)
else()
    list(APPEND TFLITE_SRCS ${TFLITE_SOURCE_DIR}/minimal_logging_default.cc)
endif()

populate_tflite_source_vars("core" TFLITE_CORE_SRCS)
populate_tflite_source_vars(
        "core/acceleration/configuration" TFLITE_CORE_ACCELERATION_SRCS
        FILTER "xnnpack_plugin.*"
        FILTER "(_test)\\.(cc|h)$"
        FILTER "delegate_registry.*"
        FILTER "nnapi_plugin.*"
        FILTER "stable_delegate_registry.*"
)
populate_tflite_source_vars("core/api" TFLITE_CORE_API_SRCS)
populate_tflite_source_vars("core/async" TFLITE_CORE_ASYNC_SRCS)
populate_tflite_source_vars(
        "core/async/c" TFLITE_CORE_ASYNC_C_SRCS
        FILTER "async_kernel.*"
        FILTER "async_signature_runner.*"
        FILTER "task.*"
)
populate_tflite_source_vars("core/async/interop" TFLITE_CORE_ASYNC_INTEROP_SRCS)
populate_tflite_source_vars("core/async/interop/c" TFLITE_CORE_ASYNC_INTEROP_C_SRCS)
populate_tflite_source_vars("core/c" TFLITE_CORE_C_SRCS)
populate_tflite_source_vars("core/experimental/acceleration/configuration" TFLITE_CORE_EXPERIMENTAL_SRCS)
populate_tflite_source_vars("core/kernels" TFLITE_CORE_KERNELS_SRCS)
populate_tflite_source_vars("core/tools" TFLITE_CORE_TOOLS_SRCS)
populate_tflite_source_vars("c" TFLITE_C_SRCS)
populate_tflite_source_vars("delegates" TFLITE_DELEGATES_SRCS)

if(NOT TFLITE_KERNEL_TEST)
    list(FILTER TFLITE_C_SRCS EXCLUDE REGEX ".*c_api_for_testing\\.cc$")
endif()
if(TFLITE_ENABLE_GPU)
    set(LIBRARY_POSTFIX "${LIBRARY_POSTFIX}-gpu")
    find_package(PkgConfig REQUIRED)
    find_package(OpenCLHeaders REQUIRED CONFIG)
    pkg_check_modules(vulkan REQUIRED vulkan)
    find_package(fp16_headers REQUIRED)
    # Android NDK already has OpenGL, EGL headers.
    if(NOT "${CMAKE_SYSTEM_NAME}" STREQUAL "Android")
        pkg_check_modules(glesv2 REQUIRED glesv2)
        pkg_check_modules(egl REQUIRED egl)
    endif()

    add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/common_generated.h
            COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api
            -I ${TENSORFLOW_SOURCE_DIR}
            -o ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl
            -c ${TFLITE_SOURCE_DIR}/delegates/gpu/gl/common.fbs
    )
    add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/workgroups_generated.h
            COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api
            -I ${TENSORFLOW_SOURCE_DIR}
            -o ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl
            -c ${TFLITE_SOURCE_DIR}/delegates/gpu/gl/workgroups.fbs
    )
    add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/metadata_generated.h
            COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api
            -I ${TENSORFLOW_SOURCE_DIR}
            -o ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl
            -c ${TFLITE_SOURCE_DIR}/delegates/gpu/gl/metadata.fbs
    )
    add_custom_command(
            OUTPUT ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/compiled_model_generated.h
            COMMAND flatbuffers::flatc --scoped-enums
            -I ${TENSORFLOW_SOURCE_DIR}
            -o ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl
            -c ${TFLITE_SOURCE_DIR}/delegates/gpu/gl/compiled_model.fbs
    )

    add_custom_target(
            gpu_fbs
            DEPENDS ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/compiled_model_generated.h ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/common_generated.h  ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/metadata_generated.h ${CMAKE_BINARY_DIR}/src/delegates/gpu/gl/workgroups_generated.h
    )

    #populate_tflite_source_vars(
    #        "delegates/gpu/cl" TFLITE_DELEGATES_GPU_CL_SRCS
    #        FILTER "(_test|gl_interop|gpu_api_delegate|egl_sync)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/cl/default" TFLITE_DELEGATES_GPU_CL_DEFAULT_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/cl/kernels" TFLITE_DELEGATES_GPU_CL_KERNELS_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    populate_tflite_source_vars(
            "delegates/gpu/common/google" TFLITE_DELEGATES_GPU_COMMON_GOOGLE_SRC
            FILTER "(_test)\\.(cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/common/mediapipe" TFLITE_DELEGATES_GPU_COMMON_MEDIAPIPE_SRCS
            FILTER "(_test)\\.(cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/common/memory_management"
            TFLITE_DELEGATES_GPU_COMMON_MEMORY_MANAGEMENT_SRCS
            FILTER "(_test)\\.(cc|h)$"
    )
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/selectors" TFLITE_DELEGATES_GPU_COMMON_SELECTORS_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/selectors/default" TFLITE_DELEGATES_GPU_COMMON_SELECTORS_DEFAULT_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/selectors/mediapipe" TFLITE_DELEGATES_GPU_COMMON_SELECTORS_MEDIAPIPE_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    populate_tflite_source_vars(
            "delegates/gpu/common" TFLITE_DELEGATES_GPU_COMMON_SRCS
            FILTER "(_test)\\.(cc|h)$"
    )
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/task"
    #        TFLITE_DELEGATES_GPU_COMMON_TASK_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/tasks"
    #        TFLITE_DELEGATES_GPU_COMMON_TASKS_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/tasks/mediapipe"
    #        TFLITE_DELEGATES_GPU_COMMON_TASKS_MEDIAPIPE_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    #populate_tflite_source_vars(
    #        "delegates/gpu/common/tasks/special"
    #        TFLITE_DELEGATES_GPU_COMMON_TASKS_SPECIAL_SRCS
    #        FILTER "(_test)\\.(cc|h)$"
    #)
    populate_tflite_source_vars(
            "delegates/gpu/common/transformations"
            TFLITE_DELEGATES_GPU_COMMON_TRANSFORMATIONS_SRCS
            FILTER "(_test)\\.(cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl/kernels/mediapipe"
            TFLITE_DELEGATES_GPU_GL_KERNELS_MEDIAPIPE_SRC
            FILTER "(_test)\\(.cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl"
            TFLITE_DELEGATES_GPU_GL_SRC
            FILTER "(_test)//(.cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl/compiler"
            TFLITE_DELEGATES_GPU_GL_COMPILER_SRC
            FILTER "(_test)//(.cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl/converters"
            TFLITE_DELEGATES_GPU_GL_CONVERTERS_SRC
            FILTER "(_test)//(.cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl/kernels"
            TFLITE_DELEGATES_GPU_GL_KERNELS_SRC
            FILTER "(_test)//(.cc|h)$"
    )
    populate_tflite_source_vars(
            "delegates/gpu/gl/workgroups"
            TFLITE_DELEGATES_GPU_GL_WORKGROUPS_SRC
            FILTER "(_test)//(.cc|h)$"
    )

    list(APPEND TFLITE_DELEGATES_GPU_SRCS
         ${TFLITE_SOURCE_DIR}/delegates/gpu/api.cc
         ${TFLITE_SOURCE_DIR}/delegates/gpu/delegate.cc
         ${TFLITE_SOURCE_DIR}/delegates/gpu/delegate_options.cc
         ${TFLITE_SOURCE_DIR}/delegates/gpu/tflite_profile.cc
         ${TFLITE_SOURCE_DIR}/delegates/gpu/gl_delegate.cc
         #${TFLITE_SOURCE_DIR}/experimental/acceleration/compatibility/android_info.cc
         #${TFLITE_DELEGATES_GPU_CL_SRCS}
         #${TFLITE_DELEGATES_GPU_CL_DEFAULT_SRCS}
         #${TFLITE_DELEGATES_GPU_CL_KERNELS_SRCS}
         ${TFLITE_DELEGATES_GPU_COMMON_GOOGLE_SRC}
         ${TFLITE_DELEGATES_GPU_COMMON_MEDIAPIPE_SRCS}
         ${TFLITE_DELEGATES_GPU_COMMON_MEMORY_MANAGEMENT_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_SELECTORS_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_SELECTORS_DEFAULT_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_SELECTORS_MEDIAPIPE_SRCS}
         ${TFLITE_DELEGATES_GPU_COMMON_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_TASK_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_TASKS_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_TASKS_MEDIAPIPE_SRCS}
         #${TFLITE_DELEGATES_GPU_COMMON_TASKS_SPECIAL_SRCS}
         ${TFLITE_DELEGATES_GPU_COMMON_TRANSFORMATIONS_SRCS}
         ${TFLITE_DELEGATES_GPU_GL_KERNELS_MEDIAPIPE_SRC}
         ${TFLITE_DELEGATES_GPU_GL_SRC}
         ${TFLITE_DELEGATES_GPU_GL_COMPILER_SRC}
         ${TFLITE_DELEGATES_GPU_GL_CONVERTERS_SRC}
         ${TFLITE_DELEGATES_GPU_GL_KERNELS_SRC}
         ${TFLITE_DELEGATES_GPU_GL_WORKGROUPS_SRC}
         ${TFLITE_SOURCE_DIR}/tools/versioning/gpu_compatibility.cc
         ${TFLITE_SOURCE_DIR}/tools/versioning/op_signature.cc
    )
    include_directories(
            AFTER
            ${TFLITE_SOURCE_DIR}/delegates/gpu/common
            ${TFLITE_SOURCE_DIR}/delegates/gpu/common/task
    )
    if(TFLITE_ENABLE_METAL AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Darwin")
        #
        # libmetal_delegate library
        #
        enable_language(OBJCXX)
        list(APPEND TFLITE_DELEGATES_METAL_SRCS
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal_delegate.mm
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/buffer.cc
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/buffer_convert.mm
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/common.mm
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/compute_task.cc
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/inference_context.cc
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/metal_arguments.cc
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/metal_device.cc
             ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/metal_spatial_tensor.cc
        )
        add_library(metal_delegate STATIC
                    ${TFLITE_DELEGATES_METAL_SRCS}
        )
        target_include_directories(metal_delegate
                                   PRIVATE ${TENSORFLOW_SOURCE_DIR}
        )
        target_link_libraries(metal_delegate PUBLIC
                              flatbuffers::flatbuffers
                              absl::status
                              absl::strings
                              absl::flat_hash_map
                              absl::time
                              absl::span
        )
        #
        # generate flatbuffers header for inference_context
        #
        add_custom_command(
                OUTPUT ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/inference_context_generated.h
                COMMAND ${FLATC} --scoped-enums
                -I ${TENSORFLOW_SOURCE_DIR}
                -o ${TFLITE_SOURCE_DIR}/delegates/gpu/metal
                -c ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/inference_context.fbs
        )
        add_custom_target(
                inference_context_cc_fbs
                DEPENDS ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/inference_context_generated.h
        )
        add_dependencies(metal_delegate inference_context_cc_fbs)
        #
        # supplementary libraries for libmetal_delegate
        #
        list(APPEND CC_SRCS
             buffer
             compute_task
             inference_context
             metal_arguments
             metal_device
             metal_spatial_tensor
        )
        SET(METAL_DELEGATE_PATH ${TFLITE_SOURCE_DIR}/delegates/gpu/metal/)

        foreach(lib_name ${CC_SRCS})
            set_source_files_properties(${METAL_DELEGATE_PATH}${lib_name}.cc  PROPERTIES LANGUAGE OBJCXX)
            add_library("${lib_name}" STATIC ${METAL_DELEGATE_PATH}${lib_name}.cc)
            set_target_properties(${lib_name} PROPERTIES LINKER_LANGUAGE OBJCXX)
            target_link_libraries(${lib_name} PUBLIC
                                  flatbuffers::flatbuffers
                                  absl::status
                                  absl::strings
                                  absl::flat_hash_map
                                  absl::time
                                  absl::span
            )
        endforeach()

        list(APPEND MM_SRCS
             buffer_convert
             common
        )
        foreach(lib_name ${MM_SRCS})
            add_library("${lib_name}" STATIC ${METAL_DELEGATE_PATH}${lib_name}.mm)
            target_link_libraries(${lib_name} PUBLIC
                                  flatbuffers::flatbuffers
                                  absl::status
                                  absl::strings
                                  absl::flat_hash_map
                                  absl::time
                                  absl::span
            )
        endforeach()
    endif()
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DCL_DELEGATE_NO_GL" "-DEGL_NO_X11")
    list(APPEND TFLITE_TARGET_DEPENDENCIES
         absl::any
         absl::flat_hash_map
    )
endif()

add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/src/schema/schema_generated.h
        COMMAND flatbuffers::flatc --gen-mutable --gen-object-api
        -I ${TENSORFLOW_SOURCE_DIR}
        -o ${CMAKE_BINARY_DIR}/src/schema
        -c ${TFLITE_SOURCE_DIR}/schema/schema.fbs
)
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/src/schema/conversion_metadata_generated.h
        COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api
        -I ${TENSORFLOW_SOURCE_DIR}
        -o ${CMAKE_BINARY_DIR}/src/schema
        -c ${TFLITE_SOURCE_DIR}/schema/conversion_metadata.fbs
)
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/src/acceleration/configuration/configuration_generated.h
        COMMAND flatbuffers::flatc --proto
        -o ${CMAKE_BINARY_DIR}/src/acceleration/configuration
        ${CMAKE_SOURCE_DIR}/src/acceleration/configuration/configuration.proto
        COMMAND sed -i 's/tflite\.proto/tflite/g' ${CMAKE_BINARY_DIR}/src/acceleration/configuration/configuration.fbs
        COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api --gen-compare
        -I ${TENSORFLOW_SOURCE_DIR}
        -o ${CMAKE_BINARY_DIR}/src/acceleration/configuration
        -c ${CMAKE_BINARY_DIR}/src/acceleration/configuration/configuration.fbs
)
add_custom_command(
        OUTPUT ${CMAKE_BINARY_DIR}/src/compiler/mlir/lite/schema/schema_generated.h
        COMMAND flatbuffers::flatc --no-union-value-namespacing --gen-object-api
        -I ${TENSORFLOW_SOURCE_DIR}
        -o ${CMAKE_BINARY_DIR}/src/compiler/mlir/lite/schema
        -c ${TFLITE_SOURCE_DIR}/compiler/mlir/lite/schema/schema.fbs
)
add_custom_target(
        main_fbs
        DEPENDS ${CMAKE_BINARY_DIR}/src/compiler/mlir/lite/schema/schema_generated.h ${CMAKE_BINARY_DIR}/src/acceleration/configuration/configuration_generated.h ${CMAKE_BINARY_DIR}/src/schema/conversion_metadata_generated.h ${CMAKE_BINARY_DIR}/src/schema/schema_generated.h
)

if(_TFLITE_ENABLE_NNAPI)
    find_package(fp16_headers REQUIRED)
    populate_tflite_source_vars("delegates/nnapi"
                                TFLITE_DELEGATES_NNAPI_SRCS
                                FILTER "(_test_list|_disabled)\\.(cc|h)$"
    )
    populate_tflite_source_vars(
            "nnapi" TFLITE_NNAPI_SRCS
            FILTER "nnapi_handler.*"
            FILTER "(_disabled)\\.(cc|h)$"
    )

    #list(APPEND TFLITE_NNAPI_SRCS
    #     "${TFLITE_SOURCE_DIR}/nnapi/sl/SupportLibrary.cc"
    #)

    if(${TFLITE_ENABLE_NNAPI_VERBOSE_VALIDATION})
        list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DNNAPI_VERBOSE_VALIDATION")
    endif()
else()
    set(TFLITE_DELEGATES_NNAPI_SRCS
        "${TFLITE_SOURCE_DIR}/delegates/nnapi/nnapi_delegate_disabled.cc"
    )
    set(TFLITE_NNAPI_SRCS
        "${TFLITE_SOURCE_DIR}/nnapi/nnapi_implementation_disabled.cc"
    )
endif()
if(TFLITE_ENABLE_XNNPACK)
    find_package(fp16_headers REQUIRED)
    find_package(tfl-XNNPACK${LIBRARY_POSTFIX} REQUIRED CONFIG)
    populate_tflite_source_vars("delegates/xnnpack"
                                TFLITE_DELEGATES_XNNPACK_SRCS
                                FILTER ".*(_test|_tester)\\.(cc|h)"
    )
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DTFLITE_BUILD_WITH_XNNPACK_DELEGATE")
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DXNNPACK_DELEGATE_ENABLE_QS8")
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DXNNPACK_DELEGATE_ENABLE_QU8")
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DXNNPACK_DELEGATE_USE_LATEST_OPS")
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DXNNPACK_DELEGATE_ENABLE_SUBGRAPH_RESHAPING")
endif()
if(TFLITE_ENABLE_EXTERNAL_DELEGATE)
    populate_tflite_source_vars("delegates/external"
                                TFLITE_DELEGATES_EXTERNAL_SRCS
                                FILTER ".*(_test|_tester)\\.(cc|h)"
    )
endif()
populate_tflite_source_vars("experimental/remat"
                            TFLITE_EXPERIMENTAL_REMAT_SRCS
                            FILTER ".*_test\\.(cc|h)$"
)
if (TFLITE_ENABLE_RESOURCE)
    populate_tflite_source_vars("experimental/resource"
                                TFLITE_EXPERIMENTAL_RESOURCE_SRCS
    )
endif()
populate_tflite_source_vars("experimental/ruy"
                            TFLITE_EXPERIMENTAL_RUY_SRCS
                            FILTER
                            ".*(test(_fast|_slow|_special_specs))\\.(cc|h)$"
                            ".*(benchmark|tune_tool|example)\\.(cc|h)$"
)
populate_tflite_source_vars("experimental/ruy/profiler"
                            TFLITE_EXPERIMENTAL_RUY_PROFILER_SRCS
                            FILTER ".*(test|test_instrumented_library)\\.(cc|h)$"
)
if(_TFLITE_ENABLE_RUY)
    message(STATUS "RUY is enabled.")
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DTFLITE_WITH_RUY")
endif()
populate_tflite_source_vars("kernels"
                            TFLITE_KERNEL_SRCS
                            FILTER "(.*_test_util_internal|test_.*|.*_ops_wrapper)\\.(cc|h)"
)
populate_tflite_source_vars("kernels/internal" TFLITE_KERNEL_INTERNAL_SRCS)
populate_tflite_source_vars("kernels/internal/optimized"
                            TFLITE_KERNEL_INTERNAL_OPT_SRCS
)
populate_tflite_source_vars("kernels/internal/optimized/integer_ops"
                            TFLITE_KERNEL_INTERNAL_OPT_INTEGER_OPS_SRCS
)
populate_tflite_source_vars("kernels/internal/optimized/sparse_ops"
                            TFLITE_KERNEL_INTERNAL_OPT_SPARSE_OPS_SRCS
)
populate_tflite_source_vars("kernels/internal/reference"
                            TFLITE_KERNEL_INTERNAL_REF_SRCS
)
populate_tflite_source_vars("kernels/internal/reference/integer_ops"
                            TFLITE_KERNEL_INTERNAL_REF_INTEGER_OPS_SRCS
)
populate_tflite_source_vars("kernels/internal/reference/sparse_ops"
                            TFLITE_KERNEL_INTERNAL_REF_SPARSE_OPS_SRCS
)
populate_tflite_source_vars("kernels/internal/optimized/4bit"
                            TFLITE_KERNEL_INTERNAL_OPT_4BIT_SRCS
                            FILTER "(.*neon.*)\\.(cc|h)"
)
populate_tflite_source_vars("kernels/internal/utils"
                            TFLITE_KERNEL_INTERNAL_UTILS_SRCS
)
set(TFLITE_PROFILER_SRCS
    ${TFLITE_SOURCE_DIR}/profiling/platform_profiler.cc
    ${TFLITE_SOURCE_DIR}/profiling/root_profiler.h
    ${TFLITE_SOURCE_DIR}/profiling/root_profiler.cc
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/profiler.cc
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/profiler.h
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/telemetry.cc
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/c/telemetry_setting_internal.cc
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/c/telemetry_setting_internal.h
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/c/profiler.h
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/c/telemetry_setting.h
    ${TFLITE_SOURCE_DIR}/profiling/telemetry/telemetry_status.h
)
if(CMAKE_SYSTEM_NAME MATCHES "Android")
    list(APPEND TFLITE_PROFILER_SRCS
         ${TFLITE_SOURCE_DIR}/profiling/atrace_profiler.cc
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "iOS")
    enable_language(OBJCXX)
    list(APPEND TFLITE_PROFILER_SRCS
         ${TFLITE_SOURCE_DIR}/profiling/signpost_profiler.mm
    )
    set_source_files_properties(${TFLITE_SOURCE_DIR}/profiling/signpost_profiler.mm PROPERTIES COMPILE_FLAGS -fno-objc-arc)
endif()

# TFLite library
set(_ALL_TFLITE_SRCS
    ${TFLITE_CORE_ACCELERATION_SRCS}
    ${TFLITE_CORE_API_SRCS}
    ${TFLITE_CORE_C_SRCS}
    ${TFLITE_CORE_EXPERIMENTAL_SRCS}
    ${TFLITE_CORE_KERNELS_SRCS}
    ${TFLITE_CORE_SRCS}
    ${TFLITE_CORE_ASYNC_SRCS}
    ${TFLITE_CORE_ASYNC_C_SRCS}
    ${TFLITE_CORE_ASYNC_INTEROP_SRCS}
    ${TFLITE_CORE_ASYNC_INTEROP_C_SRCS}
    ${TFLITE_CORE_TOOLS_SRCS}
    ${TFLITE_C_SRCS}
    ${TFLITE_DELEGATES_FLEX_SRCS}
    ${TFLITE_DELEGATES_GPU_SRCS}
    ${TFLITE_DELEGATES_NNAPI_SRCS}
    ${TFLITE_DELEGATES_SRCS}
    ${TFLITE_DELEGATES_XNNPACK_SRCS}
    ${TFLITE_DELEGATES_EXTERNAL_SRCS}
    ${TFLITE_EXPERIMENTAL_REMAT_SRCS}
    ${TFLITE_EXPERIMENTAL_RESOURCE_SRCS}
    ${TFLITE_EXPERIMENTAL_RUY_PROFILER_SRCS}
    ${TFLITE_EXPERIMENTAL_RUY_SRCS}
    ${TFLITE_KERNEL_INTERNAL_OPT_INTEGER_OPS_SRCS}
    ${TFLITE_KERNEL_INTERNAL_OPT_SPARSE_OPS_SRCS}
    ${TFLITE_KERNEL_INTERNAL_OPT_SRCS}
    ${TFLITE_KERNEL_INTERNAL_OPT_4BIT_SRCS}
    ${TFLITE_KERNEL_INTERNAL_REF_INTEGER_OPS_SRCS}
    ${TFLITE_KERNEL_INTERNAL_REF_SPARSE_OPS_SRCS}
    ${TFLITE_KERNEL_INTERNAL_REF_SRCS}
    ${TFLITE_KERNEL_INTERNAL_SRCS}
    ${TFLITE_KERNEL_INTERNAL_UTILS_SRCS}
    ${TFLITE_KERNEL_SRCS}
    ${TFLITE_NNAPI_SRCS}
    ${TFLITE_SRCS}
    ${TFLITE_PROFILER_SRCS}
    ${TFLITE_SOURCE_DIR}/internal/signature_def.h
    ${TFLITE_SOURCE_DIR}/kernels/internal/utils/sparsity_format_converter.cc
    ${CMAKE_BINARY_DIR}/src/schema/conversion_metadata_generated.h
    ${TFLITE_SOURCE_DIR}/schema/schema_utils.cc
    ${CMAKE_BINARY_DIR}/src/schema/schema_generated.h
)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(LIBRARY_NAME "tensorflow-lite${LIBRARY_POSTFIX}")
if(BUILD_SHARED_LIBS)
    add_library(${LIBRARY_NAME} SHARED
                ${_ALL_TFLITE_SRCS}
    )
else()
    add_library(${LIBRARY_NAME} STATIC
                ${_ALL_TFLITE_SRCS}
    )
endif()
add_dependencies(${LIBRARY_NAME} main_fbs)
if(TFL_ENABLE_GPU)
    add_dependencies(${LIBRARY_NAME} gpu_fbs)
endif()
target_include_directories(${LIBRARY_NAME} PRIVATE
                           ${CMAKE_SOURCE_DIR}/src
                           ${CMAKE_SOURCE_DIR}/include
                           ${CMAKE_BINARY_DIR}/src
                           ${CMAKE_SOURCE_DIR}
)
#target_include_directories(${LIBRARY_NAME} PUBLIC ${fft2d_INCLUDE_DIRS})
set(_ALL_TFLITE_HDRS ${_ALL_TFLITE_SRCS})
list(FILTER _ALL_TFLITE_HDRS INCLUDE REGEX ".*\\.h$")
target_include_directories(${LIBRARY_NAME}
                           PUBLIC $<BUILD_INTERFACE:${TENSORFLOW_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
                           #PRIVATE ${TENSORFLOW_ROOT_DIR}
)
target_link_libraries(${LIBRARY_NAME}
                      PUBLIC
                      Eigen3::Eigen
                      absl::flags
                      absl::hash
                      absl::status
                      absl::strings
                      absl::synchronization
                      absl::variant
                      farmhash::farmhash
                      fft2d::fft2d_fftsg2d
                      flatbuffers::flatbuffers
                      gemmlowp::gemmlowp
                      ruy::ruy
                      pthreadpool::pthreadpool
                      ${CMAKE_DL_LIBS}
                      ${TFLITE_TARGET_DEPENDENCIES}
)

if (NOT BUILD_SHARED_LIBS)
    list(APPEND TFLITE_TARGET_PUBLIC_OPTIONS "-DTFL_STATIC_LIBRARY_BUILD")
endif()

target_compile_options(${LIBRARY_NAME}
                       PUBLIC ${TFLITE_TARGET_PUBLIC_OPTIONS}
                       PRIVATE ${TFLITE_TARGET_PRIVATE_OPTIONS}
)
add_library(${PROJECT_NAME}::tensorflowlite${LIBRARY_POSTFIX} ALIAS ${LIBRARY_NAME})

# The install targets.
if(TFLITE_ENABLE_INSTALL)
    install(
            TARGETS ${LIBRARY_NAME}
            EXPORT ${PROJECT_NAME}Targets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    foreach(hdr ${_ALL_TFLITE_HDRS})
        get_filename_component(dir ${hdr} DIRECTORY)
        file(RELATIVE_PATH dir ${CMAKE_CURRENT_SOURCE_DIR} ${dir})

        string(REPLACE "src/" "" dir ${dir})
        if(${dir} STREQUAL "src")
            set(dir "")
        endif()
        install(
                FILES ${hdr}
                DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/tensorflow/lite/${dir}"
        )
    endforeach()

    install(
            EXPORT ${PROJECT_NAME}Targets
            NAMESPACE ${PROJECT_NAME}::
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    )

    # Generate config file that's used by find_package(tensorflow-lite CONFIG).

    include(CMakePackageConfigHelpers)

    configure_package_config_file(
            "src/tools/cmake/${PROJECT_NAME}Config.cmake.in"
            "${PROJECT_BINARY_DIR}/${LIBRARY_NAME}Config.cmake"
            INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_NAME}"
    )

    install(FILES "${PROJECT_BINARY_DIR}/${LIBRARY_NAME}Config.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${LIBRARY_NAME}"
    )
endif()

# The kernel tests.
if(TFLITE_KERNEL_TEST)
    enable_testing()
    add_subdirectory(${TFLITE_SOURCE_DIR}/kernels)
endif()

# The benchmark tool.
add_subdirectory(${TFLITE_SOURCE_DIR}/tools/benchmark)

# The label_image example.
add_subdirectory(${TFLITE_SOURCE_DIR}/examples/label_image)

# Python interpreter wrapper.
add_library(_pywrap_tensorflow_interpreter_wrapper SHARED EXCLUDE_FROM_ALL
            ${TFLITE_SOURCE_DIR}/python/interpreter_wrapper/interpreter_wrapper.cc
            ${TFLITE_SOURCE_DIR}/python/interpreter_wrapper/interpreter_wrapper_pybind11.cc
            ${TFLITE_SOURCE_DIR}/python/interpreter_wrapper/numpy.cc
            ${TFLITE_SOURCE_DIR}/python/interpreter_wrapper/python_error_reporter.cc
            ${TFLITE_SOURCE_DIR}/python/interpreter_wrapper/python_utils.cc
)

# To remove "lib" prefix.
set_target_properties(_pywrap_tensorflow_interpreter_wrapper PROPERTIES PREFIX "")

target_include_directories(_pywrap_tensorflow_interpreter_wrapper
                           PUBLIC
                           ${TENSORFLOW_SOURCE_DIR}
                           #PRIVATE
                           #${TENSORFLOW_ROOT_DIR}
)

target_link_libraries(_pywrap_tensorflow_interpreter_wrapper
                      ${LIBRARY_NAME}
                      ${CMAKE_DL_LIBS}
)
target_compile_options(_pywrap_tensorflow_interpreter_wrapper
                       PUBLIC ${TFLITE_TARGET_PUBLIC_OPTIONS}
                       PRIVATE ${TFLITE_TARGET_PRIVATE_OPTIONS}
)

message("Will be building ${LIBRARY_NAME}")
