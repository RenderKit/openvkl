// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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
    return W < 4 ? 4 : (W < 8 ? 16 : (W < 16 ? 32 : 64));
  }

  // minimum alignment is 8 if object contains any pointers
  constexpr int simd_alignment_for_width_with_ptr(int W)
  {
    return W < 4 ? 8 : (W < 8 ? 16 : (W < 16 ? 32 : 64));
  }

  constexpr int iterator_internal_state_size_for_width(int W)
  {
    return W < 4 ? ITERATOR_INTERNAL_STATE_SIZE
                 : (W < 8 ? ITERATOR_INTERNAL_STATE_SIZE_4
                          : (W < 16 ? ITERATOR_INTERNAL_STATE_SIZE_8
                                    : ITERATOR_INTERNAL_STATE_SIZE_16));
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

    void fill_inactive_lanes(const vintn<W> &mask)
    {
      for (int i = 0; i < W; i++) {
        if (mask[i]) {
          for (int k = 0; k < W; k++) {
            if (!mask[k]) {
              x[k] = x[i];
              y[k] = y[i];
              z[k] = y[i];
            }
          }
          break;
        }
      }
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width_with_ptr(W)) vVKLIntervalIteratorN
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
    vrange1fn<W> valueRange;
    vfloatn<W> nominalDeltaT;

    vVKLIntervalN<W>() = default;

    vVKLIntervalN<W>(const vVKLIntervalN<W> &v)
        : tRange(v.tRange),
          valueRange(v.valueRange),
          nominalDeltaT(v.nominalDeltaT)
    {
    }

    template <int W2 = W, typename = std::enable_if<(W == 1)>>
    void populateVKLInterval(VKLInterval &interval) const
    {
      interval.tRange.lower     = tRange.lower[0];
      interval.tRange.upper     = tRange.upper[0];
      interval.valueRange.lower = valueRange.lower[0];
      interval.valueRange.upper = valueRange.upper[0];
      interval.nominalDeltaT    = nominalDeltaT[0];
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    void populateVKLInterval4(VKLInterval4 &interval, const int *valid) const
    {
      for (int i = 0; i < 4; i++) {
        if (valid[i]) {
          interval.tRange.lower[i]     = tRange.lower[i];
          interval.tRange.upper[i]     = tRange.upper[i];
          interval.valueRange.lower[i] = valueRange.lower[i];
          interval.valueRange.upper[i] = valueRange.upper[i];
          interval.nominalDeltaT[i]    = nominalDeltaT[i];
        }
      }
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    void populateVKLInterval8(VKLInterval8 &interval, const int *valid) const
    {
      for (int i = 0; i < 8; i++) {
        if (valid[i]) {
          interval.tRange.lower[i]     = tRange.lower[i];
          interval.tRange.upper[i]     = tRange.upper[i];
          interval.valueRange.lower[i] = valueRange.lower[i];
          interval.valueRange.upper[i] = valueRange.upper[i];
          interval.nominalDeltaT[i]    = nominalDeltaT[i];
        }
      }
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    void populateVKLInterval16(VKLInterval16 &interval, const int *valid) const
    {
      for (int i = 0; i < 16; i++) {
        if (valid[i]) {
          interval.tRange.lower[i]     = tRange.lower[i];
          interval.tRange.upper[i]     = tRange.upper[i];
          interval.valueRange.lower[i] = valueRange.lower[i];
          interval.valueRange.upper[i] = valueRange.upper[i];
          interval.nominalDeltaT[i]    = nominalDeltaT[i];
        }
      }
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width_with_ptr(W)) vVKLHitIteratorN
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
    void populateVKLHit(VKLHit &hit) const
    {
      hit.t      = t[0];
      hit.sample = sample[0];
    }

    template <int W2 = W, typename = std::enable_if<(W == 4)>>
    void populateVKLHit4(VKLHit4 &hit, const int *valid) const
    {
      for (int i = 0; i < 4; i++) {
        if (valid[i]) {
          hit.t[i]      = t[i];
          hit.sample[i] = sample[i];
        }
      }
    }

    template <int W2 = W, typename = std::enable_if<(W == 8)>>
    void populateVKLHit8(VKLHit8 &hit, const int *valid) const
    {
      for (int i = 0; i < 8; i++) {
        if (valid[i]) {
          hit.t[i]      = t[i];
          hit.sample[i] = sample[i];
        }
      }
    }

    template <int W2 = W, typename = std::enable_if<(W == 16)>>
    void populateVKLHit16(VKLHit16 &hit, const int *valid) const
    {
      for (int i = 0; i < 16; i++) {
        if (valid[i]) {
          hit.t[i]      = t[i];
          hit.sample[i] = sample[i];
        }
      }
    }
  };

}  // namespace openvkl
