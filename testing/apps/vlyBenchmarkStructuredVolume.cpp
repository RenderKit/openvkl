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
#include "common/simd.h"
#include "volley_testing.h"

using namespace volley::testing;

void initializeVolley()
{
  vlyLoadModule("ispc_driver");

  VLYDriver driver = vlyNewDriver("ispc_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);
}

static void scalarRandomSample(benchmark::State &state)
{
  std::unique_ptr<WaveletProceduralVolume> v(
      new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

  VLYVolume vlyVolume = v->getVLYVolume();

  vly_box3f bbox = vlyGetBoundingBox(vlyVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  for (auto _ : state) {
    vly_vec3f objectCoordinates{distX(eng), distY(eng), distZ(eng)};

    benchmark::DoNotOptimize(
        vlyComputeSample(vlyVolume, (const vly_vec3f *)&objectCoordinates));
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

  VLYVolume vlyVolume = v->getVLYVolume();

  vly_box3f bbox = vlyGetBoundingBox(vlyVolume);

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
      vlyComputeSample4(
          valid, vlyVolume, (const vly_vvec3f4 *)&objectCoordinates, samples);
    } else if (W == 8) {
      vlyComputeSample8(
          valid, vlyVolume, (const vly_vvec3f8 *)&objectCoordinates, samples);
    } else if (W == 16) {
      vlyComputeSample16(
          valid, vlyVolume, (const vly_vvec3f16 *)&objectCoordinates, samples);
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
  static VLYVolume vlyVolume;

  if (state.thread_index == 0) {
    v = std::unique_ptr<WaveletProceduralVolume>(
        new WaveletProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

    vlyVolume = v->getVLYVolume();
  }

  vly_box3f bbox = vlyGetBoundingBox(vlyVolume);

  std::random_device rd;
  std::mt19937 eng(rd());

  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  vly_vec3f origin{distX(eng), distY(eng), -1.f};
  vly_vec3f direction{0.f, 0.f, 1.f};
  vly_range1f tRange{0.f, 1000.f};

  for (auto _ : state) {
    VLYRayIterator rayIterator =
        vlyNewRayIterator(vlyVolume, &origin, &direction, &tRange, nullptr);

    benchmark::DoNotOptimize(rayIterator);
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
}

BENCHMARK(scalarRayIteratorConstruction)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(3)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(6)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(9)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(12)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(36)->UseRealTime();
BENCHMARK(scalarRayIteratorConstruction)->Threads(72)->UseRealTime();

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeVolley();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  vlyShutdown();
}
