// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "openvkl_testing.h"
#include "benchmark_suite/volume.h"

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
    volume = rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
        vec3i(128), vec3f(0.f), vec3f(1.f));

    vklVolume  = volume->getVKLVolume(getOpenVKLDevice());
    vklSampler = vklNewSampler(vklVolume);
    vklSetInt(vklSampler, "filter", filter);
    vklSetInt(vklSampler, "gradientFilter", filter);
    vklCommit(vklSampler);
  }

  ~Structured()
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

  std::unique_ptr<WaveletStructuredRegularVolume<float>> volume;
  VKLVolume vklVolume{nullptr};
  VKLSampler vklSampler{nullptr};
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
