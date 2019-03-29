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

#include "../volume/GridAccelerator.ih"
#include "GridAcceleratorSamplesMask.ih"

export void *uniform
GridAcceleratorSamplesMask_Constructor(void *uniform _volume,
                                       const uniform int &numRanges,
                                       const box1f *uniform ranges)
{
  uniform GridAcceleratorSamplesMask *uniform self =
      uniform new uniform GridAcceleratorSamplesMask;

  self->numRanges = numRanges;
  self->ranges    = uniform new uniform box1f[numRanges];

  foreach (i = 0 ... numRanges) {
    self->ranges[i] = ranges[i];
  }

  return self;
}

export void *uniform GridAcceleratorSamplesMask_Destructor(void *uniform _self)
{
  uniform GridAcceleratorSamplesMask *uniform self =
      (uniform GridAcceleratorSamplesMask * uniform) _self;
  delete[] self->ranges;
  delete self;
}
