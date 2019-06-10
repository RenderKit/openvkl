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

      int getNativeSIMDWidth() override;

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

#define __define_newRayIteratorN(WIDTH)                                 \
  VLYRayIterator newRayIterator##WIDTH(const int *valid,                \
                                       VLYVolume volume,                \
                                       const vvec3fn<WIDTH> &origin,    \
                                       const vvec3fn<WIDTH> &direction, \
                                       const vrange1fn<WIDTH> &tRange,  \
                                       VLYSamplesMask samplesMask) override;

      __define_newRayIteratorN(1);
      __define_newRayIteratorN(4);
      __define_newRayIteratorN(8);
      __define_newRayIteratorN(16);

#undef __define_newRayIteratorN

#define __define_iterateIntervalN(WIDTH)                            \
  void iterateInterval##WIDTH(const int *valid,                     \
                              VLYRayIterator &rayIterator,          \
                              vVLYRayIntervalN<WIDTH> &rayInterval, \
                              vintn<WIDTH> &result) override;

      __define_iterateIntervalN(1);
      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

#define __define_iterateSurfaceN(WIDTH)                          \
  void iterateSurface##WIDTH(const int *valid,                   \
                             VLYRayIterator &rayIterator,        \
                             vVLYSurfaceHitN<WIDTH> &surfaceHit, \
                             vintn<WIDTH> &result) override;

      __define_iterateSurfaceN(1);
      __define_iterateSurfaceN(4);
      __define_iterateSurfaceN(8);
      __define_iterateSurfaceN(16);

#undef __define_iterateSurfaceN

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

#define __define_computeSampleN(WIDTH)                               \
  void computeSample##WIDTH(const int *valid,                        \
                            VLYVolume volume,                        \
                            const vvec3fn<WIDTH> &objectCoordinates, \
                            vfloatn<WIDTH> &samples) override;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      vec3f computeGradient(VLYVolume volume,
                            const vec3f &objectCoordinates) override;

      box3f getBoundingBox(VLYVolume volume) override;

     private:
      template <int OW>
      typename std::enable_if<(OW <= W), VLYRayIterator>::type
      newRayIteratorAnyWidth(const int *valid,
                             VLYVolume volume,
                             const vvec3fn<OW> &origin,
                             const vvec3fn<OW> &direction,
                             const vrange1fn<OW> &tRange,
                             VLYSamplesMask samplesMask);

      template <int OW>
      typename std::enable_if<(OW > W), VLYRayIterator>::type
      newRayIteratorAnyWidth(const int *valid,
                             VLYVolume volume,
                             const vvec3fn<OW> &origin,
                             const vvec3fn<OW> &direction,
                             const vrange1fn<OW> &tRange,
                             VLYSamplesMask samplesMask);

      template <int OW>
      typename std::enable_if<(OW <= W), void>::type iterateIntervalAnyWidth(
          const int *valid,
          VLYRayIterator &rayIterator,
          vVLYRayIntervalN<OW> &rayInterval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type iterateIntervalAnyWidth(
          const int *valid,
          VLYRayIterator &rayIterator,
          vVLYRayIntervalN<OW> &rayInterval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW <= W), void>::type iterateSurfaceAnyWidth(
          const int *valid,
          VLYRayIterator &rayIterator,
          vVLYSurfaceHitN<OW> &surfaceHit,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type iterateSurfaceAnyWidth(
          const int *valid,
          VLYRayIterator &rayIterator,
          vVLYSurfaceHitN<OW> &surfaceHit,
          vintn<OW> &result);

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
          vfloatn<OW> &samples);
    };

  }  // namespace ispc_driver
}  // namespace volley
