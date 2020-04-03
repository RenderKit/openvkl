// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
      // Observer /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLObserver newObserver(VKLVolume volume, const char *type) override;
      const void * mapObserver(VKLObserver observer) override;
      void unmapObserver(VKLObserver observer) override;
      VKLDataType getObserverElementType(VKLObserver observer) const override;
      size_t getObserverNumElements(VKLObserver observer) const override;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void initIntervalIterator1(vVKLIntervalIteratorN<1> &iterator,
                                 VKLVolume volume,
                                 const vvec3fn<1> &origin,
                                 const vvec3fn<1> &direction,
                                 const vrange1fn<1> &tRange,
                                 VKLValueSelector valueSelector) override;

#define __define_initIntervalIteratorN(WIDTH)                              \
  void initIntervalIterator##WIDTH(const int *valid,                       \
                                   vVKLIntervalIteratorN<WIDTH> &iterator, \
                                   VKLVolume volume,                       \
                                   const vvec3fn<WIDTH> &origin,           \
                                   const vvec3fn<WIDTH> &direction,        \
                                   const vrange1fn<WIDTH> &tRange,         \
                                   VKLValueSelector valueSelector) override;

      __define_initIntervalIteratorN(4);
      __define_initIntervalIteratorN(8);
      __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

      void iterateInterval1(vVKLIntervalIteratorN<1> &iterator,
                            vVKLIntervalN<1> &interval,
                            int *result) override;

#define __define_iterateIntervalN(WIDTH)                              \
  void iterateInterval##WIDTH(const int *valid,                       \
                              vVKLIntervalIteratorN<WIDTH> &iterator, \
                              vVKLIntervalN<WIDTH> &interval,         \
                              int *result) override;

      __define_iterateIntervalN(4);
      __define_iterateIntervalN(8);
      __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void initHitIterator1(vVKLHitIteratorN<1> &iterator,
                            VKLVolume volume,
                            const vvec3fn<1> &origin,
                            const vvec3fn<1> &direction,
                            const vrange1fn<1> &tRange,
                            VKLValueSelector valueSelector) override;

#define __define_initHitIteratorN(WIDTH)                         \
  void initHitIterator##WIDTH(const int *valid,                  \
                              vVKLHitIteratorN<WIDTH> &iterator, \
                              VKLVolume volume,                  \
                              const vvec3fn<WIDTH> &origin,      \
                              const vvec3fn<WIDTH> &direction,   \
                              const vrange1fn<WIDTH> &tRange,    \
                              VKLValueSelector valueSelector) override;

      __define_initHitIteratorN(4);
      __define_initHitIteratorN(8);
      __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

      void iterateHit1(vVKLHitIteratorN<1> &iterator,
                       vVKLHitN<1> &hit,
                       int *result) override;

#define __define_iterateHitN(WIDTH)                         \
  void iterateHit##WIDTH(const int *valid,                  \
                         vVKLHitIteratorN<WIDTH> &iterator, \
                         vVKLHitN<WIDTH> &hit,              \
                         int *result) override;

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
                            float *samples) override;

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

      range1f getValueRange(VKLVolume volume) override;

     private:
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
      typename std::enable_if<(OW == W), void>::type iterateIntervalAnyWidth(
          const int *valid,
          vVKLIntervalIteratorN<OW> &iterator,
          vVKLIntervalN<OW> &interval,
          int *result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateIntervalAnyWidth(const int *valid,
                              vVKLIntervalIteratorN<OW> &iterator,
                              vVKLIntervalN<OW> &interval,
                              int *result);

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
      typename std::enable_if<(OW == W), void>::type iterateHitAnyWidth(
          const int *valid,
          vVKLHitIteratorN<OW> &iterator,
          vVKLHitN<OW> &hit,
          int *result);

      template <int OW>
      typename std::enable_if<(OW != W && OW != 1), void>::type
      iterateHitAnyWidth(const int *valid,
                         vVKLHitIteratorN<OW> &iterator,
                         vVKLHitN<OW> &hit,
                         int *result);

      template <int OW>
      typename std::enable_if<(OW < W), void>::type computeSampleAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          float *samples);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type computeSampleAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          float *samples);

      template <int OW>
      typename std::enable_if<(OW > W), void>::type computeSampleAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          float *samples);

      template <int OW>
      typename std::enable_if<(OW < W), void>::type computeGradientAnyWidth(
          const int *valid,
          VKLVolume volume,
          const vvec3fn<OW> &objectCoordinates,
          vvec3fn<OW> &gradients);

      template <int OW>
      typename std::enable_if<(OW == W), void>::type computeGradientAnyWidth(
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
