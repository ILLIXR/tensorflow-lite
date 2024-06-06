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

# grpc uses find_package in CONFIG mode for this package, so override the
# system installation and build from source instead.
message("STARTING ABSL")
find_package(absl CONFIG)
if(absl_FOUND)
  message("ABSL FOUND ${absl_LIBRARIES}")
else()
  include(abseil-cpp)
endif()
if(abseil-cpp_POPULATED)
  set(_ABSL_LIBRARY_NAMES
      algorithm
      algorithm_container
      any
      atomic_hook
      atomic_hook_test_helper
      awesome
      bad_any_cast
      bad_any_cast_impl
      bad_optional_access
      bad_variant_access
      base
      base_internal
      bind_front
      bits
      btree
      btree_test_common
      city
      civil_time
      compare
      compressed_tuple
      config
      conformance_testing
      container
      container_common
      container_memory
      cord
      cord_test_helpers
      core_headers
      counting_allocator
      debugging
      debugging_internal
      demangle_internal
      dynamic_annotations
      endian
      errno_saver
      examine_stack
      exception_safety_testing
      exception_testing
      exponential_biased
      failure_signal_handler
      fantastic_lib
      fast_type_id
      fixed_array
      flags
      flags_commandlineflag
      flags_commandlineflag_internal
      flags_config
      flags_internal
      flags_marshalling
      flags_parse
      flags_path_util
      flags_private_handle_accessor
      flags_program_name
      flags_reflection
      flags_usage
      flags_usage_internal
      flat_hash_map
      flat_hash_set
      function_ref
      graphcycles_internal
      hash
      hash_function_defaults
      hash_generator_testing
      hash_policy_testing
      hash_policy_traits
      hash_testing
      hashtable_debug
      hashtable_debug_hooks
      hashtablez_sampler
      have_sse
      hdrs
      inlined_vector
      inlined_vector_internal
      int128
      kernel_timeout_internal
      layout
      leak_check
      leak_check_api_disabled_for_testing
      leak_check_api_enabled_for_testing
      leak_check_disable
      log_severity
      main_lib
      malloc_internal
      memory
      meta
      node_hash_map
      node_hash_policy
      node_hash_set
      numeric
      optional
      per_thread_sem_test_common
      periodic_sampler
      pow10_helper
      pretty_function
      random_bit_gen_ref
      random_distributions
      random_internal_distribution_caller
      random_internal_distribution_test_util
      random_internal_explicit_seed_seq
      random_internal_fast_uniform_bits
      random_internal_fastmath
      random_internal_generate_real
      random_internal_iostream_state_saver
      random_internal_mock_helpers
      random_internal_mock_overload_set
      random_internal_nonsecure_base
      random_internal_pcg_engine
      random_internal_platform
      random_internal_pool_urbg
      random_internal_randen
      random_internal_randen_engine
      random_internal_randen_hwaes
      random_internal_randen_hwaes_impl
      random_internal_randen_slow
      random_internal_salted_seed_seq
      random_internal_seed_material
      random_internal_sequence_urbg
      random_internal_traits
      random_internal_uniform_helper
      random_internal_wide_multiply
      random_mocking_bit_gen
      random_random
      random_seed_gen_exception
      random_seed_sequences
      raw_hash_map
      raw_hash_set
      raw_logging_internal
      scoped_set_env
      span
      spinlock_test_common
      spinlock_wait
      spy_hash_state
      stack_consumption
      stacktrace
      status
      str_format
      str_format_internal
      strerror
      strings
      strings_internal
      symbolize
      synchronization
      test_instance_tracker
      thread_pool
      throw_delegate
      time
      time_internal_test_util
      time_zone
      tracked
      type_traits
      unordered_map_constructor_test
      unordered_map_lookup_test
      unordered_map_members_test
      unordered_map_modifiers_test
      unordered_set_constructor_test
      unordered_set_lookup_test
      unordered_set_members_test
      unordered_set_modifiers_test
      utility
      variant
  )
  set(_ABSL_LIBRARIES ${_ABSL_LIBRARY_NAMES})
  foreach(_LIBRARY ${_ABSL_LIBRARY_NAMES})
    list(APPEND _ABSL_LIBRARIES "absl::${LIBRARY}")
  endforeach()
  set(ABSL_LIBRARIES ${_ABSL_LIBRARIES} CACHE STRING "absl libs")

endif()