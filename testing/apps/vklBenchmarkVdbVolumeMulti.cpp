// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "benchmark_suite/volume.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;

/*
 * Vdb volume wrapper.
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
    return 4;
  }

  Vdb()
  {
    volume = std::unique_ptr<ProceduralVdbVolumeMulti>(
        generateMultiAttributeVdbVolume(vec3i(128),
                                        vec3f(0.f),
                                        vec3f(1.f),
                                        filter,
                                        VKL_DATA_SHARED_BUFFER,
                                        true));

    if (Vdb::getNumAttributes() != volume->getNumAttributes()) {
      throw std::runtime_error("inconsistent Vdb numAttributes");
    }

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

  std::unique_ptr<ProceduralVdbVolumeMulti> volume;
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
