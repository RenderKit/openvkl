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

/*
 * Register all benchmarks for the given volume type.
 * A VolumeWrapper object corresponds to a single VKLVolume..
 *
 * Required methods are:
 *
 * // A human-readable string used in test name generation.
 * static <string-like> name()
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
}

