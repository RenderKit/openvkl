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

#include <openvkl/openvkl.isph>

export void sample_ispc(uniform VKLVolume volume)
{
  vkl_vec3f objectCoordinates;
  objectCoordinates.x = programIndex * 3 + 0;
  objectCoordinates.y = programIndex * 3 + 1;
  objectCoordinates.z = programIndex * 3 + 2;

  varying float sample = vklComputeSampleV(volume, &objectCoordinates);

  print("\nsamples: %\n", sample);
}
