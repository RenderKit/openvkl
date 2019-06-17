// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <random>
#include "benchmark/benchmark.h"
#include "../common/simd.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;

void initializeOpenVKL()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc_driver");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

static void scalarRandomSample(benchmark::State &state)
{
  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  for (auto _ : state) {
    vkl_vec3f objectCoordinates{distX(eng), distY(eng), distZ(eng)};

    benchmark::DoNotOptimize(
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(scalarRandomSample);

template <int W>
void vectorRandomSample(benchmark::State &state)
{
  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume = v->getVKLVolume();

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

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
      objectCoordinates.x[i] = distX(eng);
      objectCoordinates.y[i] = distY(eng);
      objectCoordinates.z[i] = distZ(eng);
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

BENCHMARK_TEMPLATE(vectorRandomSample, 4);
BENCHMARK_TEMPLATE(vectorRandomSample, 8);
BENCHMARK_TEMPLATE(vectorRandomSample, 16);

static void scalarRayIteratorConstruction(benchmark::State &state)
{
  static std::unique_ptr<WaveletProceduralVolume> v;
  static VKLVolume vklVolume;

  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletProceduralVolume>(
        new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

    vklVolume = v->getVKLVolume();
  }

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  vkl_vec3f origin{distX(eng), distY(eng), -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, 1000.f};

  for (auto _ : state) {
    VKLRayIterator rayIterator =
        vklNewRayIterator(vklVolume, &origin, &direction, &tRange, nullptr);

    benchmark::DoNotOptimize(rayIterator);
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(scalarRayIteratorConstruction)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(2)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(4)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(6)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(12)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(36)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(72)->UseRealTime();

static void scalarRayIteratorIterateIntervalFirst(benchmark::State &state)
{
  static std::unique_ptr<WaveletProceduralVolume> v;
  static VKLVolume vklVolume;

  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletProceduralVolume>(
        new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

    vklVolume = v->getVKLVolume();
  }

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

  vkl_vec3f origin{distX(eng), distY(eng), -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, 1000.f};

  VKLRayIterator rayIterator =
      vklNewRayIterator(vklVolume, &origin, &direction, &tRange, nullptr);

  VKLRayInterval rayInterval;

  for (auto _ : state) {
    VKLRayIterator rayIteratorTemp = rayIterator;

    bool success = vklIterateInterval(&rayIteratorTemp, &rayInterval);

    if (!success) {
      throw std::runtime_error("vklIterateInterval() returned false");
    }

    benchmark::DoNotOptimize(rayInterval);
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(scalarRayIteratorIterateIntervalFirst)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(2)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(4)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(6)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(12)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(36)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalFirst)->Threads(72)->UseRealTime();

static void scalarRayIteratorIterateIntervalSecond(benchmark::State &state)
{
  static std::unique_ptr<WaveletProceduralVolume> v;
  static VKLVolume vklVolume;

  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletProceduralVolume>(
        new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

    vklVolume = v->getVKLVolume();
  }

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

  vkl_vec3f origin{distX(eng), distY(eng), -1.f};
  vkl_vec3f direction{0.f, 0.f, 1.f};
  vkl_range1f tRange{0.f, 1000.f};

  VKLRayIterator rayIterator =
      vklNewRayIterator(vklVolume, &origin, &direction, &tRange, nullptr);

  // move past first iteration
  VKLRayInterval rayInterval;
  vklIterateInterval(&rayIterator, &rayInterval);

  for (auto _ : state) {
    VKLRayIterator rayIteratorTemp = rayIterator;

    bool success = vklIterateInterval(&rayIteratorTemp, &rayInterval);

    if (!success) {
      throw std::runtime_error("vklIterateInterval() returned false");
    }

    benchmark::DoNotOptimize(rayInterval);
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(scalarRayIteratorIterateIntervalSecond)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(2)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(4)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(6)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(12)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(36)->UseRealTime();
BENCHMARK(scalarRayIteratorIterateIntervalSecond)->Threads(72)->UseRealTime();

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();
}
