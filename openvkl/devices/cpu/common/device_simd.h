// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../../common/simd.h"
#include "DeviceTraits.h"

namespace openvkl {

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLIntervalN
  {
    vrange1fn<W> tRange;
    vrange1fn<W> valueRange;
    vfloatn<W> nominalDeltaT;

    vVKLIntervalN<W>()
    {
      using VKLIntervalW = typename vklDeviceTypes<W>::VKLIntervalW;
      static_assert(sizeof(vVKLIntervalN<W>) == sizeof(VKLIntervalW),
                    "incompatible with corresponding public wide type");
      static_assert(alignof(vVKLIntervalN<W>) == alignof(VKLIntervalW),
                    "incompatible with corresponding public wide type");
    }

    vVKLIntervalN<W>(const vVKLIntervalN<W> &v)
        : tRange(v.tRange),
          valueRange(v.valueRange),
          nominalDeltaT(v.nominalDeltaT)
    {
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLHitN
  {
    vfloatn<W> t;
    vfloatn<W> sample;
    vfloatn<W> epsilon;

    vVKLHitN<W>()
    {
      using VKLHitW = typename vklDeviceTypes<W>::VKLHitW;
      static_assert(sizeof(vVKLHitN<W>) == sizeof(VKLHitW),
                    "incompatible with corresponding public wide type");
      static_assert(alignof(vVKLHitN<W>) == alignof(VKLHitW),
                    "incompatible with corresponding public wide type");
    }

    vVKLHitN<W>(const vVKLHitN<W> &v) : t(v.t), sample(v.sample) {}
  };

}  // namespace openvkl
