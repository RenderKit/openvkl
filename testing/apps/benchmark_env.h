// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/utility/getEnvVar.h"

inline int getEnvBenchmarkVolumeDim()
{
  const int defaultDim = 128;

  auto OPENVKL_BENCHMARK_VOLUME_DIM =
      rkcommon::utility::getEnvVar<int>("OPENVKL_BENCHMARK_VOLUME_DIM");
  int dim = OPENVKL_BENCHMARK_VOLUME_DIM.value_or(defaultDim);

  static bool printOnce = false;

  if (!printOnce && dim != defaultDim) {
    printOnce = true;
    std::cerr << "using benchmark volume dimension = " << dim << std::endl;
  }

  return dim;
}
