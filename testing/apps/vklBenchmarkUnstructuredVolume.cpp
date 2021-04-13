// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include "../common/simd.h"
#include "benchmark/benchmark.h"
#include "benchmark_suite/utility.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/random.h"

using namespace openvkl;
using namespace openvkl::testing;
using namespace rkcommon::utility;

#define BENCHMARK_ALL_PRIMS(...)                   \
  BENCHMARK_TEMPLATE(__VA_ARGS__, VKL_HEXAHEDRON)  \
      ->Ranges({{0, 1}, {0, 1}, {0, 1}, {0, 1}});  \
  BENCHMARK_TEMPLATE(__VA_ARGS__, VKL_TETRAHEDRON) \
      ->Ranges({{0, 1}, {0, 1}, {0, 1}});          \
  BENCHMARK_TEMPLATE(__VA_ARGS__, VKL_WEDGE)       \
      ->Ranges({{0, 1}, {0, 1}, {0, 1}});          \
  BENCHMARK_TEMPLATE(__VA_ARGS__, VKL_PYRAMID)     \
      ->Ranges({{0, 1}, {0, 1}, {0, 1}});

void initializeOpenVKL()
{
  vklLoadModule("cpu_device");

  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);
  vklSetCurrentDevice(device);
}

template <VKLUnstructuredCellType primType>
static void scalarRandomSample(benchmark::State &state)
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          vec3i(128),
          vec3f(0.f),
          vec3f(1.f),
          primType,
          state.range(0),
          state.range(1),
          state.range(2),
          primType == VKL_HEXAHEDRON ? state.range(3) : false));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  BENCHMARK_WARMUP_AND_RUN(({
    vkl_vec3f objectCoordinates{distX(), distY(), distZ()};

    benchmark::DoNotOptimize(
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }));

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK_ALL_PRIMS(scalarRandomSample);

template <int W, VKLUnstructuredCellType primType>
void vectorRandomSample(benchmark::State &state)
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          vec3i(128),
          vec3f(0.f),
          vec3f(1.f),
          primType,
          state.range(0),
          state.range(1),
          state.range(2),
          primType == VKL_HEXAHEDRON ? state.range(3) : false));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  int valid[W];

  for (int i = 0; i < W; i++) {
    valid[i] = 1;
  }

  vvec3fn<W> objectCoordinates;
  float samples[W];

  BENCHMARK_WARMUP_AND_RUN(({
    for (int i = 0; i < W; i++) {
      objectCoordinates.x[i] = distX();
      objectCoordinates.y[i] = distY();
      objectCoordinates.z[i] = distZ();
    }

    if (W == 4) {
      vklComputeSample4(
          valid, vklSampler, (const vkl_vvec3f4 *)&objectCoordinates, samples);
    } else if (W == 8) {
      vklComputeSample8(
          valid, vklSampler, (const vkl_vvec3f8 *)&objectCoordinates, samples);
    } else if (W == 16) {
      vklComputeSample16(
          valid, vklSampler, (const vkl_vvec3f16 *)&objectCoordinates, samples);
    } else {
      throw std::runtime_error(
          "vectorRandomSample benchmark called with unimplemented calling "
          "width");
    }
  }));

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_ALL_PRIMS(vectorRandomSample, 4)
BENCHMARK_ALL_PRIMS(vectorRandomSample, 8)
BENCHMARK_ALL_PRIMS(vectorRandomSample, 16)

template <VKLUnstructuredCellType primType>
static void scalarFixedSample(benchmark::State &state)
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          vec3i(128),
          vec3f(0.f),
          vec3f(1.f),
          primType,
          state.range(0),
          state.range(1),
          state.range(2),
          primType == VKL_HEXAHEDRON ? state.range(3) : false));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  vkl_vec3f objectCoordinates{0.1701f, 0.1701f, 0.1701f};

  BENCHMARK_WARMUP_AND_RUN(({
    benchmark::DoNotOptimize(
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }));

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK_ALL_PRIMS(scalarFixedSample)

template <int W, VKLUnstructuredCellType primType>
void vectorFixedSample(benchmark::State &state)
{
  std::unique_ptr<WaveletUnstructuredProceduralVolume> v(
      new WaveletUnstructuredProceduralVolume(
          vec3i(128),
          vec3f(0.f),
          vec3f(1.f),
          primType,
          state.range(0),
          state.range(1),
          state.range(2),
          primType == VKL_HEXAHEDRON ? state.range(3) : false));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  int valid[W];

  for (int i = 0; i < W; i++) {
    valid[i] = 1;
  }

  // use fixed coordinates for all benchmark iterations
  vvec3fn<W> objectCoordinates;

  for (int i = 0; i < W; i++) {
    objectCoordinates.x[i] = 0.1701f;
    objectCoordinates.y[i] = 0.1701f;
    objectCoordinates.z[i] = 0.1701f;
  }

  float samples[W];

  BENCHMARK_WARMUP_AND_RUN(({
    if (W == 4) {
      vklComputeSample4(
          valid, vklSampler, (const vkl_vvec3f4 *)&objectCoordinates, samples);
    } else if (W == 8) {
      vklComputeSample8(
          valid, vklSampler, (const vkl_vvec3f8 *)&objectCoordinates, samples);
    } else if (W == 16) {
      vklComputeSample16(
          valid, vklSampler, (const vkl_vvec3f16 *)&objectCoordinates, samples);
    } else {
      throw std::runtime_error(
          "vectorFixedSample benchmark called with unimplemented calling "
          "width");
    }
  }));

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_ALL_PRIMS(vectorFixedSample, 4)
BENCHMARK_ALL_PRIMS(vectorFixedSample, 8)
BENCHMARK_ALL_PRIMS(vectorFixedSample, 16)

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();

  return 0;
}
