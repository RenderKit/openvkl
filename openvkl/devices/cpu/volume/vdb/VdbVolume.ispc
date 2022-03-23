// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbVolumeShared.h"
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
export void EXPORT_UNIQUE(VdbVolume_Constructor, void *uniform _self)
{
  uniform VdbVolume *uniform self = (uniform VdbVolume * uniform) _self;
  memset(self, 0, sizeof(uniform VdbVolume));
}

/*
 * Destructor for ISPC versions of the volume.
 */
export void EXPORT_UNIQUE(VdbVolume_destroy, void *uniform _self)
{
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
