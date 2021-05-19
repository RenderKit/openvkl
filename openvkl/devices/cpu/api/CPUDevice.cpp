// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CPUDevice.h"
#include "../common/Data.h"
#include "../common/export_util.h"
#include "../iterator/Iterator.h"
#include "../observer/Observer.h"
#include "../sampler/Sampler.h"
#include "../value_selector/ValueSelector.h"
#include "../volume/Volume.h"
#include "CPUDevice_ispc.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    bool CPUDevice<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    int CPUDevice<W>::getNativeSIMDWidth()
    {
      return CALL_ISPC(ISPC_getProgramCount);
    }

    template <int W>
    void CPUDevice<W>::commit()
    {
      Device::commit();
    }

    template <int W>
    void CPUDevice<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void CPUDevice<W>::release(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLData CPUDevice<W>::newData(size_t numItems,
                                  VKLDataType dataType,
                                  const void *source,
                                  VKLDataCreationFlags dataCreationFlags,
                                  size_t byteStride)
    {
      Data *data =
          new Data(numItems, dataType, source, dataCreationFlags, byteStride);
      return (VKLData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Observer ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLObserver CPUDevice<W>::newObserver(VKLVolume volume, const char *type)
    {
      auto &object          = referenceFromHandle<Volume<W>>(volume);
      Observer<W> *observer = object.newObserver(type);
      return (VKLObserver)observer;
    }

    template <int W>
    VKLObserver CPUDevice<W>::newObserver(VKLSampler sampler, const char *type)
    {
      auto &object          = referenceFromHandle<Sampler<W>>(sampler);
      Observer<W> *observer = object.newObserver(type);
      return (VKLObserver)observer;
    }

    template <int W>
    const void *CPUDevice<W>::mapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.map();
    }

    template <int W>
    void CPUDevice<W>::unmapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      observerObject.unmap();
    }

    template <int W>
    VKLDataType CPUDevice<W>::getObserverElementType(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getElementType();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverElementSize(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getElementSize();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverNumElements(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer);
      return observerObject.getNumElements();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void CPUDevice<W>::setBool(VKLObject object, const char *name, const bool b)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, b);
    }

    template <int W>
    void CPUDevice<W>::set1f(VKLObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void CPUDevice<W>::set1i(VKLObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void CPUDevice<W>::setVec3f(VKLObject object,
                                const char *name,
                                const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void CPUDevice<W>::setVec3i(VKLObject object,
                                const char *name,
                                const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void CPUDevice<W>::setObject(VKLObject object,
                                 const char *name,
                                 VKLObject setObject)
    {
      ManagedObject *target = (ManagedObject *)object;
      ManagedObject *value  = (ManagedObject *)setObject;
      target->setParam(name, value);
    }

    template <int W>
    void CPUDevice<W>::setString(VKLObject object,
                                 const char *name,
                                 const std::string &s)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, s);
    }

    template <int W>
    void CPUDevice<W>::setVoidPtr(VKLObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Value selector /////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLValueSelector CPUDevice<W>::newValueSelector(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VKLValueSelector)volumeObject.newValueSelector();
    }

    template <int W>
    void CPUDevice<W>::valueSelectorSetRanges(
        VKLValueSelector valueSelector,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &valueSelectorObject =
          referenceFromHandle<ValueSelector<W>>(valueSelector);
      valueSelectorObject.setRanges(ranges);
    }

    template <int W>
    void CPUDevice<W>::valueSelectorSetValues(
        VKLValueSelector valueSelector,
        const utility::ArrayView<const float> &values)
    {
      auto &valueSelectorObject =
          referenceFromHandle<ValueSelector<W>>(valueSelector);
      valueSelectorObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Sampler ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLSampler CPUDevice<W>::newSampler(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VKLSampler)volumeObject.newSampler();
    }

#define __define_computeSampleN(WIDTH)                                      \
  template <int W>                                                          \
  void CPUDevice<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                     \
      VKLSampler sampler,                                                   \
      const vvec3fn<WIDTH> &objectCoordinates,                              \
      float *samples,                                                       \
      unsigned int attributeIndex,                                          \
      const float *times)                                                   \
  {                                                                         \
    computeSampleAnyWidth<WIDTH>(                                           \
        valid, sampler, objectCoordinates, samples, attributeIndex, times); \
  }

    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    // support a fast path for scalar sampling
    template <int W>
    void CPUDevice<W>::computeSample1(const int *valid,
                                      VKLSampler sampler,
                                      const vvec3fn<1> &objectCoordinates,
                                      float *sample,
                                      unsigned int attributeIndex,
                                      const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      vfloatn<1> timeW(time, 1);
      vfloatn<1> sampleW;
      samplerObject.computeSample(
          objectCoordinates, sampleW, attributeIndex, timeW);
      *sample = sampleW[0];
    }

    template <int W>
    void CPUDevice<W>::computeSampleN(VKLSampler sampler,
                                      unsigned int N,
                                      const vvec3fn<1> *objectCoordinates,
                                      float *samples,
                                      unsigned int attributeIndex,
                                      const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeSampleN(
          N, objectCoordinates, samples, attributeIndex, times);
    }

#define __define_computeSampleMN(WIDTH)              \
  template <int W>                                   \
  void CPUDevice<W>::computeSampleM##WIDTH(          \
      const int *valid,                              \
      VKLSampler sampler,                            \
      const vvec3fn<WIDTH> &objectCoordinates,       \
      float *samples,                                \
      unsigned int M,                                \
      const unsigned int *attributeIndices,          \
      const float *times)                            \
  {                                                  \
    computeSampleMAnyWidth<WIDTH>(valid,             \
                                  sampler,           \
                                  objectCoordinates, \
                                  samples,           \
                                  M,                 \
                                  attributeIndices,  \
                                  times);            \
  }

    __define_computeSampleMN(4);
    __define_computeSampleMN(8);
    __define_computeSampleMN(16);

#undef __define_computeSampleMN

    // support a fast path for scalar sampling
    template <int W>
    void CPUDevice<W>::computeSampleM1(const int *valid,
                                       VKLSampler sampler,
                                       const vvec3fn<1> &objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      vfloatn<1> timeW(time, 1);
      samplerObject.computeSampleM(
          objectCoordinates, samples, M, attributeIndices, timeW);
    }

    template <int W>
    void CPUDevice<W>::computeSampleMN(VKLSampler sampler,
                                       unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeSampleMN(
          N, objectCoordinates, samples, M, attributeIndices, times);
    }

#define __define_computeGradientN(WIDTH)                                      \
  template <int W>                                                            \
  void CPUDevice<W>::computeGradient##WIDTH(                                  \
      const int *valid,                                                       \
      VKLSampler sampler,                                                     \
      const vvec3fn<WIDTH> &objectCoordinates,                                \
      vvec3fn<WIDTH> &gradients,                                              \
      unsigned int attributeIndex,                                            \
      const float *times)                                                     \
  {                                                                           \
    computeGradientAnyWidth<WIDTH>(                                           \
        valid, sampler, objectCoordinates, gradients, attributeIndex, times); \
  }

    __define_computeGradientN(1);
    __define_computeGradientN(4);
    __define_computeGradientN(8);
    __define_computeGradientN(16);

