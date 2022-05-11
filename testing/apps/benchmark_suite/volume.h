// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

/*
 * Benchmark suite for volume types.
 * This can be parametrized with a volume type class.
 */

#include "utility.h"
#include "compute_sample.h"
#include "compute_gradient.h"
#include "interval_iterators.h"
#include "compute_sample_multi.h"

template <VKLFilter filter>
constexpr const char *toString();

template <>
inline constexpr const char *toString<VKL_FILTER_NEAREST>()
{
  return "VKL_FILTER_NEAREST";
}

template <>
inline constexpr const char *toString<VKL_FILTER_TRILINEAR>()
{
  return "VKL_FILTER_TRILINEAR";
}

template <>
inline constexpr const char *toString<VKL_FILTER_TRICUBIC>()
{
  return "VKL_FILTER_TRICUBIC";
}

/*
 * Register all benchmarks for the given volume type.
 * A VolumeWrapper object corresponds to a single VKLVolume..
 *
 * Required methods are:
 *
 * // A human-readable string used in test name generation.
 * static <string-like> name()
 *
 * // Number of attributes the volume will have.
 * static constexpr unsigned int getNumAttributes()
 *
 * // Return a handle to the underlying volume.
 * VKLVolume getVolume() const
 *
 * // Return a handle to the underlying sampler object.
 * VKLSampler getSampler() const
 *
 * See vklBenchmarkVdbVolume.cpp for an example of a valid VolumeWrapper struct.
 */
template <class VolumeWrapper>
inline void registerVolumeBenchmarks()
{
  using namespace coordinate_generator;

  registerComputeSample<VolumeWrapper, Fixed>();
  registerComputeSample<VolumeWrapper, Random>();

  registerComputeGradient<VolumeWrapper, Fixed>();
  registerComputeGradient<VolumeWrapper, Random>();

  registerIntervalIterators<VolumeWrapper>();

  if (VolumeWrapper::getNumAttributes() > 1)
  {
    registerComputeSampleMulti<VolumeWrapper, Fixed>();
    registerComputeSampleMulti<VolumeWrapper, Random>();
  }
}

