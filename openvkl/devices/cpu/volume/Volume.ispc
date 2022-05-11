// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "Volume.ih"

export void EXPORT_UNIQUE(Volume_setBackground,
                          void *uniform _volume,
                          const float *uniform background)
{
  Volume *uniform volume = (Volume * uniform) _volume;
  volume->background     = background;
}