#undef __define_computeGradientN

    template <int W>
    void CPUDevice<W>::computeGradientN(VKLSampler sampler,
                                        unsigned int N,
                                        const vvec3fn<1> *objectCoordinates,
                                        vvec3fn<1> *gradients,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);
      samplerObject.computeGradientN(
          N, objectCoordinates, gradients, attributeIndex, times);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume CPUDevice<W>::newVolume(const char *type)
    {
      // warn for deprecated snake case volume types
      std::string typeStr(type);

      if (typeStr.find("_") != std::string::npos) {
        postLogMessage(this, VKL_LOG_WARNING)
            << "volume type name '" << typeStr
            << "' may be deprecated; volume type names are now camelCase (no "
               "underscores)";
      }

      std::stringstream ss;
      ss << type << "_" << W;

      return (VKLVolume)Volume<W>::createInstance(this, ss.str());
    }

    template <int W>
    box3f CPUDevice<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    template <int W>
    unsigned int CPUDevice<W>::getNumAttributes(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getNumAttributes();
    }

    template <int W>
    range1f CPUDevice<W>::getValueRange(VKLVolume volume,
                                        unsigned int attributeIndex)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getValueRange(attributeIndex);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      samplerObject.computeSampleV(validW, ocW, samplesW, attributeIndex, tW);

      for (int i = 0; i < OW; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> tW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      vfloatn<W> samplesW;

      samplerObject.computeSampleV(
          validW, objectCoordinates, samplesW, attributeIndex, tW);

      for (int i = 0; i < W; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        VKLSampler sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        vfloatn<W> samplesW;

        samplerObject.computeSampleV(validW, ocW, samplesW, attributeIndex, tW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          samples[i] = samplesW[i - packIndex * W];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      float *samplesW = (float *)alloca(M * W * sizeof(float));

      samplerObject.computeSampleMV(
          validW, ocW, samplesW, M, attributeIndices, tW);

      for (unsigned int a = 0; a < M; a++) {
        for (int i = 0; i < OW; i++) {
          samples[a * OW + i] = samplesW[a * W + i];
        }
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> timesW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      samplerObject.computeSampleMV(
          validW, objectCoordinates, samples, M, attributeIndices, timesW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeSampleMAnyWidth(const int *valid,
                                         VKLSampler sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        float *samplesW = (float *)alloca(M * W * sizeof(float));

        samplerObject.computeSampleMV(
            validW, ocW, samplesW, M, attributeIndices, tW);

        for (unsigned int a = 0; a < M; a++) {
          for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
            samples[a * OW + i] = samplesW[a * W + (i - packIndex * W)];
        }
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);
      vfloatn<W> tW(times, OW);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      ocW.fill_inactive_lanes(validW);
      tW.fill_inactive_lanes(validW);

      vvec3fn<W> gradientsW;

      samplerObject.computeGradientV(
          validW, ocW, gradientsW, attributeIndex, tW);

      for (int i = 0; i < OW; i++) {
        gradients.x[i] = gradientsW.x[i];
        gradients.y[i] = gradientsW.y[i];
        gradients.z[i] = gradientsW.z[i];
      }
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<W> tW(times, W);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = valid[i];

      samplerObject.computeGradientV(
          validW, objectCoordinates, gradients, attributeIndex, tW);
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    CPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          VKLSampler sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler);

      vfloatn<OW> tOW(times, OW);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);
        vfloatn<W> tW  = tOW.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        ocW.fill_inactive_lanes(validW);
        tW.fill_inactive_lanes(validW);

        vvec3fn<W> gradientsW;

        samplerObject.computeGradientV(
            validW, ocW, gradientsW, attributeIndex, tW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++) {
          gradients.x[i] = gradientsW.x[i - packIndex * W];
          gradients.y[i] = gradientsW.y[i - packIndex * W];
          gradients.z[i] = gradientsW.z[i - packIndex * W];
        }
      }
    }

    VKL_REGISTER_DEVICE(CPUDevice<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_cpu_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl

extern "C" OPENVKL_DLLEXPORT void CONCAT1(openvkl_init_module_cpu_device_,
                                          VKL_TARGET_WIDTH)()
{
}
