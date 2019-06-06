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

#include "../samples_mask/SamplesMask.h"
#include "common/simd.h"
#include "volley/volley.h"

using namespace ospcommon;

namespace volley {
  namespace ispc_driver {

    template <typename U>
    inline VLYRayIterator toVLYRayIterator(U &&x)
    {
      static_assert(RAY_ITERATOR_INTERNAL_STATE_SIZE >= sizeof(U),
                    "RAY_ITERATOR_INTERNAL_STATE_SIZE must be >= "
                    "source object size");
      VLYRayIterator result{(VLYVolume)x.volume};
      std::memcpy((void *)std::addressof(result.internalState),
                  (const void *)std::addressof(x),
                  sizeof(U));
      return result;
    }

    template <typename T>
    inline T fromVLYRayIterator(VLYRayIterator &x)
    {
      static_assert(sizeof(T) <= RAY_ITERATOR_INTERNAL_STATE_SIZE,
                    "fromVLYRayIterator destination object size must be <= "
                    "RAY_ITERATOR_INTERNAL_STATE_SIZE");
      T result;
      std::memcpy((void *)std::addressof(result),
                  (const void *)std::addressof(x.internalState),
                  sizeof(T));
      return result;
    }

    template <int W>
    struct Volume;

    template <int W>
    struct RayInterval
    {
      vrange1fn<W> tRange;
      vfloatn<W> nominalDeltaT;
    };

    template <int W>
    struct SurfaceHit
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

      virtual const RayInterval<W> *getCurrentRayInterval() const      = 0;
      virtual void iterateInterval(const int *valid, vintn<W> &result) = 0;

      virtual const SurfaceHit<W> *getCurrentSurfaceHit() const       = 0;
      virtual void iterateSurface(const int *valid, vintn<W> &result) = 0;

      const Volume<W> *volume;
    };

  }  // namespace ispc_driver
}  // namespace volley
