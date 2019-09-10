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

#include "ISPCDriver.h"
#include "../common/Data.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/Volume.h"
#include "ispc_util_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    bool ISPCDriver<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    int ISPCDriver<W>::getNativeSIMDWidth()
    {
      return ispc::get_programCount();
    }

    template <int W>
    void ISPCDriver<W>::commit()
    {
      Driver::commit();
    }

    template <int W>
    void ISPCDriver<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void ISPCDriver<W>::release(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLData ISPCDriver<W>::newData(size_t numItems,
                                   VKLDataType dataType,
                                   const void *source,
                                   VKLDataCreationFlags dataCreationFlags)
    {
      Data *data = new Data(numItems, dataType, source, dataCreationFlags);
      return (VKLData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

#define __define_initIntervalIteratorN(WIDTH)                               \
  template <int W>                                                          \
  void ISPCDriver<W>::initIntervalIterator##WIDTH(                          \
      const int *valid,                                                     \
      vVKLIntervalIteratorN<WIDTH> &iterator,                               \
      VKLVolume volume,                                                     \
      const vvec3fn<WIDTH> &origin,                                         \
      const vvec3fn<WIDTH> &direction,                                      \
      const vrange1fn<WIDTH> &tRange,                                       \
      VKLValueSelector valueSelector)                                       \
  {                                                                         \
    initIntervalIteratorAnyWidth<WIDTH>(                                    \
        valid, iterator, volume, origin, direction, tRange, valueSelector); \
  }

    __define_initIntervalIteratorN(1);
    __define_initIntervalIteratorN(4);
    __define_initIntervalIteratorN(8);
    __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

#define __define_iterateIntervalN(WIDTH)                               \
  template <int W>                                                     \
  void ISPCDriver<W>::iterateInterval##WIDTH(                          \
      const int *valid,                                                \
      vVKLIntervalIteratorN<WIDTH> &iterator,                          \
      vVKLIntervalN<WIDTH> &interval,                                  \
      vintn<WIDTH> &result)                                            \
  {                                                                    \
    iterateIntervalAnyWidth<WIDTH>(valid, iterator, interval, result); \
  }

    __define_iterateIntervalN(1);
    __define_iterateIntervalN(4);
    __define_iterateIntervalN(8);
    __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

#define __define_initHitIteratorN(WIDTH)                                    \
  template <int W>                                                          \
  void ISPCDriver<W>::initHitIterator##WIDTH(                               \
      const int *valid,                                                     \
      vVKLHitIteratorN<WIDTH> &iterator,                                    \
      VKLVolume volume,                                                     \
      const vvec3fn<WIDTH> &origin,                                         \
      const vvec3fn<WIDTH> &direction,                                      \
      const vrange1fn<WIDTH> &tRange,                                       \
      VKLValueSelector valueSelector)                                       \
  {                                                                         \
    initHitIteratorAnyWidth<WIDTH>(                                         \
        valid, iterator, volume, origin, direction, tRange, valueSelector); \
  }

    __define_initHitIteratorN(1);
    __define_initHitIteratorN(4);
    __define_initHitIteratorN(8);
    __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

#define __define_iterateHitN(WIDTH)                                        \
  template <int W>                                                         \
  void ISPCDriver<W>::iterateHit##WIDTH(const int *valid,                  \
                                        vVKLHitIteratorN<WIDTH> &iterator, \
                                        vVKLHitN<WIDTH> &hit,              \
                                        vintn<WIDTH> &result)              \
  {                                                                        \
    iterateHitAnyWidth<WIDTH>(valid, iterator, hit, result);               \
  }

    __define_iterateHitN(1);
    __define_iterateHitN(4);
    __define_iterateHitN(8);
    __define_iterateHitN(16);

