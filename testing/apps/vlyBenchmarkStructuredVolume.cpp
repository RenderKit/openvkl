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
#include "volley_testing.h"

using namespace volley::testing;

class VolleyFixture : public benchmark::Fixture
{
 public:
  void SetUp(const ::benchmark::State &state)
  {
    vlyLoadModule("ispc_driver");

    VLYDriver driver = vlyNewDriver("ispc_driver");
    vlyCommitDriver(driver);
    vlySetCurrentDriver(driver);
  }
};

BENCHMARK_F(VolleyFixture, scalarRandomSample)(benchmark::State &state)
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

    float sample =
        vlyComputeSample(vlyVolume, (const vly_vec3f *)&objectCoordinates);
  }
}

BENCHMARK_MAIN();
