// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "GridAcceleratorIterator.ih"

// this export ensures the GridAcceleratorIterator struct will be published in
// the _ispc.h header, so we can compute the size at compile time as needed. We
// do it separately here so we can avoid transitively exporting other structs
// (through use of forward declarations only).

// Ignore warning about exporting uniform-pointer-to-varying, as this is in
// fact legal.
#pragma ignore warning(all)
export void EXPORT_UNIQUE(GridAcceleratorIterator_export,
                          uniform vec3f &dummy_vec3f,
                          const varying GridAcceleratorIterator *uniform it)
{
}
