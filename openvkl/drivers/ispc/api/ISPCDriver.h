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
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_initIntervalIteratorN(WIDTH)                              \
  void initIntervalIterator##WIDTH(const int *valid,                       \
                                   vVKLIntervalIteratorN<WIDTH> &iterator, \
                                   VKLVolume volume,                       \
                                   const vvec3fn<WIDTH> &origin,           \
                                   const vvec3fn<WIDTH> &direction,        \
                                   const vrange1fn<WIDTH> &tRange,         \
                                   VKLValueSelector valueSelector) override;

      __define_initIntervalIteratorN(1);
      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

#define __define_iterateIntervalN(WIDTH)                              \
  void iterateInterval##WIDTH(const int *valid,                       \
                              vVKLIntervalIteratorN<WIDTH> &iterator, \
                              vVKLIntervalN<WIDTH> &interval,         \
                              vintn<WIDTH> &result) override;

      __define_iterateIntervalN(1);
      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

#define __define_initHitIteratorN(WIDTH)                         \
  void initHitIterator##WIDTH(const int *valid,                  \
                              vVKLHitIteratorN<WIDTH> &iterator, \
                              VKLVolume volume,                  \
                              const vvec3fn<WIDTH> &origin,      \
                              const vvec3fn<WIDTH> &direction,   \
                              const vrange1fn<WIDTH> &tRange,    \
                              VKLValueSelector valueSelector) override;

      __define_initHitIteratorN(1);
      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

#define __define_iterateHitN(WIDTH)                         \
  void iterateHit##WIDTH(const int *valid,                  \
                         vVKLHitIteratorN<WIDTH> &iterator, \
                         vVKLHitN<WIDTH> &hit,              \
                         vintn<WIDTH> &result) override;

      __define_iterateHitN(1);
      __define_iterateHitN(4);
      __define_iterateHitN(8);
      __define_iterateHitN(16);

#undef __define_iterateHitN

      /////////////////////////////////////////////////////////////////////////
      // Module ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLError loadModule(const char *moduleName) override;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void setBool(VKLObject object, const char *name, const bool b) override;
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
      // Value selector ///////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLValueSelector newValueSelector(VKLVolume volume) override;

      void valueSelectorSetRanges(
          VKLValueSelector valueSelector,
          const utility::ArrayView<const range1f> &ranges) override;

      void valueSelectorSetValues(
          VKLValueSelector valueSelector,
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

#define __define_computeGradientN(WIDTH)                               \
  void computeGradient##WIDTH(const int *valid,                        \
                              VKLVolume volume,                        \
                              const vvec3fn<WIDTH> &objectCoordinates, \
                              vvec3fn<WIDTH> &gradients) override;

      __define_computeGradientN(1);
      __define_computeGradientN(4);
      __define_computeGradientN(8);
      __define_computeGradientN(16);

#undef __define_computeGradientN

      box3f getBoundingBox(VKLVolume volume) override;

     private:
      template <int OW>
      typename std::enable_if<(OW == 1), void>::type
      initIntervalIteratorAnyWidth(const int *valid,
                                   vVKLIntervalIteratorN<OW> &iterator,
                                   VKLVolume volume,
                                   const vvec3fn<OW> &origin,
                                   const vvec3fn<OW> &direction,
                                   const vrange1fn<OW> &tRange,
                                   VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type
      initIntervalIteratorAnyWidth(const int *valid,
                                   vVKLIntervalIteratorN<OW> &iterator,
                                   VKLVolume volume,
                                   const vvec3fn<OW> &origin,
                                   const vvec3fn<OW> &direction,
                                   const vrange1fn<OW> &tRange,
                                   VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      initIntervalIteratorAnyWidth(const int *valid,
                                   vVKLIntervalIteratorN<OW> &iterator,
                                   VKLVolume volume,
                                   const vvec3fn<OW> &origin,
                                   const vvec3fn<OW> &direction,
                                   const vrange1fn<OW> &tRange,
                                   VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW == 1), void>::type iterateIntervalAnyWidth(
          const int *valid,
          vVKLIntervalIteratorN<OW> &iterator1,
          vVKLIntervalN<OW> &interval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type iterateIntervalAnyWidth(
          const int *valid,
          vVKLIntervalIteratorN<OW> &iterator,
          vVKLIntervalN<OW> &interval,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateIntervalAnyWidth(const int *valid,
                              vVKLIntervalIteratorN<OW> &iterator,
                              vVKLIntervalN<OW> &interval,
                              vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == 1), void>::type initHitIteratorAnyWidth(
          const int *valid,
          vVKLHitIteratorN<OW> &iterator,
          VKLVolume volume,
          const vvec3fn<OW> &origin,
          const vvec3fn<OW> &direction,
          const vrange1fn<OW> &tRange,
          VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type initHitIteratorAnyWidth(
          const int *valid,
          vVKLHitIteratorN<OW> &iterator,
          VKLVolume volume,
          const vvec3fn<OW> &origin,
          const vvec3fn<OW> &direction,
          const vrange1fn<OW> &tRange,
          VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      initHitIteratorAnyWidth(const int *valid,
                              vVKLHitIteratorN<OW> &iterator,
                              VKLVolume volume,
                              const vvec3fn<OW> &origin,
                              const vvec3fn<OW> &direction,
                              const vrange1fn<OW> &tRange,
                              VKLValueSelector valueSelector);

      template <int OW>
      typename std::enable_if<(OW == 1), void>::type iterateHitAnyWidth(
          const int *valid,
          vVKLHitIteratorN<OW> &iterator1,
          vVKLHitN<OW> &hit,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type iterateHitAnyWidth(
          const int *valid,
          vVKLHitIteratorN<OW> &iterator,
          vVKLHitN<OW> &hit,
          vintn<OW> &result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateHitAnyWidth(const int *valid,
                         vVKLHitIteratorN<OW> &iterator,
                         vVKLHitN<OW> &hit,
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

      template <int OW>
      typename std::enable_if<(OW <= W), void>::type computeGradientAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vvec3fn<OW> &gradients);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type computeGradientAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vvec3fn<OW> &gradients);
    };

  }  // namespace ispc_driver
}  // namespace openvkl
