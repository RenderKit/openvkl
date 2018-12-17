// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "../common/Ray.h"
#include "../transferFunction/TransferFunction.h"

namespace ospray {
  namespace scalar_volley_device {

    struct Volume : public ManagedObject
    {
      Volume() = default;

      virtual void commit() override;

      // returns true if the given ray intersects the volume and sets the ray t
      // values as appropriate
      virtual bool intersect(Ray &ray) const = 0;

      // computes a sample at the world-space coordinate within the volume
      virtual float computeSample(
          const ospcommon::vec3f &worldCoordinates) const = 0;

      // advance the given ray by a step appropriate to the volume
      virtual void advance(Ray &ray) const = 0;

      virtual int setRegion(const void *source,
                            const vec3i &index,
                            const vec3i &count) = 0;

      inline float getSamplingStep();
      inline float getSamplingRate();

      const TransferFunction &getTransferFunction() const;

     protected:
      // nominal sampling step size; should be set by the underlying volume
      // implementation
      float samplingStep{1.f};
      float samplingRate{1.f};

      TransferFunction *transferFunction{nullptr};
    };

    // Inlined members ////////////////////////////////////////////////////////

    inline float Volume::getSamplingStep()
    {
      return samplingStep;
    }

    inline float Volume::getSamplingRate()
    {
      return samplingRate;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
