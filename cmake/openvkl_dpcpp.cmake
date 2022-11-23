## Copyright 2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

macro(openvkl_add_sycl_target target)
  # "-fsycl" requires at least C++17
  target_compile_features(${target} PUBLIC cxx_std_17)
  target_compile_options(${target} PUBLIC -fsycl)
  target_link_options(${target} PUBLIC -fsycl)
endmacro()
