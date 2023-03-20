// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

/*
 * Version for uniform IC and TIME.
 *
 * This parallelizes over the stencil, so BODY sees coord and tgtIdx as
 * varying quantities!
 */
#define __vkl_stencil_dispatch_uniform(FILTER, IC, TIME, BODY)           \
  {                                                                      \
    const vec3i offset[] = VKL_STENCIL_##FILTER##_OFFSETS;               \
    const size_t N       = VKL_STENCIL_##FILTER##_SIZE;                  \
                                                                         \
    {                                                                    \
      const float TIME##Disp = TIME;                                     \
      for (uint32_t o = 0; o < N; o++) {                                 \
        const vec3i IC##Disp = make_vec3i(                               \
            IC.x + offset[o].x, IC.y + offset[o].y, IC.z + offset[o].z); \
        const uint32 tgtIdx = o;                                         \
        BODY                                                             \
      }                                                                  \
    }                                                                    \
  }
