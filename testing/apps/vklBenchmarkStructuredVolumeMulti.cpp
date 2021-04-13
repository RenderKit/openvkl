// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "benchmark_suite/volume.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;
using openvkl::testing::WaveletStructuredRegularVolume;

/*
 * StructuredMulti volume wrapper.
 */
template <VKLFilter filter>
struct StructuredMulti
{
  static std::string name()
  {
    return toString<filter>();
  }

  static constexpr unsigned int getNumAttributes()
  {
    return 4;
  }

  StructuredMulti()
  {
    volume = std::unique_ptr<TestingStructuredVolumeMulti>(
        generateMultiAttributeStructuredRegularVolume(vec3i(128),
                                                      vec3f(0.f),
                                                      vec3f(1.f),
                                                      TemporalConfig(),
                                                      VKL_DATA_SHARED_BUFFER,
                                                      true));

    if (StructuredMulti::getNumAttributes() != volume->getNumAttributes()) {
      throw std::runtime_error("inconsistent StructuredMulti numAttributes");
    }

    vklVolume  = volume->getVKLVolume();
    vklSampler = vklNewSampler(vklVolume);
    vklSetInt(vklSampler, "filter", filter);
    vklSetInt(vklSampler, "gradientFilter", filter);
    vklCommit(vklSampler);
  }

  ~StructuredMulti()
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

  std::unique_ptr<TestingStructuredVolumeMulti> volume;
  VKLVolume vklVolume{nullptr};
  VKLSampler vklSampler{nullptr};
};

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  vklLoadModule("cpu_device");

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);
  vklSetCurrentDevice(device);

  registerVolumeBenchmarks<StructuredMulti<VKL_FILTER_NEAREST>>();
  registerVolumeBenchmarks<StructuredMulti<VKL_FILTER_TRILINEAR>>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();

  return 0;
}
