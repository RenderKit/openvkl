// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include "openvkl/openvkl.h"

#include "../include/openvkl/device/openvkl.h"

namespace openvkl {

  // Public wide types for given widths
  template <int W>
  struct vklDeviceTypes
  {
    using VKLIntervalIteratorW = void;
    using VKLIntervalW         = void;
    using VKLHitIteratorW      = void;
    using VKLHitW              = void;

    using vklGetHitIteratorSizeWType = void;
    using vklInitHitIteratorWType    = void;
    using vklIterateHitWType         = void;
  };

  template <>
  struct vklDeviceTypes<1>
  {
    using VKLIntervalIteratorW = VKLIntervalIterator;
    using VKLIntervalW         = VKLInterval;
    using VKLHitIteratorW      = VKLHitIterator;
    using VKLHitW              = VKLHit;

    using vklGetHitIteratorSizeWType =
        std::function<decltype(vklGetHitIteratorSize)>;
    using vklInitHitIteratorWType = std::function<decltype(vklInitHitIterator)>;
    using vklIterateHitWType      = std::function<decltype(vklIterateHit)>;

    const vklGetHitIteratorSizeWType vklGetHitIteratorSizeW =
        vklGetHitIteratorSize;
    const vklInitHitIteratorWType vklInitHitIteratorW = vklInitHitIterator;
    const vklIterateHitWType vklIterateHitW           = vklIterateHit;
  };

  template <>
  struct vklDeviceTypes<4>
  {
    using VKLIntervalIteratorW = VKLIntervalIterator4;
    using VKLIntervalW         = VKLInterval4;
    using VKLHitIteratorW      = VKLHitIterator4;
    using VKLHitW              = VKLHit4;

    using vklGetHitIteratorSizeWType =
        std::function<decltype(vklGetHitIteratorSize4)>;
    using vklInitHitIteratorWType =
        std::function<decltype(vklInitHitIterator4)>;
    using vklIterateHitWType = std::function<decltype(vklIterateHit4)>;

    const vklGetHitIteratorSizeWType vklGetHitIteratorSizeW =
        vklGetHitIteratorSize4;
    const vklInitHitIteratorWType vklInitHitIteratorW = vklInitHitIterator4;
    const vklIterateHitWType vklIterateHitW           = vklIterateHit4;
  };

  template <>
  struct vklDeviceTypes<8>
  {
    using VKLIntervalIteratorW = VKLIntervalIterator8;
    using VKLIntervalW         = VKLInterval8;
    using VKLHitIteratorW      = VKLHitIterator8;
    using VKLHitW              = VKLHit8;

    using vklGetHitIteratorSizeWType =
        std::function<decltype(vklGetHitIteratorSize8)>;
    using vklInitHitIteratorWType =
        std::function<decltype(vklInitHitIterator8)>;
    using vklIterateHitWType = std::function<decltype(vklIterateHit8)>;

    const vklGetHitIteratorSizeWType vklGetHitIteratorSizeW =
        vklGetHitIteratorSize8;
    const vklInitHitIteratorWType vklInitHitIteratorW = vklInitHitIterator8;
    const vklIterateHitWType vklIterateHitW           = vklIterateHit8;
  };

  template <>
  struct vklDeviceTypes<16>
  {
    using VKLIntervalIteratorW = VKLIntervalIterator16;
    using VKLIntervalW         = VKLInterval16;
    using VKLHitIteratorW      = VKLHitIterator16;
    using VKLHitW              = VKLHit16;

    using vklGetHitIteratorSizeWType =
        std::function<decltype(vklGetHitIteratorSize16)>;
    using vklInitHitIteratorWType =
        std::function<decltype(vklInitHitIterator16)>;
    using vklIterateHitWType = std::function<decltype(vklIterateHit16)>;

    const vklGetHitIteratorSizeWType vklGetHitIteratorSizeW =
        vklGetHitIteratorSize16;
    const vklInitHitIteratorWType vklInitHitIteratorW = vklInitHitIterator16;
    const vklIterateHitWType vklIterateHitW           = vklIterateHit16;
  };

}  // namespace openvkl
