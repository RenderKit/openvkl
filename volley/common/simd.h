// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#pragma once

#include "ospray/ospcommon/platform.h"
#include "volley/volley.h"

namespace volley {

  template <int W>
  struct vfloatn
  {
    float v[W];

    vfloatn<W>() = default;

    float &operator[](std::size_t index)
    {
      return v[index];
    }

    const float &operator[](std::size_t index) const
    {
      return v[index];
    }

    vfloatn<W>(const vfloatn<W> &o)
    {
      for (int i = 0; i < W; i++) {
        v[i] = o[i];
      }
    }
  };

  template <int W>
  using vintn = int[W];

  template <int W>
  struct __aligned(64) vrange1fn
  {
    vfloatn<W> lower;
    vfloatn<W> upper;

    vrange1fn<W>() = default;

    vrange1fn<W>(const vrange1fn<W> &v) : lower(v.lower), upper(v.upper) {}

    template <int OW>
    explicit operator vrange1fn<OW>() const
    {
      static_assert(W <= OW, "can only up-convert vrange1fn types");

      vrange1fn<OW> newRange;

      for (int i = 0; i < W; i++) {
        newRange.lower[i] = lower[i];
        newRange.upper[i] = upper[i];
      }

      return newRange;
    }
  };

  template <int W>
  struct __aligned(64) vvec3fn
  {
    vfloatn<W> x;
    vfloatn<W> y;
    vfloatn<W> z;

    vvec3fn<W>() = default;

    vvec3fn<W>(const vvec3fn<W> &v) : x(v.x), y(v.y), z(v.z) {}

    template <int OW>
    explicit operator vvec3fn<OW>() const
    {
      static_assert(W <= OW, "can only up-convert vvec3fn types");

      vvec3fn<OW> newVec;

      for (int i = 0; i < W; i++) {
        newVec.x[i] = x[i];
        newVec.y[i] = y[i];
        newVec.z[i] = z[i];
      }

      return newVec;
    }

    template <int OW>
    vvec3fn<OW> extract_pack(int packIndex) const
    {
      vvec3fn<OW> newVec;

      for (int i = packIndex * OW; i < (packIndex + 1) * OW && i < W; i++) {
        newVec.x[i - packIndex * OW] = x[i];
        newVec.y[i - packIndex * OW] = y[i];
        newVec.z[i - packIndex * OW] = z[i];
      }

      return newVec;
    }
  };

  template <int W>
  struct vVLYRayIntervalN
  {
    vrange1fn<W> tRange;
    vfloatn<W> nominalDeltaT;

    vVLYRayIntervalN<W>() = default;

    vVLYRayIntervalN<W>(const vVLYRayIntervalN<W> &v)
        : tRange(v.tRange), nominalDeltaT(v.nominalDeltaT)
    {
    }

    template <int W2 = W, typename = std::enable_if<(W >= 4)>>
    explicit operator VLYRayInterval()
    {
      VLYRayInterval rayInterval1;

      rayInterval1.tRange.lower  = tRange.lower[0];
      rayInterval1.tRange.upper  = tRange.upper[0];
      rayInterval1.nominalDeltaT = nominalDeltaT[0];

      return rayInterval1;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 4)>>
    explicit operator VLYRayInterval4()
    {
      VLYRayInterval4 rayInterval4;

      for (int i = 0; i < 4; i++) {
        rayInterval4.tRange.lower[i]  = tRange.lower[i];
        rayInterval4.tRange.upper[i]  = tRange.upper[i];
        rayInterval4.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval4;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 8)>>
    explicit operator VLYRayInterval8()
    {
      VLYRayInterval8 rayInterval8;

      for (int i = 0; i < 8; i++) {
        rayInterval8.tRange.lower[i]  = tRange.lower[i];
        rayInterval8.tRange.upper[i]  = tRange.upper[i];
        rayInterval8.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval8;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 16)>>
    explicit operator VLYRayInterval16()
    {
      VLYRayInterval16 rayInterval16;

      for (int i = 0; i < 16; i++) {
        rayInterval16.tRange.lower[i]  = tRange.lower[i];
        rayInterval16.tRange.upper[i]  = tRange.upper[i];
        rayInterval16.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval16;
    }
  };

  template <int W>
  struct vVLYSurfaceHitN
  {
    vfloatn<W> t;
    vfloatn<W> sample;

    vVLYSurfaceHitN<W>() = default;

    vVLYSurfaceHitN<W>(const vVLYSurfaceHitN<W> &v) : t(v.t), sample(v.sample)
    {
    }
  };

}  // namespace volley
