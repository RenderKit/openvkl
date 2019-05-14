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
  namespace ispc_driver {

    template <int W>
    struct ISPCDriver : public api::Driver
    {
      ISPCDriver()           = default;
      ~ISPCDriver() override = default;

      bool supportsWidth(int width) override;

      void commit() override;

      void commit(VLYObject object) override;

      void release(VLYObject object) override;

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYData newData(size_t numItems,
                      VLYDataType dataType,
                      const void *source,
                      VLYDataCreationFlags dataCreationFlags) override;

      /////////////////////////////////////////////////////////////////////////
      // Iterator /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYRayIterator newRayIterator(VLYVolume volume,
                                    const vec3f &origin,
                                    const vec3f &direction,
                                    const range1f &tRange,
                                    VLYSamplesMask samplesMask) override;

      VLYRayIterator newRayIterator8(const int *valid,
                                     VLYVolume volume,
                                     const vvec3fn<8> &origin,
                                     const vvec3fn<8> &direction,
                                     const vrange1fn<8> &tRange,
                                     VLYSamplesMask samplesMask) override;

      bool iterateInterval(VLYRayIterator rayIterator,
                           VLYRayInterval &rayInterval) override;

      void iterateInterval8(const int *valid,
                            VLYRayIterator rayIterator,
                            VLYRayInterval8 &rayInterval,
                            vintn<8> &result) override;

      void iterateSurface8(const int *valid,
                           VLYRayIterator rayIterator,
                           VLYSurfaceHit8 &surfaceHit,
                           vintn<8> &result) override;

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
      void setObject(VLYObject object,
                     const char *name,
                     VLYObject setObject) override;
      void setString(VLYObject object,
                     const char *name,
                     const std::string &s) override;
      void setVoidPtr(VLYObject object, const char *name, void *v) override;

      /////////////////////////////////////////////////////////////////////////
      // Samples mask /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYSamplesMask newSamplesMask(VLYVolume volume) override;

      void samplesMaskSetRanges(
          VLYSamplesMask samplesMask,
          const utility::ArrayView<const range1f> &ranges) override;

      void samplesMaskSetValues(
          VLYSamplesMask samplesMask,
          const utility::ArrayView<const float> &values) override;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VLYVolume newVolume(const char *type) override;

      float computeSample(VLYVolume volume,
                          const vec3f &objectCoordinates) override;

#define __define_computeSampleN(WIDTH)                               \
  void computeSample##WIDTH(const int *valid,                        \
                            VLYVolume volume,                        \
                            const vvec3fn<WIDTH> &objectCoordinates, \
                            vfloatn<WIDTH> &samples) override;

      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      vec3f computeGradient(VLYVolume volume,
                            const vec3f &objectCoordinates) override;

      box3f getBoundingBox(VLYVolume volume) override;

     private:
      template <int OW>
      typename std::enable_if<(OW <= W), void>::type computeSampleAnyWidth(
          const int *valid,
          VLYVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vfloatn<OW> &samples);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type computeSampleAnyWidth(
          const int *valid,
          VLYVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vfloatn<OW> &samples)
      {
        throw std::runtime_error("computeSample() not legal for calling width > native vector width");
      }
    };

  }  // namespace ispc_driver
}  // namespace volley
