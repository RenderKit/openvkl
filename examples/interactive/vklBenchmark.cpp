// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
#include "window/VKLWindow.h"
// openvkl_testing
#include "openvkl_testing.h"
// google benchmark
#include "benchmark/benchmark.h"
// std
#include <sstream>
// ospcommon
#include "ospcommon/common.h"
#include "ospcommon/math/box.h"

using namespace openvkl::examples;
using namespace openvkl::testing;
using namespace ospcommon::math;
using openvkl::testing::WaveletVdbVolume;

static void render_wavelet_structured_regular(benchmark::State &state,
                                              const std::string &rendererType,
                                              const vec2i &windowSize,
                                              int volumeDimension,
                                              bool useISPC)
{
  auto proceduralVolume =
      ospcommon::make_unique<WaveletStructuredRegularVolume<float>>(
          vec3i(volumeDimension), vec3f(-1.f), vec3f(2.f / volumeDimension));

  Scene scene;
  scene.volume = proceduralVolume->getVKLVolume();

  auto window =
      ospcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

  window->setUseISPC(useISPC);

  for (auto _ : state) {
    window->render();
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());

  // save image on completion of benchmark; note we apparently have no way to
  // get the formal benchmark name, so we'll create one here
  static int ppmCounter = 0;
  std::stringstream ss;
  ss << "render_wavelet_structured_regular" << rendererType << "_" << ppmCounter
     << ".ppm";
  window->savePPM(ss.str());
  ppmCounter++;
}

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  density_pathtracer / 512 / scalar,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  density_pathtracer / 512 / ispc,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  hit_iterator / 512 / scalar,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  hit_iterator / 512 / ispc,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  ray_march_iterator / 512 / scalar,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_structured_regular,
                  ray_march_iterator / 512 / ispc,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  true);

static void render_wavelet_vdb(benchmark::State &state,
                               const std::string &rendererType,
                               const vec2i &windowSize,
                               int volumeDimension,
                               bool useISPC)
{
  auto proceduralVolume = ospcommon::make_unique<WaveletVdbVolume>(
      vec3i(volumeDimension), vec3f(-1.f), vec3f(2.f / volumeDimension));

  Scene scene;
  scene.volume = proceduralVolume->getVKLVolume();

  auto window =
      ospcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

  window->setUseISPC(useISPC);

  for (auto _ : state) {
    window->render();
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());

  // save image on completion of benchmark; note we apparently have no way to
  // get the formal benchmark name, so we'll create one here
  static int ppmCounter = 0;
  std::stringstream ss;
  ss << "render_wavelet_vdb" << rendererType << "_" << ppmCounter << ".ppm";
  window->savePPM(ss.str());
  ppmCounter++;
}

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  density_pathtracer / 512 / scalar,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  density_pathtracer / 512 / ispc,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  hit_iterator / 512 / scalar,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  hit_iterator / 512 / ispc,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  ray_march_iterator / 512 / scalar,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_vdb,
                  ray_march_iterator / 512 / ispc,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  true);

static void render_wavelet_unstructured_hex(benchmark::State &state,
                                            const std::string &rendererType,
                                            const vec2i &windowSize,
                                            int volumeDimension,
                                            bool useISPC)
{
  auto proceduralVolume =
      ospcommon::make_unique<WaveletUnstructuredProceduralVolume>(
          vec3i(volumeDimension),
          vec3f(-1.f),
          vec3f(2.f / volumeDimension),
          VKL_HEXAHEDRON,
          false);

  Scene scene;
  scene.volume = proceduralVolume->getVKLVolume();

  auto window =
      ospcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

  window->setUseISPC(useISPC);

  for (auto _ : state) {
    window->render();
  }

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());

  // save image on completion of benchmark; note we apparently have no way to
  // get the formal benchmark name, so we'll create one here
  static int ppmCounter = 0;
  std::stringstream ss;
  ss << "render_wavelet_unstructured_hex" << rendererType << "_" << ppmCounter
     << ".ppm";
  window->savePPM(ss.str());
  ppmCounter++;
}

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  density_pathtracer / 512 / scalar,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  density_pathtracer / 512 / ispc,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  hit_iterator / 512 / scalar,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  hit_iterator / 512 / ispc,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  ray_march_iterator / 512 / scalar,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet_unstructured_hex,
                  ray_march_iterator / 512 / ispc,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  true);

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
