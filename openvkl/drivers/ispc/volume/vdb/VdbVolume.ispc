// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbSampleConfig.h"
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
                          const void *uniform _config,
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

/*
 * Destructor for ISPC versions of the volume.
 */
export void EXPORT_UNIQUE(VdbVolume_destroy,
                          void *uniform _self)
{
  VdbVolume *uniform volume = (VdbVolume * uniform) _self;
  delete volume;
}

/* This is here for the default iterator. */
varying float VdbVolume_sample(const void *uniform _volume,
                               const varying vec3f &objectCoordinates)
{
  float samples                   = 0.f;
  const int mask                  = __mask;
  const VdbVolume *uniform volume = (const VdbVolume *uniform)_volume;
  CALL_ISPC(VdbSampler_computeSample,
            (uniform const int *uniform) & mask,
            volume->grid,
            volume->globalConfig,
            (const void *uniform) & objectCoordinates,
            (void *uniform) & samples);
  return samples;
}

/*
 * Initialize the volume data structure.
 */
export void EXPORT_UNIQUE(VdbVolume_setGrid,
                          void *uniform _self,
                          const VdbGrid *uniform grid,
                          const VdbSampleConfig *uniform globalConfig)
{
  VdbVolume *uniform volume           = (VdbVolume * uniform) _self;
  volume->grid                        = grid;
  volume->globalConfig                = globalConfig;
  volume->super.computeSample_varying = VdbVolume_sample;
}
