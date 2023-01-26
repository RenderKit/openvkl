// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "benchmark_env.h"
#include "benchmark_suite/volume.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;
using openvkl::testing::WaveletStructuredRegularVolume;

/*
 * Structured volume wrapper.
 */
template <VKLFilter filter>
struct Structured
{
  static std::string name()
  {
    return toString<filter>();
  }

  static constexpr unsigned int getNumAttributes()
  {
    return 1;
  }

  Structured()
  {
    const int dim = getEnvBenchmarkVolumeDim();

    volume = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        vec3i(dim), vec3f(0.f), vec3f(1.f));

    vklVolume  = volume->getVKLVolume(getOpenVKLDevice());
    vklSampler = vklNewSampler(vklVolume);
    vklSetInt2(vklSampler, "filter", filter);
    vklSetInt2(vklSampler, "gradientFilter", filter);
    vklCommit2(vklSampler);
  }

  ~Structured()
  {
    vklRelease2(vklSampler);
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

  std::unique_ptr<WaveletStructuredRegularVolume<float>> volume;
  VKLVolume vklVolume;
  VKLSampler vklSampler;
};

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  registerVolumeBenchmarks<Structured<VKL_FILTER_NEAREST>>();
  registerVolumeBenchmarks<Structured<VKL_FILTER_TRILINEAR>>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  shutdownOpenVKL();

  return 0;
}
