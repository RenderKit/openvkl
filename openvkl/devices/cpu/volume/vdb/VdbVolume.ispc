// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

/*
 * Factory for ISPC versions of the volume.
 */
export void *uniform EXPORT_UNIQUE(VdbVolume_create)
{
  VdbVolume *uniform self = uniform new uniform VdbVolume;
  memset(self, 0, sizeof(uniform VdbVolume));
  return self;
}

/*
 * Destructor for ISPC versions of the volume.
 */
export void EXPORT_UNIQUE(VdbVolume_destroy, void *uniform _self)
{
  VdbVolume *uniform volume = (VdbVolume * uniform) _self;
  delete volume;
}

/*
 * Initialize the volume data structure.
 */
export void EXPORT_UNIQUE(VdbVolume_setGrid,
                          void *uniform _self,
                          const VdbGrid *uniform grid)
{
  assert(_self);
  VdbVolume *uniform volume = (VdbVolume * uniform) _self;
  volume->grid              = grid;
}
