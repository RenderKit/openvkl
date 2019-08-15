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

#include "../common/simd.h"
#include "../samples_mask/SamplesMask.h"
#include "openvkl/openvkl.h"

using namespace ospcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W, typename U>
    inline vVKLIntervalIteratorN<W> toVKLIntervalIterator(U &&x)
    {
      static_assert(ray_iterator_internal_state_size_for_width(W) >= sizeof(U),
                    "ray iterator internal state size must be >= "
                    "source object size");
      vVKLIntervalIteratorN<W> result;
      std::memcpy((void *)std::addressof(result.internalState),
                  (const void *)std::addressof(x),
                  sizeof(U));
      result.volume = (VKLVolume)x.volume;
      return result;
    }

    template <typename T, int W>
    inline T *fromVKLIntervalIterator(vVKLIntervalIteratorN<W> *x)
    {
      static_assert(
          alignof(T) == alignof(vVKLIntervalIteratorN<W>),
          "alignment of destination type must be == alignment of source type");
      static_assert(
          sizeof(T) <= ray_iterator_internal_state_size_for_width(W),
          "fromVKLIntervalIterator destination object size must be <= "
          "ray iterator internal state size");
      return reinterpret_cast<T *>(&x->internalState[0]);
    }

    template <int W, typename U>
    inline vVKLHitIteratorN<W> toVKLHitIterator(U &&x)
    {
      static_assert(ray_iterator_internal_state_size_for_width(W) >= sizeof(U),
                    "ray iterator internal state size must be >= "
                    "source object size");
      vVKLHitIteratorN<W> result;
      std::memcpy((void *)std::addressof(result.internalState),
                  (const void *)std::addressof(x),
                  sizeof(U));
      result.volume = (VKLVolume)x.volume;
      return result;
    }

    template <typename T, int W>
    inline T *fromVKLHitIterator(vVKLHitIteratorN<W> *x)
    {
      static_assert(
          alignof(T) == alignof(vVKLHitIteratorN<W>),
          "alignment of destination type must be == alignment of source type");
      static_assert(sizeof(T) <= ray_iterator_internal_state_size_for_width(W),
                    "fromVKLHitIterator destination object size must be <= "
                    "ray iterator internal state size");
      return reinterpret_cast<T *>(&x->internalState[0]);
    }

    template <int W>
    struct Volume;

    template <int W>
    struct Interval
    {
      vrange1fn<W> tRange;
      vfloatn<W> nominalDeltaT;
    };

    template <int W>
    struct Hit
    {
      vfloatn<W> t;
      vfloatn<W> sample;
    };

    template <int W>
    struct RayIterator
    {
      RayIterator() {}

      RayIterator(const Volume<W> *volume,
                  const vvec3fn<W> &origin,
                  const vvec3fn<W> &direction,
                  const vrange1fn<W> &tRange,
                  const SamplesMask *samplesMask)
          : volume(volume)
      {
      }

      virtual ~RayIterator() = default;

      virtual const Interval<W> *getCurrentInterval() const            = 0;
      virtual void iterateInterval(const int *valid, vintn<W> &result) = 0;

      virtual const Hit<W> *getCurrentHit() const                 = 0;
      virtual void iterateHit(const int *valid, vintn<W> &result) = 0;

      const Volume<W> *volume;
    };

  }  // namespace ispc_driver
}  // namespace openvkl
