// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Traits.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/platform.h"

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

    operator int *()
    {
      return v;
    }

    operator const int *() const
    {
      return v;
    }

    vintn<W>(const vintn<W> &o)
    {
      for (int i = 0; i < W; i++) {
        v[i] = o[i];
      }
    }
  };

  template <int W>
  struct alignas(simd_alignment_for_width(W)) vfloatn
  {
    float v[W];

    vfloatn<W>() = default;

    vfloatn<W>(float _v)
    {
      static_assert(W == 1, "scalar constructor only valid for W=1");
      v[0] = _v;
    }

    float &operator[](std::size_t index)
    {
      return v[index];
    }

    const float &operator[](std::size_t index) const
    {
      return v[index];
    }

    operator float *()
    {
      return v;
    }

    operator const float *() const
    {
      return v;
    }

    vfloatn<W>(const vfloatn<W> &o)
    {
      for (int i = 0; i < W; i++) {
        v[i] = o[i];
      }
    }

    vfloatn<W>(const float *o, int oW)
    {
      assert(oW <= W);
      for (int i = 0; i < W; i++) {
        v[i] = o == nullptr ? 0.f : (i < oW ? o[i] : 0.f);
      }
    }

    template <int OW>
    explicit operator vfloatn<OW>() const
    {
      static_assert(W <= OW, "can only up-convert vfloatn types");

      vfloatn<OW> newVec;

      for (int i = 0; i < W; i++) {
        newVec.v[i] = v[i];
      }

      return newVec;
    }

    template <int OW>
    vfloatn<OW> extract_pack(int packIndex) const
    {
      vfloatn<OW> newVec;

      for (int i = packIndex * OW; i < (packIndex + 1) * OW && i < W; i++) {
        newVec.v[i - packIndex * OW] = v[i];
      }

      return newVec;
    }

    void fill_inactive_lanes(const vintn<W> &mask)
    {
      for (int i = 0; i < W; i++) {
        if (mask[i]) {
          for (int k = 0; k < W; k++) {
            if (!mask[k]) {
              v[k] = v[i];
            }
          }
          break;
        }
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

    vvec3fn<W>(const vfloatn<W> &x, const vfloatn<W> &y, const vfloatn<W> &z)
        : x(x), y(y), z(z)
    {
    }

    vvec3fn<W>(const rkcommon::math::vec3f &v)
    {
      static_assert(W == 1, "vec3f constructor only valid for W=1");
      x[0] = v.x;
      y[0] = v.y;
      z[0] = v.z;
    }

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

}  // namespace openvkl
