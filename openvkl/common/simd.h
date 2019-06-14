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

#include "openvkl/openvkl.h"
#include "ospray/ospcommon/platform.h"

namespace openvkl {

  /////////////////////////////////////////////////////////////////////////////
  // any new types here should have corresponding SIMD conformance tests added!
  /////////////////////////////////////////////////////////////////////////////

  template <class T>
  bool is_aligned_for_type(const void *ptr) noexcept
  {
    auto iptr = reinterpret_cast<std::uintptr_t>(ptr);
    return !(iptr % alignof(T));
  }

  constexpr int simd_alignment_for_width(int W)
  {
    return W < 4 ? 0 : (W < 8 ? 16 : (W < 16 ? 32 : 64));
  }

  constexpr int ray_iterator_internal_state_size_for_width(int W)
  {
    return W < 4 ? RAY_ITERATOR_INTERNAL_STATE_SIZE
                 : (W < 8 ? RAY_ITERATOR_INTERNAL_STATE_SIZE_4
                          : (W < 16 ? RAY_ITERATOR_INTERNAL_STATE_SIZE_8
                                    : RAY_ITERATOR_INTERNAL_STATE_SIZE_16));
  }

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vfloatn
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
  struct alignas(simd_alignment_for_width(W)) vintn
  {
    int v[W];

    vintn<W>() = default;

    int &operator[](std::size_t index)
    {
      return v[index];
    }

    const int &operator[](std::size_t index) const
    {
      return v[index];
    }

    vintn<W>(const vintn<W> &o)
    {
      for (int i = 0; i < W; i++) {
        v[i] = o[i];
      }
    }

