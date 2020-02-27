// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include "../common/simd.h"
#include "benchmark/benchmark.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/random.h"

using namespace openvkl::testing;
using namespace ospcommon::utility;
using openvkl::testing::WaveletVdbVolume;

void initializeOpenVKL()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

template <VKLFilter filter>
static void scalarRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      128, vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  for (auto _ : state) {
    vkl_vec3f objectCoordinates{distX(), distY(), distZ()};

    benchmark::DoNotOptimize(
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(scalarRandomSample, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(scalarRandomSample, VKL_FILTER_TRILINEAR);

template <int W, VKLFilter filter>
void vectorRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      128, vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  int valid[W];

  for (int i = 0; i < W; i++) {
    valid[i] = 1;
  }

  struct vvec3f
  {
    float x[W];
    float y[W];
    float z[W];
  };

  vvec3f objectCoordinates;
  float samples[W];

  for (auto _ : state) {
    for (int i = 0; i < W; i++) {
      objectCoordinates.x[i] = distX();
      objectCoordinates.y[i] = distY();
      objectCoordinates.z[i] = distZ();
    }

    if (W == 4) {
      vklComputeSample4(
          valid, vklVolume, (const vkl_vvec3f4 *)&objectCoordinates, samples);
    } else if (W == 8) {
      vklComputeSample8(
          valid, vklVolume, (const vkl_vvec3f8 *)&objectCoordinates, samples);
    } else if (W == 16) {
      vklComputeSample16(
          valid, vklVolume, (const vkl_vvec3f16 *)&objectCoordinates, samples);
    } else {
      throw std::runtime_error(
          "vectorRandomSample benchmark called with unimplemented calling "
          "width");
    }
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
}

BENCHMARK_TEMPLATE(vectorRandomSample, 4, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(vectorRandomSample, 8, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(vectorRandomSample, 16, VKL_FILTER_NEAREST);

BENCHMARK_TEMPLATE(vectorRandomSample, 4, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(vectorRandomSample, 8, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(vectorRandomSample, 16, VKL_FILTER_TRILINEAR);

template <int N, VKLFilter filter>
void streamRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      vec3i(128), vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  std::vector<vkl_vec3f> objectCoordinates(N);
  std::vector<float> samples(N);

  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      objectCoordinates[i].x = distX();
      objectCoordinates[i].y = distY();
      objectCoordinates[i].z = distZ();
    }

    vklComputeSampleN(vklVolume, N, objectCoordinates.data(), samples.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
}

// use mangled name so these don't yet show up in CI-run benchmarks
BENCHMARK_TEMPLATE(streamRandomSample, 1, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 4, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 8, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 16, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 32, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 64, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 128, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamRandomSample, 256, VKL_FILTER_NEAREST);

BENCHMARK_TEMPLATE(streamRandomSample, 1, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 4, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 8, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 16, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 32, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 64, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 128, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamRandomSample, 256, VKL_FILTER_TRILINEAR);

template <VKLFilter filter>
static void scalarFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      128, vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  vkl_vec3f objectCoordinates{0.1701f, 0.1701f, 0.1701f};

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(scalarFixedSample, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(scalarFixedSample, VKL_FILTER_TRILINEAR);

template <int W, VKLFilter filter>
void vectorFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      128, vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  int valid[W];

  for (int i = 0; i < W; i++) {
    valid[i] = 1;
  }

  struct vvec3f
  {
    float x[W];
    float y[W];
    float z[W];
  };

  // use fixed coordinates for all benchmark iterations
  vvec3f objectCoordinates;

  for (int i = 0; i < W; i++) {
    objectCoordinates.x[i] = 0.1701f;
    objectCoordinates.y[i] = 0.1701f;
    objectCoordinates.z[i] = 0.1701f;
  }

  float samples[W];

  for (auto _ : state) {
    if (W == 4) {
      vklComputeSample4(
          valid, vklVolume, (const vkl_vvec3f4 *)&objectCoordinates, samples);
    } else if (W == 8) {
      vklComputeSample8(
          valid, vklVolume, (const vkl_vvec3f8 *)&objectCoordinates, samples);
    } else if (W == 16) {
      vklComputeSample16(
          valid, vklVolume, (const vkl_vvec3f16 *)&objectCoordinates, samples);
    } else {
      throw std::runtime_error(
          "vectorFixedSample benchmark called with unimplemented calling "
          "width");
    }
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
}

BENCHMARK_TEMPLATE(vectorFixedSample, 4, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(vectorFixedSample, 8, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(vectorFixedSample, 16, VKL_FILTER_NEAREST);

BENCHMARK_TEMPLATE(vectorFixedSample, 4, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(vectorFixedSample, 8, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(vectorFixedSample, 16, VKL_FILTER_TRILINEAR);

template <int N, VKLFilter filter>
void streamFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletVdbVolume>(
      vec3i(128), vec3f(0.f), vec3f(1.f), filter);

  VKLVolume vklVolume = v->getVKLVolume();

  // use fixed coordinates for all benchmark iterations
  std::vector<vkl_vec3f> objectCoordinates(
      N, vkl_vec3f{0.1701f, 0.1701f, 0.1701f});

  std::vector<float> samples(N);

  for (auto _ : state) {
    vklComputeSampleN(vklVolume, N, objectCoordinates.data(), samples.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
}

// use mangled name so these don't yet show up in CI-run benchmarks
BENCHMARK_TEMPLATE(streamFixedSample, 1, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 4, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 8, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 16, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 32, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 64, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 128, VKL_FILTER_NEAREST);
BENCHMARK_TEMPLATE(streamFixedSample, 256, VKL_FILTER_NEAREST);

BENCHMARK_TEMPLATE(streamFixedSample, 1, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 4, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 8, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 16, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 32, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 64, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 128, VKL_FILTER_TRILINEAR);
BENCHMARK_TEMPLATE(streamFixedSample, 256, VKL_FILTER_TRILINEAR);

template <VKLFilter filter>
static void scalarIntervalIteratorConstruction(benchmark::State &state)
{
  static std::unique_ptr<WaveletVdbVolume> v;
  static VKLVolume vklVolume;

  static vkl_vec3f origin;

  // global setup only in first thread
  if (state.thread_index == 0) {
    v = ospcommon::make_unique<WaveletVdbVolume>(
        128, vec3f(0.f), vec3f(1.f), filter);

    vklVolume = v->getVKLVolume();

    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    std::random_device rd;
    std::mt19937 eng(rd());

    origin = vkl_vec3f{distX(eng), distY(eng), -1.f};
  }

  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, 1000.f};

  for (auto _ : state) {
    VKLIntervalIterator iterator;
    vklInitIntervalIterator(
        &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

    benchmark::DoNotOptimize(iterator);
  }

  // global teardown only in first thread
  if (state.thread_index == 0) {
    v = nullptr;
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(6)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(36)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_NEAREST)
    ->Threads(72)
    ->UseRealTime();

BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(6)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(36)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorConstruction, VKL_FILTER_TRILINEAR)
    ->Threads(72)
    ->UseRealTime();

template <VKLFilter filter>
static void scalarIntervalIteratorIterateFirst(benchmark::State &state)
{
  static std::unique_ptr<WaveletVdbVolume> v;
  static VKLVolume vklVolume;

  static VKLIntervalIterator iterator;

  // global setup only in first thread
  if (state.thread_index == 0) {
    v = ospcommon::make_unique<WaveletVdbVolume>(
        128, vec3f(0.f), vec3f(1.f), filter);

    vklVolume = v->getVKLVolume();

    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    std::random_device rd;
    std::mt19937 eng(rd());

    vkl_vec3f origin{distX(eng), distY(eng), -1.f};
    vkl_vec3f direction{0.f, 0.f, 1.f};
    vkl_range1f tRange{0.f, 1000.f};

    vklInitIntervalIterator(
        &iterator, vklVolume, &origin, &direction, &tRange, nullptr);
  }

  VKLInterval interval;

  for (auto _ : state) {
    VKLIntervalIterator iteratorTemp = iterator;

    bool success = vklIterateInterval(&iteratorTemp, &interval);

    if (!success) {
      throw std::runtime_error("vklIterateInterval() returned false");
    }

    benchmark::DoNotOptimize(interval);
  }

  // global teardown only in first thread
  if (state.thread_index == 0) {
    v = nullptr;
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(6)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(36)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_NEAREST)
    ->Threads(72)
    ->UseRealTime();

BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(2)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(4)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(6)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(12)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(36)
    ->UseRealTime();
BENCHMARK_TEMPLATE(scalarIntervalIteratorIterateFirst, VKL_FILTER_TRILINEAR)
    ->Threads(72)
    ->UseRealTime();

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
