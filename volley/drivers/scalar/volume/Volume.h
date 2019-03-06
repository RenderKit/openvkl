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

#include <ospray/ospcommon/box.h>
#include "../iterator/DumbRayIterator.h"
#include "common/ManagedObject.h"
#include "common/objectFactory.h"
#include "volley/volley.h"

using namespace ospcommon;

namespace volley {
  namespace scalar_driver {

    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type)
      {
        return createInstanceHelper<Volume, VLY_VOLUME>(type);
      }

      virtual void commit() override
      {
        ManagedObject::commit();
      }

      // volumes can provide their own ray iterators based on their internal
      // acceleration structures. the default "dumb" ray iterator assumes no
      // acceleration structure.
      virtual RayIterator *newRayIterator(const vec3f &origin,
                                          const vec3f &direction,
                                          const range1f &tRange,
                                          const SamplesMask *samplesMask)
      {
        return new DumbRayIterator(
            this, origin, direction, tRange, samplesMask);
      }

      virtual float computeSample(const vec3f &objectCoordinates) const = 0;

      // default implementation if no vector implementations are defined
      virtual void computeSample8(const int *valid,
                                  const vly_vvec3f8 &objectCoordinates,
                                  float *samples)
      {
        for (int i = 0; i < 8; i++) {
          if (valid[i]) {
            samples[i] = computeSample(vec3f{objectCoordinates.x[i],
                                             objectCoordinates.y[i],
                                             objectCoordinates.z[i]});
          }
        }
      }

      virtual vec3f computeGradient(const vec3f &objectCoordinates) const = 0;
      virtual box3f getBoundingBox() const                                = 0;
    };

#define VLY_REGISTER_VOLUME(InternalClass, external_name) \
  VLY_REGISTER_OBJECT(Volume, volume, InternalClass, external_name)

  }  // namespace scalar_driver
}  // namespace volley
