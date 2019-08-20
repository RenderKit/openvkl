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
#include "ospcommon/platform.h"

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

  constexpr int iterator_internal_state_size_for_width(int W)
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
  struct alignas(simd_alignment_for_width(W)) vVKLIntervalIteratorN
  {
    alignas(simd_alignment_for_width(
        W)) char internalState[iterator_internal_state_size_for_width(W)];
    VKLVolume volume;

    vVKLIntervalIteratorN<W>() = default;

    vVKLIntervalIteratorN<W>(const vVKLIntervalIteratorN<W> &v)
        : volume(v.volume)
    {
      memcpy(internalState,
             v.internalState,
             iterator_internal_state_size_for_width(W));
    }

    // vVKLIntervalIteratorN<1> is maximally sized, so can hold internal state
    // for any other iterator width; this is to support execution of the
    // scalar APIs through the native vector-wide implementation. therefore we
    // allow conversion between width=1 and any other width.
    //
    // any width => scalar conversion
    explicit operator vVKLIntervalIteratorN<1>()
    {
      static_assert(iterator_internal_state_size_for_width(1) >=
                        iterator_internal_state_size_for_width(W),
                    "vVKLIntervalIteratorN<1> is not sufficiently sized to "
                    "hold wider type");

      vVKLIntervalIteratorN<1> iterator1;
      memcpy(iterator1.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator1.volume = volume;
      return iterator1;
    }

    // scalar (width 1) => any width conversion
    template <int W2, typename = std::enable_if<(W == 1 && W2 != W)>>
    explicit operator vVKLIntervalIteratorN<W2>()
    {
      static_assert(iterator_internal_state_size_for_width(W2) <=
                        iterator_internal_state_size_for_width(W),
                    "vVKLIntervalIteratorN<W2> is larger than source type");

      vVKLIntervalIteratorN<W2> iteratorW2;
      memcpy(iteratorW2.internalState,
             internalState,
             iterator_internal_state_size_for_width(W2));
      iteratorW2.volume = volume;
      return iteratorW2;
    }

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    explicit operator VKLIntervalIterator()
    {
      VKLIntervalIterator iterator1;
      memcpy(iterator1.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator1.volume = volume;
      return iterator1;
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    explicit operator VKLIntervalIterator4()
    {
      VKLIntervalIterator4 iterator4;
      memcpy(iterator4.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator4.volume = volume;
      return iterator4;
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    explicit operator VKLIntervalIterator8()
    {
      VKLIntervalIterator8 iterator8;
      memcpy(iterator8.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator8.volume = volume;
      return iterator8;
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    explicit operator VKLIntervalIterator16()
    {
      VKLIntervalIterator16 iterator16;
      memcpy(iterator16.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator16.volume = volume;
      return iterator16;
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLIntervalN
  {
    vrange1fn<W> tRange;
    vfloatn<W> nominalDeltaT;

    vVKLIntervalN<W>() = default;

    vVKLIntervalN<W>(const vVKLIntervalN<W> &v)
        : tRange(v.tRange), nominalDeltaT(v.nominalDeltaT)
    {
    }

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    explicit operator VKLInterval()
    {
      VKLInterval interval1;

      interval1.tRange.lower  = tRange.lower[0];
      interval1.tRange.upper  = tRange.upper[0];
      interval1.nominalDeltaT = nominalDeltaT[0];

      return interval1;
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    explicit operator VKLInterval4()
    {
      VKLInterval4 interval4;

      for (int i = 0; i < 4; i++) {
        interval4.tRange.lower[i]  = tRange.lower[i];
        interval4.tRange.upper[i]  = tRange.upper[i];
        interval4.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return interval4;
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    explicit operator VKLInterval8()
    {
      VKLInterval8 interval8;

      for (int i = 0; i < 8; i++) {
        interval8.tRange.lower[i]  = tRange.lower[i];
        interval8.tRange.upper[i]  = tRange.upper[i];
        interval8.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return interval8;
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    explicit operator VKLInterval16()
    {
      VKLInterval16 interval16;

      for (int i = 0; i < 16; i++) {
        interval16.tRange.lower[i]  = tRange.lower[i];
        interval16.tRange.upper[i]  = tRange.upper[i];
        interval16.nominalDeltaT[i] = nominalDeltaT[i];
      }

      return interval16;
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLHitIteratorN
  {
    alignas(simd_alignment_for_width(
        W)) char internalState[iterator_internal_state_size_for_width(W)];
    VKLVolume volume;

    vVKLHitIteratorN<W>() = default;

    vVKLHitIteratorN<W>(const vVKLHitIteratorN<W> &v) : volume(v.volume)
    {
      memcpy(internalState,
             v.internalState,
             iterator_internal_state_size_for_width(W));
    }

    // vVKLHitIteratorN<1> is maximally sized, so can hold internal state for
    // any other iterator width; this is to support execution of the scalar
    // APIs through the native vector-wide implementation. therefore we allow
    // conversion between width=1 and any other width.
    //
    // any width => scalar conversion
    explicit operator vVKLHitIteratorN<1>()
    {
      static_assert(
          iterator_internal_state_size_for_width(1) >=
              iterator_internal_state_size_for_width(W),
          "vVKLHitIteratorN<1> is not sufficiently sized to hold wider type");

      vVKLHitIteratorN<1> iterator1;
      memcpy(iterator1.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator1.volume = volume;
      return iterator1;
    }

    // scalar (width 1) => any width conversion
    template <int W2, typename = std::enable_if<(W == 1 && W2 != W)>>
    explicit operator vVKLHitIteratorN<W2>()
    {
      static_assert(iterator_internal_state_size_for_width(W2) <=
                        iterator_internal_state_size_for_width(W),
                    "vVKLHitIteratorN<W2> is larger than source type");

      vVKLHitIteratorN<W2> iteratorW2;
      memcpy(iteratorW2.internalState,
             internalState,
             iterator_internal_state_size_for_width(W2));
      iteratorW2.volume = volume;
      return iteratorW2;
    }

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    explicit operator VKLHitIterator()
    {
      VKLHitIterator iterator1;
      memcpy(iterator1.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator1.volume = volume;
      return iterator1;
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    explicit operator VKLHitIterator4()
    {
      VKLHitIterator4 iterator4;
      memcpy(iterator4.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator4.volume = volume;
      return iterator4;
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    explicit operator VKLHitIterator8()
    {
      VKLHitIterator8 iterator8;
      memcpy(iterator8.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator8.volume = volume;
      return iterator8;
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    explicit operator VKLHitIterator16()
    {
      VKLHitIterator16 iterator16;
      memcpy(iterator16.internalState,
             internalState,
             iterator_internal_state_size_for_width(W));
      iterator16.volume = volume;
      return iterator16;
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vVKLHitN
  {
    vfloatn<W> t;
    vfloatn<W> sample;

    vVKLHitN<W>() = default;

    vVKLHitN<W>(const vVKLHitN<W> &v) : t(v.t), sample(v.sample) {}

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    explicit operator VKLHit()
    {
      VKLHit hit1;

      hit1.t      = t[0];
      hit1.sample = sample[0];

      return hit1;
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    explicit operator VKLHit4()
    {
      VKLHit4 hit4;

      for (int i = 0; i < 4; i++) {
        hit4.t[i]      = t[i];
        hit4.sample[i] = sample[i];
      }

      return hit4;
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    explicit operator VKLHit8()
    {
      VKLHit8 hit8;

      for (int i = 0; i < 8; i++) {
        hit8.t[i]      = t[i];
        hit8.sample[i] = sample[i];
      }

      return hit8;
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    explicit operator VKLHit16()
    {
      VKLHit16 hit16;

      for (int i = 0; i < 16; i++) {
        hit16.t[i]      = t[i];
        hit16.sample[i] = sample[i];
      }

      return hit16;
    }
  };

}  // namespace openvkl
