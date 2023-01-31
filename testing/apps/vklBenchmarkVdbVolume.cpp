// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "benchmark_env.h"
#include "benchmark_suite/volume.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;
using openvkl::testing::WaveletVdbVolumeFloat;

/*
 * VDB volume wrapper.
 * Parametrize with the lookup filter type.
 */
template <VKLFilter filter>
struct Vdb
{
  static std::string name()
  {
    return toString<filter>();
  }

  static constexpr unsigned int getNumAttributes()
  {
    return 1;
  }

  Vdb()
  {
    const int dim = getEnvBenchmarkVolumeDim();

    const bool repackNodes = true;

    volume = rkcommon::make_unique<WaveletVdbVolumeFloat>(
        getOpenVKLDevice(), vec3i(dim), vec3f(0.f), vec3f(1.f), repackNodes);

    vklVolume  = volume->getVKLVolume(getOpenVKLDevice());
    vklSampler = vklNewSampler(vklVolume);
    vklSetInt(vklSampler, "filter", filter);
    vklSetInt(vklSampler, "gradientFilter", filter);
    vklCommit(vklSampler);
  }

  ~Vdb()
  {
    vklRelease(vklSampler);
    volume.reset();  // also releases the vklVolume handle
  }

  inline VKLVolume getVolume() const
  {
    return vklVolume;
  }

  inline VKLSampler getSampler() const
  {
    return vklSampler;
  }

  std::unique_ptr<WaveletVdbVolumeFloat> volume;
  VKLVolume vklVolume;
  VKLSampler vklSampler;
};

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  registerVolumeBenchmarks<Vdb<VKL_FILTER_NEAREST>>();
  registerVolumeBenchmarks<Vdb<VKL_FILTER_TRILINEAR>>();
  registerVolumeBenchmarks<Vdb<VKL_FILTER_TRICUBIC>>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  shutdownOpenVKL();

  return 0;
}
