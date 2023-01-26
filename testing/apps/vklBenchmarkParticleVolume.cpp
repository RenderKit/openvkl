// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "benchmark/benchmark.h"
#include "benchmark_suite/volume.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;
using namespace rkcommon::utility;
using openvkl::testing::ProceduralParticleVolume;

/*
 * Particle volume wrapper.
 */
struct Particle
{
  static std::string name()
  {
    return std::string();
  }

  static constexpr unsigned int getNumAttributes()
  {
    return 1;
  }

  Particle()
  {
    volume = rkcommon::make_unique<ProceduralParticleVolume>(1000);

    vklVolume  = volume->getVKLVolume(getOpenVKLDevice());
    vklSampler = vklNewSampler(vklVolume);
    vklCommit2(vklSampler);
  }

  ~Particle()
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

  std::unique_ptr<ProceduralParticleVolume> volume;
  VKLVolume vklVolume;
  VKLSampler vklSampler;
};

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  registerVolumeBenchmarks<Particle>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  shutdownOpenVKL();

  return 0;
}
