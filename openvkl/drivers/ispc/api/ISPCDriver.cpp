// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ISPCDriver.h"
#include "../common/Data.h"
#include "../common/Observer.h"
#include "../common/export_util.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/Volume.h"
#include "ISPCDriver_ispc.h"

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
      return CALL_ISPC(ISPCDriver_getProgramCount);
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
    // Observer ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLObserver ISPCDriver<W>::newObserver(VKLVolume volume, const char *type)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.newObserver(type);
    }

    template <int W>
    const void * ISPCDriver<W>::mapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer>(observer);
      return observerObject.map();
    }

    template <int W>
    void ISPCDriver<W>::unmapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer>(observer);
      observerObject.unmap();
    }

    template <int W>
    VKLDataType ISPCDriver<W>::getObserverElementType(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer>(observer);
      return observerObject.getElementType();
    }

    template <int W>
    size_t ISPCDriver<W>::getObserverNumElements(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer>(observer);
      return observerObject.getNumElements();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void ISPCDriver<W>::initIntervalIterator1(
        vVKLIntervalIteratorN<1> &iterator,
        VKLVolume volume,
        const vvec3fn<1> &origin,
        const vvec3fn<1> &direction,
        const vrange1fn<1> &tRange,
        VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      iterator.volume = (VKLVolume)&volumeObject;

      volumeObject.initIntervalIteratorU(
          iterator,
          origin,
          direction,
          tRange,
          reinterpret_cast<const ValueSelector<W> *>(valueSelector));
    }

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

    __define_initIntervalIteratorN(4);
    __define_initIntervalIteratorN(8);
    __define_initIntervalIteratorN(16);

#undef __define_initIntervalIteratorN

    template <int W>
    void ISPCDriver<W>::iterateInterval1(vVKLIntervalIteratorN<1> &iterator,
                                         vVKLIntervalN<1> &interval,
                                         int *result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      volumeObject.iterateIntervalU(
          iterator, interval, reinterpret_cast<vintn<1> &>(*result));
    }

#define __define_iterateIntervalN(WIDTH)                               \
  template <int W>                                                     \
  void ISPCDriver<W>::iterateInterval##WIDTH(                          \
      const int *valid,                                                \
      vVKLIntervalIteratorN<WIDTH> &iterator,                          \
      vVKLIntervalN<WIDTH> &interval,                                  \
      int *result)                                                     \
  {                                                                    \
    iterateIntervalAnyWidth<WIDTH>(valid, iterator, interval, result); \
  }

    __define_iterateIntervalN(4);
    __define_iterateIntervalN(8);
    __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void ISPCDriver<W>::initHitIterator1(vVKLHitIteratorN<1> &iterator,
                                         VKLVolume volume,
                                         const vvec3fn<1> &origin,
                                         const vvec3fn<1> &direction,
                                         const vrange1fn<1> &tRange,
                                         VKLValueSelector valueSelector)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      iterator.volume = (VKLVolume)&volumeObject;

      volumeObject.initHitIteratorU(
          iterator,
          origin,
          direction,
          tRange,
          reinterpret_cast<const ValueSelector<W> *>(valueSelector));
    }

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

    __define_initHitIteratorN(4);
    __define_initHitIteratorN(8);
    __define_initHitIteratorN(16);

#undef __define_initHitIteratorN

    template <int W>
    void ISPCDriver<W>::iterateHit1(vVKLHitIteratorN<1> &iterator,
                                    vVKLHitN<1> &hit,
                                    int *result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      volumeObject.iterateHitU(
          iterator, hit, reinterpret_cast<vintn<1> &>(*result));
    }

#define __define_iterateHitN(WIDTH)                                        \
  template <int W>                                                         \
  void ISPCDriver<W>::iterateHit##WIDTH(const int *valid,                  \
                                        vVKLHitIteratorN<WIDTH> &iterator, \
                                        vVKLHitN<WIDTH> &hit,              \
                                        int *result)                       \
  {                                                                        \
    iterateHitAnyWidth<WIDTH>(valid, iterator, hit, result);               \
  }

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
          referenceFromHandle<ValueSelector<W>>(valueSelector);
      valueSelectorObject.setRanges(ranges);
    }

    template <int W>
    void ISPCDriver<W>::valueSelectorSetValues(
        VKLValueSelector valueSelector,
        const utility::ArrayView<const float> &values)
    {
      auto &valueSelectorObject =
          referenceFromHandle<ValueSelector<W>>(valueSelector);
      valueSelectorObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume ISPCDriver<W>::newVolume(const char *type)
    {
      // warn for deprecated snake case volume types
      std::string typeStr(type);

      if (typeStr.find("_") != std::string::npos) {
        postLogMessage(VKL_LOG_WARNING)
            << "volume type name '" << typeStr
            << "' may be deprecated; volume type names are now camelCase (no "
               "underscores)";
      }

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
      float *samples)                                                        \
  {                                                                          \
    computeSampleAnyWidth<WIDTH>(valid, volume, objectCoordinates, samples); \
  }

    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    // support a fast path for scalar sampling
    template <int W>
    void ISPCDriver<W>::computeSample1(const int *valid,
                                       VKLVolume volume,
                                       const vvec3fn<1> &objectCoordinates,
                                       float *sample)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      vfloatn<1> sampleW;
      volumeObject.computeSample(objectCoordinates, sampleW);
      *sample = sampleW[0];
    }

