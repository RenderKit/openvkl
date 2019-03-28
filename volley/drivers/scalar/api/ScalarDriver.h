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

#include "api/Driver.h"

namespace volley {
  namespace scalar_driver {

    struct ScalarDriver : public api::Driver
    {
      ScalarDriver()           = default;
      ~ScalarDriver() override = default;

      virtual void commit() override;

      virtual void commit(VLYObject object) override;

      virtual void release(VLYObject object) override;

      /////////////////////////////////////////////////////////////////////////
      // Iterator /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYRayIterator newRayIterator(VLYVolume volume,
                                    const vec3f &origin,
                                    const vec3f &direction,
                                    const range1f &tRange,
                                    VLYSamplesMask samplesMask) override;

      bool iterateInterval(VLYRayIterator rayIterator,
                           VLYRayInterval &rayInterval) override;

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYError loadModule(const char *moduleName) override;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void set1f(VLYObject object, const char *name, const float x) override;
      void set1i(VLYObject object, const char *name, const int x) override;
      void setVec3f(VLYObject object,
                    const char *name,
                    const vec3f &v) override;
      void setVec3i(VLYObject object,
                    const char *name,
                    const vec3i &v) override;
      void setVoidPtr(VLYObject object, const char *name, void *v) override;

      /////////////////////////////////////////////////////////////////////////
      // Samples mask /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYSamplesMask newSamplesMask(VLYVolume volume) override;

      void samplesMaskAddRanges(
          VLYSamplesMask samplesMask,
          const utility::ArrayView<const range1f> &ranges) override;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYVolume newVolume(const char *type) override;

      float computeSample(VLYVolume volume,
                          const vec3f &objectCoordinates) override;

      void computeSample8(const int *valid,
                          VLYVolume volume,
                          const vly_vvec3f8 &objectCoordinates,
                          float *samples) override;

      vec3f computeGradient(VLYVolume volume,
                            const vec3f &objectCoordinates) override;

      box3f getBoundingBox(VLYVolume volume) override;
    };

  }  // namespace scalar_driver
}  // namespace volley
