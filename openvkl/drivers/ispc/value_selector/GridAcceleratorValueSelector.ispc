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
#include "GridAcceleratorValueSelector.ih"

export void *uniform
GridAcceleratorValueSelector_Constructor(void *uniform _volume,
                                         const uniform int &numRanges,
                                         const box1f *uniform ranges,
                                         const uniform int &numValues,
                                         const float *uniform values)
{
  uniform GridAcceleratorValueSelector *uniform self =
      uniform new uniform GridAcceleratorValueSelector;

  self->numRanges = numRanges;
  self->ranges    = uniform new uniform box1f[numRanges];

  foreach (i = 0 ... numRanges) {
    self->ranges[i] = ranges[i];
  }

  self->rangesMinMax = make_box1f(inf, -inf);

  foreach (i = 0... numRanges) {
    self->rangesMinMax.lower =
        min(self->rangesMinMax.lower, reduce_min(ranges[i].lower));
    self->rangesMinMax.upper =
        max(self->rangesMinMax.upper, reduce_max(ranges[i].upper));
  }

  self->numValues = numValues;
  self->values    = uniform new uniform float[numValues];

  foreach (i = 0 ... numValues) {
    self->values[i] = values[i];
  }

  self->valuesMinMax = make_box1f(inf, -inf);

  foreach (i = 0... numValues) {
    self->valuesMinMax.lower =
        min(self->valuesMinMax.lower, reduce_min(values[i]));
    self->valuesMinMax.upper =
        max(self->valuesMinMax.upper, reduce_max(values[i]));
  }

  return self;
}

export void *uniform
GridAcceleratorValueSelector_Destructor(void *uniform _self)
{
  uniform GridAcceleratorValueSelector *uniform self =
      (uniform GridAcceleratorValueSelector * uniform) _self;
  delete[] self->ranges;
  delete[] self->values;
  delete self;
}