    operator const int *() const
    {
      return &v[0];
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vrange1fn
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
  struct alignas(simd_alignment_for_width(W)) vvec3fn
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
  struct alignas(simd_alignment_for_width(W)) vVKLRayIteratorN
  {
    alignas(simd_alignment_for_width(
        W)) char internalState[ray_iterator_internal_state_size_for_width(W)];
    VKLVolume volume;

    vVKLRayIteratorN<W>() = default;

    vVKLRayIteratorN<W>(const vVKLRayIteratorN<W> &v) : volume(v.volume)
    {
      memcpy(internalState,
             v.internalState,
             ray_iterator_internal_state_size_for_width(W));
    }

    // vVKLRayIteratorN<1> is maximally sized, so can hold internal state for
    // any other ray iterator width; this is to support execution of the scalar
    // APIs through the native vector-wide implementation. therefore we allow
    // conversion between width=1 and any other width.
    //
    // any width => scalar conversion
    explicit operator vVKLRayIteratorN<1>()
    {
      static_assert(
          ray_iterator_internal_state_size_for_width(1) >=
              ray_iterator_internal_state_size_for_width(W),
          "vVKLRayIteratorN<1> is not sufficiently sized to hold wider type");

      vVKLRayIteratorN<1> rayIterator1;
      memcpy(rayIterator1.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W));
      rayIterator1.volume = volume;
      return rayIterator1;
    }

    // scalar (width 1) => any width conversion
    template <int W2, typename = std::enable_if<(W == 1 && W2 != W)>>
    explicit operator vVKLRayIteratorN<W2>()
    {
      static_assert(ray_iterator_internal_state_size_for_width(W2) <=
                        ray_iterator_internal_state_size_for_width(W),
                    "vVKLRayIteratorN<W2> is larger than source type");

      vVKLRayIteratorN<W2> rayIteratorW2;
      memcpy(rayIteratorW2.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W2));
      rayIteratorW2.volume = volume;
      return rayIteratorW2;
    }

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    explicit operator VKLRayIterator()
    {
      VKLRayIterator rayIterator1;
      memcpy(rayIterator1.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W));
      rayIterator1.volume = volume;
      return rayIterator1;
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    explicit operator VKLRayIterator4()
    {
      VKLRayIterator4 rayIterator4;
      memcpy(rayIterator4.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W));
      rayIterator4.volume = volume;
      return rayIterator4;
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    explicit operator VKLRayIterator8()
    {
      VKLRayIterator8 rayIterator8;
      memcpy(rayIterator8.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W));
      rayIterator8.volume = volume;
      return rayIterator8;
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    explicit operator VKLRayIterator16()
    {
      VKLRayIterator16 rayIterator16;
      memcpy(rayIterator16.internalState,
             internalState,
             ray_iterator_internal_state_size_for_width(W));
      rayIterator16.volume = volume;
      return rayIterator16;
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLRayIntervalN
  {
    vrange1fn<W> tRange;
    vfloatn<W> nominalDeltaT;

    vVKLRayIntervalN<W>() = default;

    vVKLRayIntervalN<W>(const vVKLRayIntervalN<W> &v)
        : tRange(v.tRange), nominalDeltaT(v.nominalDeltaT)
    {
    }

    template <int W2 = W, typename = std::enable_if<(W >= 1)>>
    explicit operator VKLRayInterval()
    {
      VKLRayInterval rayInterval1;

      rayInterval1.tRange.lower  = tRange.lower[0];
      rayInterval1.tRange.upper  = tRange.upper[0];
      rayInterval1.nominalDeltaT = nominalDeltaT[0];

      return rayInterval1;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 4)>>
    explicit operator VKLRayInterval4()
    {
      VKLRayInterval4 rayInterval4;

      for (int i = 0; i < 4; i++) {
        rayInterval4.tRange.lower[i]  = tRange.lower[i];
        rayInterval4.tRange.upper[i]  = tRange.upper[i];
        rayInterval4.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval4;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 8)>>
    explicit operator VKLRayInterval8()
    {
      VKLRayInterval8 rayInterval8;

      for (int i = 0; i < 8; i++) {
        rayInterval8.tRange.lower[i]  = tRange.lower[i];
        rayInterval8.tRange.upper[i]  = tRange.upper[i];
        rayInterval8.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval8;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 16)>>
    explicit operator VKLRayInterval16()
    {
      VKLRayInterval16 rayInterval16;

      for (int i = 0; i < 16; i++) {
        rayInterval16.tRange.lower[i]  = tRange.lower[i];
        rayInterval16.tRange.upper[i]  = tRange.upper[i];
        rayInterval16.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return rayInterval16;
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLSurfaceHitN
  {
    vfloatn<W> t;
    vfloatn<W> sample;

    vVKLSurfaceHitN<W>() = default;

    vVKLSurfaceHitN<W>(const vVKLSurfaceHitN<W> &v) : t(v.t), sample(v.sample)
    {
    }

    template <int W2 = W, typename = std::enable_if<(W >= 1)>>
    explicit operator VKLSurfaceHit()
    {
      VKLSurfaceHit surfaceHit1;

      surfaceHit1.t      = t[0];
      surfaceHit1.sample = sample[0];

      return surfaceHit1;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 4)>>
    explicit operator VKLSurfaceHit4()
    {
      VKLSurfaceHit4 surfaceHit4;

      for (int i = 0; i < 4; i++) {
        surfaceHit4.t[i]      = t[i];
        surfaceHit4.sample[i] = sample[i];
      }

      return surfaceHit4;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 8)>>
    explicit operator VKLSurfaceHit8()
    {
      VKLSurfaceHit8 surfaceHit8;

      for (int i = 0; i < 8; i++) {
        surfaceHit8.t[i]      = t[i];
        surfaceHit8.sample[i] = sample[i];
      }

      return surfaceHit8;
    }

    template <int W2 = W, typename = std::enable_if<(W >= 16)>>
    explicit operator VKLSurfaceHit16()
    {
      VKLSurfaceHit16 surfaceHit16;

      for (int i = 0; i < 16; i++) {
        surfaceHit16.t[i]      = t[i];
        surfaceHit16.sample[i] = sample[i];
      }

      return surfaceHit16;
    }
  };

}  // namespace openvkl
