// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include "benchmark/benchmark.h"
#include "openvkl_testing.h"
#include "benchmark_suite/volume.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;
using openvkl::testing::WaveletVdbVolume;


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

  Vdb()
  {
    volume =
        rkcommon::make_unique<WaveletVdbVolume>(128, vec3f(0.f), vec3f(1.f));

    vklVolume  = volume->getVKLVolume();
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

  std::unique_ptr<WaveletVdbVolume> volume;
  VKLVolume vklVolume{nullptr};
  VKLSampler vklSampler{nullptr};
};



// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  registerVolumeBenchmarks<Vdb<VKL_FILTER_NEAREST>>();
  registerVolumeBenchmarks<Vdb<VKL_FILTER_TRILINEAR>>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();

  return 0;
}
