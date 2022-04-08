// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer_ispc.h"  // for Renderer_pixelsPerJob()
#include "renderer/Renderer.h"
#include "renderer/Scene.h"

// openvkl_testing
#include "openvkl_testing.h"
// google benchmark
#include "benchmark/benchmark.h"
// std
#include <iomanip>
#include <sstream>
#include <string>
// rkcommon
#include "rkcommon/common.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/SaveImage.h"

using namespace openvkl::examples;
using namespace openvkl::testing;
using namespace rkcommon::math;
using openvkl::testing::WaveletVdbVolumeFloat;

static bool rendererIsCompatibleWithDevice(const std::string &rendererType,
                                           std::string &errorString)
{
  // ISPC renderers that use iterator APIs must match width with the
  // instantiated VKL device
  if (rendererType.find("ispc") &&
      rendererType.find("iterator") != std::string::npos) {
    const int deviceNativeSIMDWidth = vklGetNativeSIMDWidth(getOpenVKLDevice());
    const int ispcRendererSIMDWidth = ispc::Renderer_pixelsPerJob();

    if (deviceNativeSIMDWidth != ispcRendererSIMDWidth) {
      std::stringstream ss;
      ss << rendererType
         << " is not compatible with the current VKL device (device width = "
         << deviceNativeSIMDWidth
         << ", renderer width = " << ispcRendererSIMDWidth << ")";

      errorString = ss.str();

      return false;
    }
  }

  return true;
}

// This macro will only instantiate the benchmark in Google benchmark if it's
// compatible with the current device. There are other approaches to abort
// benchmark runs (e.g. `state.SkipWithError()`), but with those benchmark
// results to still have JSON output populated, which we do not want. This
// approach avoids that.
#define BENCHMARK_CAPTURE_IF_COMPATIBLE(                                       \
    FUNC, TEST_CASE_NAME, RENDERER_TYPE, WINDOW_SIZE, VOLUME_DIMENSION)        \
  {                                                                            \
    std::string errorString;                                                   \
    if (!rendererIsCompatibleWithDevice(RENDERER_TYPE, errorString)) {         \
      std::cerr << "skipping benchmark capture: " << #FUNC << ", "             \
                << #TEST_CASE_NAME << "\n\t" << errorString << std::endl;      \
    } else {                                                                   \
      BENCHMARK_CAPTURE(                                                       \
          FUNC, TEST_CASE_NAME, RENDERER_TYPE, WINDOW_SIZE, VOLUME_DIMENSION); \
    }                                                                          \
  }

static void run_benchmark(benchmark::State &state,
                          const std::string &field,
                          const std::string &volumeType,
                          const std::string &rendererType,
                          const vec2i &resolution,
                          int volumeDimension)
{
  const std::list<std::string> args = {"vklBenchmark",
                                       "-sync",
                                       "-volumeType",
                                       volumeType,
                                       "-field",
                                       field,
                                       "-framebufferSize",
                                       std::to_string(resolution.x),
                                       std::to_string(resolution.y),
                                       "-gridDimensions",
                                       std::to_string(volumeDimension),
                                       std::to_string(volumeDimension),
                                       std::to_string(volumeDimension),
                                       "-vdbRepackNodes",
                                       "1"};

  Scene scene;
  scene.parseCommandLine(args);

  auto &scheduler = scene.scheduler;
  auto &volume    = scene.volume;

  volume.updateVKLObjects();
  scene.camera->fitToScreen(volume.getBounds());
  scene.camera.incrementVersion();

  auto rendererPtr = scene.createRenderer(rendererType);
  assert(rendererPtr);

  Renderer &renderer = *(rendererPtr.get());
  // This call will resize the framebuffer to our desired output
  // resolution.
  renderer.getFramebuffer(resolution.x, resolution.y);

  scheduler.start(renderer);
  for (auto _ : state) {
    scheduler.renderFrame(renderer);
  }
  scheduler.stop(renderer);

  // enables rates in report output
  state.SetItemsProcessed(state.iterations());

  static size_t ctr = 0;
  std::ostringstream os;
  os << std::setw(4) << std::setfill('0') << ctr++ << "_" << field
     << "_" << volumeType << "_" << rendererType << ".ppm";

  const auto &framebuffer = renderer.getFramebuffer(resolution.x, resolution.y);
  const auto &fb          = framebuffer.getFrontBuffer();
  rkcommon::utility::writePFM(
      os.str(), fb.getWidth(), fb.getHeight(), fb.getRgba());
}

static void render_wavelet_structured_regular(benchmark::State &state,
                                              const std::string &rendererType,
                                              const vec2i &windowSize,
                                              int volumeDimension)
{
  run_benchmark(state,
                "wavelet",
                "structuredRegular",
                rendererType,
                windowSize,
                volumeDimension);
}

static void render_wavelet_vdb(benchmark::State &state,
                               const std::string &rendererType,
                               const vec2i &windowSize,
                               int volumeDimension)
{
  run_benchmark(state,
                "wavelet",
                "vdb",
                rendererType,
                windowSize,
                volumeDimension);
}

static void render_wavelet_unstructured_hex(benchmark::State &state,
                                            const std::string &rendererType,
                                            const vec2i &windowSize,
                                            int volumeDimension)
{
  // Note: this is a hex volume because hex cells are the default!
  run_benchmark(state,
                "wavelet",
                "unstructured",
                rendererType,
                windowSize,
                volumeDimension);
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
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  density_pathtracer / 512 / ispc,
                                  "density_pathtracer_ispc",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  hit_iterator / 512 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  hit_iterator / 512 / ispc,
                                  "hit_iterator_ispc",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  ray_march_iterator / 512 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_structured_regular,
                                  ray_march_iterator / 512 / ispc,
                                  "ray_march_iterator_ispc",
                                  vec2i(1024),
                                  512);

  // wavelet vdb
  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  density_pathtracer / 512 / scalar,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  density_pathtracer / 512 / ispc,
                                  "density_pathtracer_ispc",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  hit_iterator / 512 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  hit_iterator / 512 / ispc,
                                  "hit_iterator_ispc",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  ray_march_iterator / 512 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  512);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_vdb,
                                  ray_march_iterator / 512 / ispc,
                                  "ray_march_iterator_ispc",
                                  vec2i(1024),
                                  512);

  // wavelet unstructured
  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  density_pathtracer / 128 / scalar,
                                  "density_pathtracer",
                                  vec2i(1024),
                                  128);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  density_pathtracer / 128 / ispc,
                                  "density_pathtracer_ispc",
                                  vec2i(1024),
                                  128);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  hit_iterator / 128 / scalar,
                                  "hit_iterator",
                                  vec2i(1024),
                                  128);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  hit_iterator / 128 / ispc,
                                  "hit_iterator_ispc",
                                  vec2i(1024),
                                  128);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  ray_march_iterator / 128 / scalar,
                                  "ray_march_iterator",
                                  vec2i(1024),
                                  128);

  BENCHMARK_CAPTURE_IF_COMPATIBLE(render_wavelet_unstructured_hex,
                                  ray_march_iterator / 128 / ispc,
                                  "ray_march_iterator_ispc",
                                  vec2i(1024),
                                  128);

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  shutdownOpenVKL();
}
