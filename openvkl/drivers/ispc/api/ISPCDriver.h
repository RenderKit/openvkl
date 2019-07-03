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

#include "../../../api/Driver.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct ISPCDriver : public api::Driver
    {
      ISPCDriver()           = default;
      ~ISPCDriver() override = default;

      bool supportsWidth(int width) override;

      int getNativeSIMDWidth() override;

      void commit() override;

      void commit(VKLObject object) override;

      void release(VKLObject object) override;

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLData newData(size_t numItems,
                      VKLDataType dataType,
                      const void *source,
                      VKLDataCreationFlags dataCreationFlags) override;

      /////////////////////////////////////////////////////////////////////////
      // Iterator /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_initRayIteratorN(WIDTH)                            \
  void initRayIterator##WIDTH(const int *valid,                     \
                              vVKLRayIteratorN<WIDTH> &rayIterator, \
                              VKLVolume volume,                     \
                              const vvec3fn<WIDTH> &origin,         \
                              const vvec3fn<WIDTH> &direction,      \
                              const vrange1fn<WIDTH> &tRange,       \
                              VKLSamplesMask samplesMask) override;

      __define_initRayIteratorN(1);
      __define_initRayIteratorN(4);
      __define_initRayIteratorN(8);
      __define_initRayIteratorN(16);

#undef __define_initRayIteratorN

#define __define_iterateIntervalN(WIDTH)                            \
  void iterateInterval##WIDTH(const int *valid,                     \
                              vVKLRayIteratorN<WIDTH> &rayIterator, \
                              vVKLRayIntervalN<WIDTH> &rayInterval, \
                              vintn<WIDTH> &result) override;

      __define_iterateIntervalN(1);
      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

#define __define_iterateSurfaceN(WIDTH)                            \
  void iterateSurface##WIDTH(const int *valid,                     \
                             vVKLRayIteratorN<WIDTH> &rayIterator, \
                             vVKLSurfaceHitN<WIDTH> &surfaceHit,   \
                             vintn<WIDTH> &result) override;

      __define_iterateSurfaceN(1);
      __define_iterateSurfaceN(4);
      __define_iterateSurfaceN(8);
      __define_iterateSurfaceN(16);

#undef __define_iterateSurfaceN

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLError loadModule(const char *moduleName) override;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void set1f(VKLObject object, const char *name, const float x) override;
      void set1i(VKLObject object, const char *name, const int x) override;
      void setVec3f(VKLObject object,
                    const char *name,
                    const vec3f &v) override;
      void setVec3i(VKLObject object,
                    const char *name,
                    const vec3i &v) override;
      void setObject(VKLObject object,
                     const char *name,
                     VKLObject setObject) override;
      void setString(VKLObject object,
                     const char *name,
                     const std::string &s) override;
      void setVoidPtr(VKLObject object, const char *name, void *v) override;

      /////////////////////////////////////////////////////////////////////////
      // Samples mask /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLSamplesMask newSamplesMask(VKLVolume volume) override;

      void samplesMaskSetRanges(
          VKLSamplesMask samplesMask,
          const utility::ArrayView<const range1f> &ranges) override;

      void samplesMaskSetValues(
          VKLSamplesMask samplesMask,
          const utility::ArrayView<const float> &values) override;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLVolume newVolume(const char *type) override;

#define __define_computeSampleN(WIDTH)                               \
  void computeSample##WIDTH(const int *valid,                        \
                            VKLVolume volume,                        \
                            const vvec3fn<WIDTH> &objectCoordinates, \
                            vfloatn<WIDTH> &samples) override;

      __define_computeSampleN(1);
      __define_computeSampleN(4);
      __define_computeSampleN(8);
      __define_computeSampleN(16);

#undef __define_computeSampleN

      vec3f computeGradient(VKLVolume volume,
                            const vec3f &objectCoordinates) override;

      box3f getBoundingBox(VKLVolume volume) override;

     private:
      template <int OW>
      typename std::enable_if<(OW == 1), void>::type initRayIteratorAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator,
          VKLVolume volume,
          const vvec3fn<OW> &origin,
          const vvec3fn<OW> &direction,
          const vrange1fn<OW> &tRange,
          VKLSamplesMask samplesMask);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type initRayIteratorAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator,
          VKLVolume volume,
          const vvec3fn<OW> &origin,
          const vvec3fn<OW> &direction,
          const vrange1fn<OW> &tRange,
          VKLSamplesMask samplesMask);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      initRayIteratorAnyWidth(const int *valid,
                              vVKLRayIteratorN<OW> &rayIterator,
                              VKLVolume volume,
                              const vvec3fn<OW> &origin,
                              const vvec3fn<OW> &direction,
                              const vrange1fn<OW> &tRange,
                              VKLSamplesMask samplesMask);

      template <int OW>
      typename std::enable_if<(OW == 1), void>::type iterateIntervalAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator1,
          vVKLRayIntervalN<OW> &rayInterval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type iterateIntervalAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator,
          vVKLRayIntervalN<OW> &rayInterval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateIntervalAnyWidth(const int *valid,
                              vVKLRayIteratorN<OW> &rayIterator,
                              vVKLRayIntervalN<OW> &rayInterval,
                              vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == 1), void>::type iterateSurfaceAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator1,
          vVKLSurfaceHitN<OW> &surfaceHit,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type iterateSurfaceAnyWidth(
          const int *valid,
          vVKLRayIteratorN<OW> &rayIterator,
          vVKLSurfaceHitN<OW> &surfaceHit,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateSurfaceAnyWidth(const int *valid,
                             vVKLRayIteratorN<OW> &rayIterator,
                             vVKLSurfaceHitN<OW> &surfaceHit,
                             vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW <= W), void>::type computeSampleAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vfloatn<OW> &samples);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type computeSampleAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vfloatn<OW> &samples);
    };

  }  // namespace ispc_driver
}  // namespace openvkl
