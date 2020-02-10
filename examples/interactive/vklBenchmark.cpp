// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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

static void render_wavelet(benchmark::State &state,
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
  ss << "render_wavelet_" << rendererType << "_" << ppmCounter << ".ppm";
  window->savePPM(ss.str());
  ppmCounter++;
}

BENCHMARK_CAPTURE(render_wavelet,
                  density_pathtracer / 512 / scalar,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet,
                  density_pathtracer / 512 / ispc,
                  "density_pathtracer",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet,
                  hit_iterator / 512 / scalar,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet,
                  hit_iterator / 512 / ispc,
                  "hit_iterator",
                  vec2i(1024),
                  512,
                  true);

BENCHMARK_CAPTURE(render_wavelet,
                  ray_march_iterator / 512 / scalar,
                  "ray_march_iterator",
                  vec2i(1024),
                  512,
                  false);

BENCHMARK_CAPTURE(render_wavelet,
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
