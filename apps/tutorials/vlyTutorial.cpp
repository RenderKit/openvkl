// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include <iostream>
#include <vector>
#include "volley/volley.h"

int main()
{
  vlyLoadModule("scalar_driver");

  VLYDriver driver = vlyNewDriver("scalar_driver");
  vlyCommitDriver(driver);
  vlySetCurrentDriver(driver);

  VLYVolume volume = vlyNewVolume("simple_procedural_volume");
  vlyCommit(volume);

  const size_t numValues = 10;

  std::vector<vly_vec3f> worldCoordinates;

  for (size_t i = 0; i < numValues; i++) {
    worldCoordinates.push_back(vly_vec3f{float(i), float(i), float(i)});
  }

  std::vector<float> results;
  results.resize(numValues);

  vlySampleVolume(
      volume, VLY_SAMPLE_LINEAR, numValues, worldCoordinates.data(), results.data());

  for (const auto &result : results) {
    std::cout << result << std::endl;
  }

  return 0;
}