#undef __define_iterateHitN

    ///////////////////////////////////////////////////////////////////////////
    // Module /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLError ISPCDriver<W>::loadModule(const char *moduleName)
    {
      return openvkl::loadLocalModule(moduleName);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void ISPCDriver<W>::setBool(VKLObject object,
                                const char *name,
                                const bool b)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, b);
    }

    template <int W>
    void ISPCDriver<W>::set1f(VKLObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::set1i(VKLObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::setVec3f(VKLObject object,
                                 const char *name,
                                 const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setVec3i(VKLObject object,
                                 const char *name,
                                 const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setObject(VKLObject object,
                                  const char *name,
                                  VKLObject setObject)
    {
      ManagedObject *target = (ManagedObject *)object;
      ManagedObject *value  = (ManagedObject *)setObject;
      target->setParam(name, value);
    }

    template <int W>
    void ISPCDriver<W>::setString(VKLObject object,
                                  const char *name,
                                  const std::string &s)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, s);
    }

    template <int W>
    void ISPCDriver<W>::setVoidPtr(VKLObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    /////////////////////////////////////////////////////////////////////////
    // Value selector ///////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLValueSelector ISPCDriver<W>::newValueSelector(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VKLValueSelector)volumeObject.newValueSelector();
    }

    template <int W>
    void ISPCDriver<W>::valueSelectorSetRanges(
        VKLValueSelector valueSelector,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &valueSelectorObject =
          referenceFromHandle<ValueSelector>(valueSelector);
      valueSelectorObject.setRanges(ranges);
    }

    template <int W>
    void ISPCDriver<W>::valueSelectorSetValues(
        VKLValueSelector valueSelector,
        const utility::ArrayView<const float> &values)
    {
      auto &valueSelectorObject =
          referenceFromHandle<ValueSelector>(valueSelector);
      valueSelectorObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume ISPCDriver<W>::newVolume(const char *type)
    {
      std::stringstream ss;
      ss << type << "_" << W;
      return (VKLVolume)Volume<W>::createInstance(ss.str());
    }

#define __define_computeSampleN(WIDTH)                                       \
  template <int W>                                                           \
  void ISPCDriver<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                      \
      VKLVolume volume,                                                      \
      const vvec3fn<WIDTH> &objectCoordinates,                               \
      vfloatn<WIDTH> &samples)                                               \
  {                                                                          \
    computeSampleAnyWidth<WIDTH>(valid, volume, objectCoordinates, samples); \
  }

    __define_computeSampleN(1);
    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    template <int W>
    vec3f ISPCDriver<W>::computeGradient(VKLVolume volume,
                                         const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.computeGradient(objectCoordinates);
    }

    template <int W>
    box3f ISPCDriver<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::initIntervalIteratorAnyWidth(
        const int *valid,
        vVKLIntervalIteratorN<OW> &iterator,
        VKLVolume volume,
        const vvec3fn<OW> &origin,
        const vvec3fn<OW> &direction,
        const vrange1fn<OW> &tRange,
        VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      vVKLIntervalIteratorN<W> iteratorW;

      volumeObject.initIntervalIteratorV(
          validW,
          iteratorW,
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const ValueSelector *>(valueSelector));

      iterator = static_cast<vVKLIntervalIteratorN<1>>(iteratorW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::initIntervalIteratorAnyWidth(
        const int *valid,
        vVKLIntervalIteratorN<OW> &iterator,
        VKLVolume volume,
        const vvec3fn<OW> &origin,
        const vvec3fn<OW> &direction,
        const vrange1fn<OW> &tRange,
        VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      volumeObject.initIntervalIteratorV(
          validW,
          iterator,
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const ValueSelector *>(valueSelector));
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::initIntervalIteratorAnyWidth(
        const int *valid,
        vVKLIntervalIteratorN<OW> &iterator,
        VKLVolume volume,
        const vvec3fn<OW> &origin,
        const vvec3fn<OW> &direction,
        const vrange1fn<OW> &tRange,
        VKLValueSelector valueSelector)
    {
      throw std::runtime_error(
          "interval iterators cannot be created for widths different than the "
          "native runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLIntervalIteratorN<OW> &iterator1,
                                           vVKLIntervalN<OW> &interval,
                                           vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator1.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLIntervalN<W> intervalW;

      vintn<W> resultW;

      vVKLIntervalIteratorN<W> iteratorW =
          static_cast<vVKLIntervalIteratorN<W>>(iterator1);

      volumeObject.iterateIntervalV(validW, iteratorW, intervalW, resultW);

      iterator1 = static_cast<vVKLIntervalIteratorN<1>>(iteratorW);

      for (int i = 0; i < OW; i++) {
        interval.tRange.lower[i]     = intervalW.tRange.lower[i];
        interval.tRange.upper[i]     = intervalW.tRange.upper[i];
        interval.valueRange.lower[i] = intervalW.valueRange.lower[i];
        interval.valueRange.upper[i] = intervalW.valueRange.upper[i];
        interval.nominalDeltaT[i]    = intervalW.nominalDeltaT[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLIntervalIteratorN<OW> &iterator,
                                           vVKLIntervalN<OW> &interval,
                                           vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLIntervalN<W> intervalW;

      vintn<W> resultW;

      volumeObject.iterateIntervalV(validW, iterator, intervalW, resultW);

      for (int i = 0; i < OW; i++) {
        interval.tRange.lower[i]     = intervalW.tRange.lower[i];
        interval.tRange.upper[i]     = intervalW.tRange.upper[i];
        interval.valueRange.lower[i] = intervalW.valueRange.lower[i];
        interval.valueRange.upper[i] = intervalW.valueRange.upper[i];
        interval.nominalDeltaT[i]    = intervalW.nominalDeltaT[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLIntervalIteratorN<OW> &iterator,
                                           vVKLIntervalN<OW> &interval,
                                           vintn<OW> &result)
    {
      throw std::runtime_error(
          "cannot iterate on intervals for widths different than the "
          "native runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::initHitIteratorAnyWidth(const int *valid,
                                           vVKLHitIteratorN<OW> &iterator,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &origin,
                                           const vvec3fn<OW> &direction,
                                           const vrange1fn<OW> &tRange,
                                           VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      vVKLHitIteratorN<W> iteratorW;

      volumeObject.initHitIteratorV(
          validW,
          iteratorW,
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const ValueSelector *>(valueSelector));

      iterator = static_cast<vVKLHitIteratorN<1>>(iteratorW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::initHitIteratorAnyWidth(const int *valid,
                                           vVKLHitIteratorN<OW> &iterator,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &origin,
                                           const vvec3fn<OW> &direction,
                                           const vrange1fn<OW> &tRange,
                                           VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      volumeObject.initHitIteratorV(
          validW,
          iterator,
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const ValueSelector *>(valueSelector));
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::initHitIteratorAnyWidth(const int *valid,
                                           vVKLHitIteratorN<OW> &iterator,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &origin,
                                           const vvec3fn<OW> &direction,
                                           const vrange1fn<OW> &tRange,
                                           VKLValueSelector valueSelector)
    {
      throw std::runtime_error(
          "hit iterators cannot be created for widths different than the "
          "native runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::iterateHitAnyWidth(const int *valid,
                                      vVKLHitIteratorN<OW> &iterator1,
                                      vVKLHitN<OW> &hit,
                                      vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator1.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLHitN<W> hitW;

      vintn<W> resultW;

      vVKLHitIteratorN<W> iteratorW =
          static_cast<vVKLHitIteratorN<W>>(iterator1);

      volumeObject.iterateHitV(validW, iteratorW, hitW, resultW);

      iterator1 = static_cast<vVKLHitIteratorN<1>>(iteratorW);

      for (int i = 0; i < OW; i++) {
        hit.t[i]      = hitW.t[i];
        hit.sample[i] = hitW.sample[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateHitAnyWidth(const int *valid,
                                      vVKLHitIteratorN<OW> &iterator,
                                      vVKLHitN<OW> &hit,
                                      vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLHitN<W> hitW;

      vintn<W> resultW;

      volumeObject.iterateHitV(validW, iterator, hitW, resultW);

      for (int i = 0; i < OW; i++) {
        hit.t[i]      = hitW.t[i];
        hit.sample[i] = hitW.sample[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateHitAnyWidth(const int *valid,
                                      vVKLHitIteratorN<OW> &iterator,
                                      vVKLHitN<OW> &hit,
                                      vintn<OW> &result)
    {
      throw std::runtime_error(
          "cannot iterate on hits for widths different than the native "
          "runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW <= W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         vfloatn<OW> &samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vfloatn<W> samplesW;

      volumeObject.computeSampleV(validW, ocW, samplesW);

      for (int i = 0; i < OW; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         vfloatn<OW> &samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        vfloatn<W> samplesW;

        volumeObject.computeSampleV(validW, ocW, samplesW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          samples[i] = samplesW[i - packIndex * W];
      }
    }

    VKL_REGISTER_DRIVER(ISPCDriver<4>, ispc_driver_4)
    VKL_REGISTER_DRIVER(ISPCDriver<8>, ispc_driver_8)
    VKL_REGISTER_DRIVER(ISPCDriver<16>, ispc_driver_16)

  }  // namespace ispc_driver
}  // namespace openvkl

extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_ispc_driver() {}
