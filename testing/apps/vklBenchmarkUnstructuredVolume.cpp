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
#include "../common/simd.h"
#include "benchmark/benchmark.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;

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
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
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

  vkl_box3f bbox = vklGetBoundingBox(vklVolume);

  vkl_vec3f objectCoordinates{0.1701f, 0.1701f, 0.1701f};

  for (auto _ : state) {
    benchmark::DoNotOptimize(
        vklComputeSample(vklVolume, (const vkl_vec3f *)&objectCoordinates));
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());
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
