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

#include "VdbVolume.ih"
#include "common/export_util.h"

/*
 * This is a dummy export to make sure ISPC generates code
 * for the parameter type.
 */
export void EXPORT_UNIQUE(VdbVolume__declare_types_dummy,
                          uniform vec3i &dummy_vec3i,
                          uniform range1f &dummy_range1f)
{
}

export void EXPORT_UNIQUE(VdbSampler_computeSample,
                          uniform const int *uniform imask,
                          const void *uniform _volume,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples);

/*
 * Factory for ISPC versions of the volume.
 */
export void *uniform EXPORT_UNIQUE(VdbVolume_create)
{
  VdbVolume *uniform self = uniform new uniform VdbVolume;
  return self;
}

/* This is here for the default iterator. */
varying float VdbVolume_sample(const void *uniform volume,
                               const varying vec3f &objectCoordinates)
{
  float samples  = 0.f;
  const int mask = __mask;
  CALL_ISPC(VdbSampler_computeSample,
            (uniform const int *uniform) & mask,
            volume,
            (const void *uniform) & objectCoordinates,
            (void *uniform) & samples);
  return samples;
}

/*
 * Initialize the volume data structure.
 */
export void EXPORT_UNIQUE(VdbVolume_setGrid,
                          void *uniform _self,
                          const VdbGrid *uniform grid)
{
  VdbVolume *uniform volume           = (VdbVolume * uniform) _self;
  volume->grid                        = grid;
  volume->super.computeSample_varying = VdbVolume_sample;
}