#define __define_computeGradientN(WIDTH)              \
  template <int W>                                    \
  void ISPCDriver<W>::computeGradient##WIDTH(         \
      const int *valid,                               \
      VKLVolume volume,                               \
      const vvec3fn<WIDTH> &objectCoordinates,        \
      vvec3fn<WIDTH> &gradients)                      \
  {                                                   \
    computeGradientAnyWidth<WIDTH>(                   \
        valid, volume, objectCoordinates, gradients); \
  }

    __define_computeGradientN(1);
    __define_computeGradientN(4);
    __define_computeGradientN(8);
    __define_computeGradientN(16);

#undef __define_computeGradientN

    template <int W>
    box3f ISPCDriver<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    template <int W>
    range1f ISPCDriver<W>::getValueRange(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getValueRange();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

      iterator.volume = (VKLVolume)&volumeObject;

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      volumeObject.initIntervalIteratorV(
          validW,
          iterator,
          origin,
          direction,
          tRange,
          reinterpret_cast<const ValueSelector<W> *>(valueSelector));
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
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLIntervalIteratorN<OW> &iterator,
                                           vVKLIntervalN<OW> &interval,
                                           int *result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      vintn<W> resultW;

      volumeObject.iterateIntervalV(validW, iterator, interval, resultW);

      for (int i = 0; i < W; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLIntervalIteratorN<OW> &iterator,
                                           vVKLIntervalN<OW> &interval,
                                           int *result)
    {
      throw std::runtime_error(
          "cannot iterate on intervals for widths different than the "
          "native runtime vector width");
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

      iterator.volume = (VKLVolume)&volumeObject;

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      volumeObject.initHitIteratorV(
          validW,
          iterator,
          origin,
          direction,
          tRange,
          reinterpret_cast<const ValueSelector<W> *>(valueSelector));
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
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateHitAnyWidth(const int *valid,
                                      vVKLHitIteratorN<OW> &iterator,
                                      vVKLHitN<OW> &hit,
                                      int *result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(iterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      vintn<W> resultW;

      volumeObject.iterateHitV(validW, iterator, hit, resultW);

      for (int i = 0; i < W; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateHitAnyWidth(const int *valid,
                                      vVKLHitIteratorN<OW> &iterator,
                                      vVKLHitN<OW> &hit,
                                      int *result)
    {
      throw std::runtime_error(
          "cannot iterate on hits for widths different than the native "
          "runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      volumeObject.computeSampleV(validW, ocW, samplesW);

      for (int i = 0; i < OW; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      vfloatn<W> samplesW;

      volumeObject.computeSampleV(validW, objectCoordinates, samplesW);

      for (int i = 0; i < W; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);

        vfloatn<W> samplesW;

        volumeObject.computeSampleV(validW, ocW, samplesW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          samples[i] = samplesW[i - packIndex * W];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    ISPCDriver<W>::computeGradientAnyWidth(const int *valid,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &objectCoordinates,
                                           vvec3fn<OW> &gradients)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);

      vvec3fn<W> gradientsW;

      volumeObject.computeGradientV(validW, ocW, gradientsW);

      for (int i = 0; i < OW; i++) {
        gradients.x[i] = gradientsW.x[i];
        gradients.y[i] = gradientsW.y[i];
        gradients.z[i] = gradientsW.z[i];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::computeGradientAnyWidth(const int *valid,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &objectCoordinates,
                                           vvec3fn<OW> &gradients)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      volumeObject.computeGradientV(validW, objectCoordinates, gradients);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    ISPCDriver<W>::computeGradientAnyWidth(const int *valid,
                                           VKLVolume volume,
                                           const vvec3fn<OW> &objectCoordinates,
                                           vvec3fn<OW> &gradients)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);

        vvec3fn<W> gradientsW;

        volumeObject.computeGradientV(validW, ocW, gradientsW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++) {
          gradients.x[i] = gradientsW.x[i - packIndex * W];
          gradients.y[i] = gradientsW.y[i - packIndex * W];
          gradients.z[i] = gradientsW.z[i - packIndex * W];
        }
      }
    }

    VKL_REGISTER_DRIVER(ISPCDriver<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_ispc_, VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl

extern "C" OPENVKL_DLLEXPORT void CONCAT1(openvkl_init_module_ispc_driver_,
                                          VKL_TARGET_WIDTH)()
{
}
