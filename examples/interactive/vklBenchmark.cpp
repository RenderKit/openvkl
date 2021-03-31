// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
#include "Renderer_ispc.h"
#include "window/VKLWindow.h"
// openvkl_testing
#include "openvkl_testing.h"
// google benchmark
#include "benchmark/benchmark.h"
// std
#include <sstream>
#include <string>
// rkcommon
#include "rkcommon/common.h"
#include "rkcommon/math/box.h"

using namespace openvkl::examples;
using namespace openvkl::testing;
using namespace rkcommon::math;
using openvkl::testing::WaveletVdbVolumeFloat;

static bool rendererIsCompatibleWithDriver(const std::string &rendererType,
                                           bool useISPC,
                                           std::string &errorString)
{
  // ISPC renderers that use iterator APIs must match width with the
  // instantiated VKL driver
  if (useISPC && rendererType.find("iterator") != std::string::npos) {
    const int driverNativeSIMDWidth = vklGetNativeSIMDWidth();
    const int ispcRendererSIMDWidth = ispc::Renderer_pixelsPerJob();

    if (driverNativeSIMDWidth != ispcRendererSIMDWidth) {
      std::stringstream ss;
      ss << rendererType << " (useISPC = " << useISPC
         << ") is not compatible with the current VKL driver (driver width = "
         << driverNativeSIMDWidth
         << ", renderer width = " << ispcRendererSIMDWidth << ")";

      errorString = ss.str();

      return false;
    }
  }

  return true;
}

// This macro will only instantiate the benchmark in Google benchmark if it's
// compatible with the current driver. There are other approaches to abort
// benchmark runs (e.g. `state.SkipWithError()`), but with those benchmark
// results to still have JSON output populated, which we do not want. This
// approach avoids that.
#define BENCHMARK_CAPTURE_IF_COMPATIBLE(FUNC,                             \
                                        TEST_CASE_NAME,                   \
                                        RENDERER_TYPE,                    \
                                        WINDOW_SIZE,                      \
                                        VOLUME_DIMENSION,                 \
                                        USE_ISPC)                         \
  {                                                                       \
    std::string errorString;                                              \
    if (!rendererIsCompatibleWithDriver(                                  \
            RENDERER_TYPE, USE_ISPC, errorString)) {                      \
      std::cerr << "skipping benchmark capture: " << #FUNC << ", "        \
                << #TEST_CASE_NAME << "\n\t" << errorString << std::endl; \
    } else {                                                              \
      BENCHMARK_CAPTURE(FUNC,                                             \
                        TEST_CASE_NAME,                                   \
                        RENDERER_TYPE,                                    \
                        WINDOW_SIZE,                                      \
                        VOLUME_DIMENSION,                                 \
                        USE_ISPC);                                        \
    }                                                                     \
  }

static void setupSceneDefaults(Scene &scene)
{
  if (!scene.volume) {
    throw std::runtime_error("scene must have an active volume");
  }

  // set a default transfer function
  static TransferFunction transferFunction;

  scene.tfValueRange            = box1f(0.f, 1.f);
  scene.tfColorsAndOpacities    = transferFunction.colorsAndOpacities.data();
  scene.tfNumColorsAndOpacities = transferFunction.colorsAndOpacities.size();

  // and a default value selector, with default isovalues
  scene.updateValueSelector(transferFunction,
                            std::vector<float>{-1.f, 0.f, 1.f});
}

static void render_wavelet_structured_regular(benchmark::State &state,
                                              const std::string &rendererType,
                                              const vec2i &windowSize,
                                              int volumeDimension,
                                              bool useISPC)
{
  auto proceduralVolume =
      rkcommon::make_unique<WaveletStructuredRegularVolume<float>>(
          vec3i(volumeDimension), vec3f(-1.f), vec3f(2.f / volumeDimension));

  Scene scene;
  scene.updateVolume(proceduralVolume->getVKLVolume());

  setupSceneDefaults(scene);

  auto window =
      rkcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

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

static void render_wavelet_vdb(benchmark::State &state,
                               const std::string &rendererType,
                               const vec2i &windowSize,
                               int volumeDimension,
                               bool useISPC)
{
  auto proceduralVolume = rkcommon::make_unique<WaveletVdbVolumeFloat>(
      vec3i(volumeDimension), vec3f(-1.f), vec3f(2.f / volumeDimension));

  Scene scene;
  scene.updateVolume(proceduralVolume->getVKLVolume());

  setupSceneDefaults(scene);

  auto window =
      rkcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

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

static void render_wavelet_unstructured_hex(benchmark::State &state,
                                            const std::string &rendererType,
                                            const vec2i &windowSize,
                                            int volumeDimension,
                                            bool useISPC)
{
  auto proceduralVolume =
      rkcommon::make_unique<WaveletUnstructuredProceduralVolume>(
          vec3i(volumeDimension),
          vec3f(-1.f),
          vec3f(2.f / volumeDimension),
          VKL_HEXAHEDRON,
          false);

  Scene scene;
  scene.updateVolume(proceduralVolume->getVKLVolume());

  setupSceneDefaults(scene);

  auto window =
      rkcommon::make_unique<VKLWindow>(windowSize, scene, rendererType);

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

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  // wavelet structured regular
  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  density_pathtracer / 512 / scalar,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  density_pathtracer / 512 / ispc,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  hit_iterator / 512 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  hit_iterator / 512 / ispc,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  ray_march_iterator / 512 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  ray_march_iterator / 512 / ispc,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  // wavelet vdb
  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  density_pathtracer / 512 / scalar,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  density_pathtracer / 512 / ispc,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  hit_iterator / 512 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  hit_iterator / 512 / ispc,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  ray_march_iterator / 512 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  ray_march_iterator / 512 / ispc,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  // wavelet unstructured
  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  density_pathtracer / 512 / scalar,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  density_pathtracer / 512 / ispc,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  hit_iterator / 512 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  hit_iterator / 512 / ispc,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  ray_march_iterator / 512 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  false);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  ray_march_iterator / 512 / ispc,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512,
                                  true);

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();
}
