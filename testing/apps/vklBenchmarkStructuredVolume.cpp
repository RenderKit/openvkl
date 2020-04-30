// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include "../common/simd.h"
#include "benchmark/benchmark.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/random.h"

using namespace openvkl;
using namespace openvkl::testing;
using namespace ospcommon::utility;

void initializeOpenVKL()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

static void scalarRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  for (auto _ : state) {
    vkl_vec3f objectCoordinates{distX(), distY(), distZ()};

    benchmark::DoNotOptimize(
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK(scalarRandomSample);

template <int W>
void vectorRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
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

  for (auto _ : state) {
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
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(vectorRandomSample, 4);
BENCHMARK_TEMPLATE(vectorRandomSample, 8);
BENCHMARK_TEMPLATE(vectorRandomSample, 16);

template <int N>
void streamRandomSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

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

    vklComputeSampleN(vklSampler, N, objectCoordinates.data(), samples.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(streamRandomSample, 1);
BENCHMARK_TEMPLATE(streamRandomSample, 4);
BENCHMARK_TEMPLATE(streamRandomSample, 8);
BENCHMARK_TEMPLATE(streamRandomSample, 16);
BENCHMARK_TEMPLATE(streamRandomSample, 32);
BENCHMARK_TEMPLATE(streamRandomSample, 64);
BENCHMARK_TEMPLATE(streamRandomSample, 128);
BENCHMARK_TEMPLATE(streamRandomSample, 256);

static void scalarFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  vkl_vec3f objectCoordinates{0.1701f, 0.1701f, 0.1701f};

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK(scalarFixedSample);

template <int W>
void vectorFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
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

  for (auto _ : state) {
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
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(vectorFixedSample, 4);
BENCHMARK_TEMPLATE(vectorFixedSample, 8);
BENCHMARK_TEMPLATE(vectorFixedSample, 16);

template <int N>
void streamFixedSample(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  // use fixed coordinates for all benchmark iterations
  std::vector<vkl_vec3f> objectCoordinates(
      N, vkl_vec3f{0.1701f, 0.1701f, 0.1701f});

  std::vector<float> samples(N);

  for (auto _ : state) {
    vklComputeSampleN(vklSampler, N, objectCoordinates.data(), samples.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(streamFixedSample, 1);
BENCHMARK_TEMPLATE(streamFixedSample, 4);
BENCHMARK_TEMPLATE(streamFixedSample, 8);
BENCHMARK_TEMPLATE(streamFixedSample, 16);
BENCHMARK_TEMPLATE(streamFixedSample, 32);
BENCHMARK_TEMPLATE(streamFixedSample, 64);
BENCHMARK_TEMPLATE(streamFixedSample, 128);
BENCHMARK_TEMPLATE(streamFixedSample, 256);

static void scalarRandomGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  for (auto _ : state) {
    vkl_vec3f objectCoordinates{distX(), distY(), distZ()};

    benchmark::DoNotOptimize(
        vklComputeGradient(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK(scalarRandomGradient);

template <int W>
void vectorRandomGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
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
  vkl_vvec3f4 gradient4;
  vkl_vvec3f8 gradient8;
  vkl_vvec3f16 gradient16;

  for (auto _ : state) {
    for (int i = 0; i < W; i++) {
      objectCoordinates.x[i] = distX();
      objectCoordinates.y[i] = distY();
      objectCoordinates.z[i] = distZ();
    }

    if (W == 4) {
      vklComputeGradient4(valid,
                          vklSampler,
                          (const vkl_vvec3f4 *)&objectCoordinates,
                          &gradient4);
    } else if (W == 8) {
      vklComputeGradient8(valid,
                          vklSampler,
                          (const vkl_vvec3f8 *)&objectCoordinates,
                          &gradient8);
    } else if (W == 16) {
      vklComputeGradient16(valid,
                           vklSampler,
                           (const vkl_vvec3f16 *)&objectCoordinates,
                           &gradient16);
    } else {
      throw std::runtime_error(
          "vectorRandomGradient benchmark called with unimplemented calling "
          "width");
    }
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(vectorRandomGradient, 4);
BENCHMARK_TEMPLATE(vectorRandomGradient, 8);
BENCHMARK_TEMPLATE(vectorRandomGradient, 16);

template <int N>
void streamRandomGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  pcg32_biased_float_distribution distX(rd(), 0, bbox.lower.x, bbox.upper.x);
  pcg32_biased_float_distribution distY(rd(), 0, bbox.lower.y, bbox.upper.y);
  pcg32_biased_float_distribution distZ(rd(), 0, bbox.lower.z, bbox.upper.z);

  std::vector<vkl_vec3f> objectCoordinates(N);
  std::vector<vkl_vec3f> gradients(N);

  for (auto _ : state) {
    for (int i = 0; i < N; i++) {
      objectCoordinates[i].x = distX();
      objectCoordinates[i].y = distY();
      objectCoordinates[i].z = distZ();
    }

    vklComputeGradientN(
        vklSampler, N, objectCoordinates.data(), gradients.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(streamRandomGradient, 1);
BENCHMARK_TEMPLATE(streamRandomGradient, 4);
BENCHMARK_TEMPLATE(streamRandomGradient, 8);
BENCHMARK_TEMPLATE(streamRandomGradient, 16);
BENCHMARK_TEMPLATE(streamRandomGradient, 32);
BENCHMARK_TEMPLATE(streamRandomGradient, 64);
BENCHMARK_TEMPLATE(streamRandomGradient, 128);
BENCHMARK_TEMPLATE(streamRandomGradient, 256);

static void scalarFixedGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  vkl_vec3f objectCoordinates{0.1701f, 0.1701f, 0.1701f};

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        vklComputeGradient(vklSampler, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
  vklRelease(vklSampler);
}

BENCHMARK(scalarFixedGradient);

template <int W>
void vectorFixedGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume   = v->getVKLVolume();
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

  vkl_vvec3f4 gradient4;
  vkl_vvec3f8 gradient8;
  vkl_vvec3f16 gradient16;

  for (auto _ : state) {
    if (W == 4) {
      vklComputeGradient4(valid,
                          vklSampler,
                          (const vkl_vvec3f4 *)&objectCoordinates,
                          &gradient4);
    } else if (W == 8) {
      vklComputeGradient8(valid,
                          vklSampler,
                          (const vkl_vvec3f8 *)&objectCoordinates,
                          &gradient8);
    } else if (W == 16) {
      vklComputeGradient16(valid,
                           vklSampler,
                           (const vkl_vvec3f16 *)&objectCoordinates,
                           &gradient16);
    } else {
      throw std::runtime_error(
          "vectorFixedGradient benchmark called with unimplemented calling "
          "width");
    }
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * W);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(vectorFixedGradient, 4);
BENCHMARK_TEMPLATE(vectorFixedGradient, 8);
BENCHMARK_TEMPLATE(vectorFixedGradient, 16);

template <int N>
void streamFixedGradient(benchmark::State &state)
{
  auto v = ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
      vec3i(128), vec3f(0.f), vec3f(1.f));

  VKLVolume vklVolume = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  // use fixed coordinates for all benchmark iterations
  std::vector<vkl_vec3f> objectCoordinates(
      N, vkl_vec3f{0.1701f, 0.1701f, 0.1701f});

  std::vector<vkl_vec3f> gradients(N);

  for (auto _ : state) {
    vklComputeGradientN(
        vklSampler, N, objectCoordinates.data(), gradients.data());
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations() * N);
  vklRelease(vklSampler);
}

BENCHMARK_TEMPLATE(streamFixedGradient, 1);
BENCHMARK_TEMPLATE(streamFixedGradient, 4);
BENCHMARK_TEMPLATE(streamFixedGradient, 8);
BENCHMARK_TEMPLATE(streamFixedGradient, 16);
BENCHMARK_TEMPLATE(streamFixedGradient, 32);
BENCHMARK_TEMPLATE(streamFixedGradient, 64);
BENCHMARK_TEMPLATE(streamFixedGradient, 128);
BENCHMARK_TEMPLATE(streamFixedGradient, 256);

static void scalarIntervalIteratorConstruction(benchmark::State &state)
{
  static std::unique_ptr<WaveletStructuredRegularVolume<float>> v;
  static VKLVolume vklVolume;

  static vkl_vec3f origin;

  // global setup only in first thread
  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletStructuredRegularVolume<float>>(
        new WaveletStructuredRegularVolume<float>(
            vec3i(128), vec3f(0.f), vec3f(1.f)));

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

BENCHMARK(scalarIntervalIteratorConstruction)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(2)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(4)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(6)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(12)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(36)->UseRealTime();
BENCHMARK(scalarIntervalIteratorConstruction)->Threads(72)->UseRealTime();

static void scalarIntervalIteratorIterateFirst(benchmark::State &state)
{
  static std::unique_ptr<WaveletStructuredRegularVolume<float>> v;
  static VKLVolume vklVolume;

  static VKLIntervalIterator iterator;

  // global setup only in first thread
  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletStructuredRegularVolume<float>>(
        new WaveletStructuredRegularVolume<float>(
            vec3i(128), vec3f(0.f), vec3f(1.f)));

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

BENCHMARK(scalarIntervalIteratorIterateFirst)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(2)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(4)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(6)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(12)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(36)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateFirst)->Threads(72)->UseRealTime();

static void scalarIntervalIteratorIterateSecond(benchmark::State &state)
{
  static std::unique_ptr<WaveletStructuredRegularVolume<float>> v;
  static VKLVolume vklVolume;

  static VKLIntervalIterator iterator;

  // global setup only in first thread
  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletStructuredRegularVolume<float>>(
        new WaveletStructuredRegularVolume<float>(
            vec3i(128), vec3f(0.f), vec3f(1.f)));

    vklVolume = v->getVKLVolume();

    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

    vkl_vec3f origin{distX(eng), distY(eng), -1.f};
    vkl_vec3f direction{0.f, 0.f, 1.f};
    vkl_range1f tRange{0.f, 1000.f};

    vklInitIntervalIterator(
        &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

    // move past first iteration
    VKLInterval interval;
    vklIterateInterval(&iterator, &interval);
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

BENCHMARK(scalarIntervalIteratorIterateSecond)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(2)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(4)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(6)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(12)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(36)->UseRealTime();
BENCHMARK(scalarIntervalIteratorIterateSecond)->Threads(72)->UseRealTime();

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
