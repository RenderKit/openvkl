// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "CPUDevice.h"
#include "../common/Data.h"
#include "../common/ObjectFactory.h"
#include "../common/export_util.h"
#include "../common/ispc_isa.h"
#include "../iterator/Iterator.h"
#include "../observer/Observer.h"
#include "../sampler/Sampler.h"
#include "../volume/Volume.h"
#include "CPUDevice_ispc.h"
#include "openvkl/devices/common/BufferShared.h"

namespace openvkl {
  namespace cpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // CPUDevice //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    CPUDevice<W>::~CPUDevice()
    {
      delete (ispcrt::Context*) context;
    }

    template <int W>
    api::memstate * CPUDevice<W>::allocateBytes(size_t numBytes) const
    {
      api::memstate *container = new api::memstate;
      void *buffer = (char *)new char[numBytes];
      container->privateManagement = nullptr;
      container->allocatedBuffer = buffer;
      return container;
    }

    template <int W>
    void CPUDevice<W>::freeMemState(api::memstate *container) const
    {
       delete[] (char*)container->allocatedBuffer;
       delete container;
    }

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

      if (!context) {
        auto _context = new ispcrt::Context(ISPCRT_DEVICE_TYPE_CPU);
        context = (void*) _context;
      }

      VKLISPCTarget target =
          static_cast<VKLISPCTarget>(CALL_ISPC(ISPC_getTarget));

      postLogMessage(this, VKL_LOG_DEBUG)
          << "CPU device instantiated with width: " << getNativeSIMDWidth()
          << ", ISA: " << stringForVKLISPCTarget(target);
    }

    template <int W>
    void CPUDevice<W>::commit(APIObject object)
    {
      ManagedObject *managedObject = static_cast<ManagedObject *>(object.host);
      managedObject->commit();
    }

    template <int W>
    void CPUDevice<W>::release(APIObject object)
    {
      ManagedObject *managedObject = static_cast<ManagedObject *>(object.host);
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
      Data *data = new Data(
          this, numItems, dataType, source, dataCreationFlags, byteStride);

      VKLData d;
      d.host   = data;
      d.device = nullptr;

      return d;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Observer ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLObserver CPUDevice<W>::newVolumeObserver(VKLVolume volume,
                                                const char *type)
    {
      auto &object          = referenceFromHandle<Volume<W>>(volume.host);
      Observer<W> *observer = object.newObserver(type);
      VKLObserver o;
      o.host   = static_cast<void *>(observer);
      o.device = nullptr;  // Observer has no shared struct
      return o;
    }

    template <int W>
    VKLObserver CPUDevice<W>::newSamplerObserver(VKLSampler sampler,
                                                 const char *type)
    {
      auto &object          = referenceFromHandle<Sampler<W>>(sampler.host);
      Observer<W> *observer = object.newObserver(type);
      VKLObserver o;
      o.host   = static_cast<void *>(observer);
      o.device = nullptr;  // Observer has no shared struct
      return o;
    }

    template <int W>
    const void *CPUDevice<W>::mapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer.host);
      return observerObject.map();
    }

    template <int W>
    void CPUDevice<W>::unmapObserver(VKLObserver observer)
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer.host);
      observerObject.unmap();
    }

    template <int W>
    VKLDataType CPUDevice<W>::getObserverElementType(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer.host);
      return observerObject.getElementType();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverElementSize(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer.host);
      return observerObject.getElementSize();
    }

    template <int W>
    size_t CPUDevice<W>::getObserverNumElements(VKLObserver observer) const
    {
      auto &observerObject = referenceFromHandle<Observer<W>>(observer.host);
      return observerObject.getNumElements();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLIntervalIteratorContext CPUDevice<W>::newIntervalIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler.host);
      IntervalIteratorContext<W> *iteratorContext =
          samplerObject.getIntervalIteratorFactory().newContext(samplerObject);
      VKLIntervalIteratorContext ic;
      ic.host   = static_cast<void *>(iteratorContext);
      ic.device = static_cast<void *>(iteratorContext->getSh());
      return ic;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLHitIteratorContext CPUDevice<W>::newHitIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler.host);
      HitIteratorContext<W> *iteratorContext =
          samplerObject.getHitIteratorFactory().newContext(samplerObject);
      VKLHitIteratorContext ic;
      ic.host   = static_cast<void *>(iteratorContext);
      ic.device = static_cast<void *>(iteratorContext->getSh());
      return ic;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Sampler ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLSampler CPUDevice<W>::newSampler(VKLVolume volume)
    {
      auto &volumeObject  = referenceFromHandle<Volume<W>>(volume.host);
      Sampler<W> *sampler = volumeObject.newSampler();
      VKLSampler s;
      s.host   = static_cast<void *>(sampler);
      s.device = static_cast<void *>(sampler->getSh());
      return s;
    }

#define __define_computeSampleN(WIDTH)                                      \
  template <int W>                                                          \
  void CPUDevice<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                     \
      const VKLSampler *sampler,                                            \
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
                                      const VKLSampler *sampler,
                                      const vvec3fn<1> &objectCoordinates,
                                      float *sample,
                                      unsigned int attributeIndex,
                                      const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);
      vfloatn<1> timeW(time, 1);
      vfloatn<1> sampleW;
      samplerObject.computeSample(
          objectCoordinates, sampleW, attributeIndex, timeW);
      *sample = sampleW[0];
    }

    template <int W>
    void CPUDevice<W>::computeSampleN(const VKLSampler *sampler,
                                      unsigned int N,
                                      const vvec3fn<1> *objectCoordinates,
                                      float *samples,
                                      unsigned int attributeIndex,
                                      const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);
      samplerObject.computeSampleN(
          N, objectCoordinates, samples, attributeIndex, times);
    }

#define __define_computeSampleMN(WIDTH)              \
  template <int W>                                   \
  void CPUDevice<W>::computeSampleM##WIDTH(          \
      const int *valid,                              \
      const VKLSampler *sampler,                     \
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
                                       const VKLSampler *sampler,
                                       const vvec3fn<1> &objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *time)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);
      vfloatn<1> timeW(time, 1);
      samplerObject.computeSampleM(
          objectCoordinates, samples, M, attributeIndices, timeW);
    }

    template <int W>
    void CPUDevice<W>::computeSampleMN(const VKLSampler *sampler,
                                       unsigned int N,
                                       const vvec3fn<1> *objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);
      samplerObject.computeSampleMN(
          N, objectCoordinates, samples, M, attributeIndices, times);
    }

#define __define_computeGradientN(WIDTH)                                      \
  template <int W>                                                            \
  void CPUDevice<W>::computeGradient##WIDTH(                                  \
      const int *valid,                                                       \
      const VKLSampler *sampler,                                              \
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
    void CPUDevice<W>::computeGradientN(const VKLSampler *sampler,
                                        unsigned int N,
                                        const vvec3fn<1> *objectCoordinates,
                                        vvec3fn<1> *gradients,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);
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

      Volume<W> *volume = Volume<W>::createInstance(this, ss.str());
      VKLVolume v;
      v.host   = static_cast<void *>(volume);
      v.device = static_cast<void *>(volume->getSh());
      return v;
    }

    template <int W>
    box3f CPUDevice<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume.host);
      return volumeObject.getBoundingBox();
    }

    template <int W>
    unsigned int CPUDevice<W>::getNumAttributes(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume.host);
      return volumeObject.getNumAttributes();
    }

    template <int W>
    range1f CPUDevice<W>::getValueRange(VKLVolume volume,
                                        unsigned int attributeIndex)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume.host);
      return volumeObject.getValueRange(attributeIndex);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    CPUDevice<W>::computeSampleAnyWidth(const int *valid,
                                        const VKLSampler *sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                        const VKLSampler *sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                        const VKLSampler *sampler,
                                        const vvec3fn<OW> &objectCoordinates,
                                        float *samples,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                         const VKLSampler *sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                         const VKLSampler *sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                         const VKLSampler *sampler,
                                         const vvec3fn<OW> &objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                          const VKLSampler *sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                          const VKLSampler *sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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
                                          const VKLSampler *sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject = referenceFromHandle<Sampler<W>>(sampler->host);

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

#define VKL_MAKE_TARGET_WIDTH_NAME(name) \
  CONCAT1(name, CONCAT1(_, VKL_TARGET_WIDTH))

#define VKL_WRAP_DEVICE_REGISTRATION(internal_name)           \
  extern "C" OPENVKL_DLLEXPORT openvkl::api::Device *CONCAT1( \
      openvkl_create_device__, internal_name)();

#define VKL_WRAP_VOLUME_REGISTRATION(internal_name)                          \
  extern "C" OPENVKL_DLLEXPORT openvkl::cpu_device::Volume<VKL_TARGET_WIDTH> \
      *CONCAT1(openvkl_create_volume__, internal_name)(openvkl::api::Device* device);

VKL_WRAP_DEVICE_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_cpu))

VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_amr))
VKL_WRAP_VOLUME_REGISTRATION(
    VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegular))
VKL_WRAP_VOLUME_REGISTRATION(
    VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegularLegacy))
VKL_WRAP_VOLUME_REGISTRATION(
    VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredSpherical))
VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_unstructured))
VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_vdb))
VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_particle))

#define VKL_REGISTER_DEVICE_FACTORY_FCN(internal_name, external_name) \
  openvkl::Device::registerDevice(                                      \
      TOSTRING(external_name),                                        \
      CONCAT1(openvkl_create_device__, internal_name))

#define VKL_REGISTER_VOLUME_FACTORY_FCN(internal_name, external_name) \
  openvkl::cpu_device::Volume<VKL_TARGET_WIDTH>::registerType(        \
      TOSTRING(external_name),                                        \
      CONCAT1(openvkl_create_volume__, internal_name))

extern "C" OPENVKL_DLLEXPORT void CONCAT1(openvkl_init_module_cpu_device_,
                                          VKL_TARGET_WIDTH)()
{
  VKL_REGISTER_DEVICE_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_cpu),
                                  VKL_MAKE_TARGET_WIDTH_NAME(cpu));

#if OPENVKL_DEVICE_CPU_AMR
  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_amr),
                                  VKL_MAKE_TARGET_WIDTH_NAME(amr));
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegular),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredRegular));
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR_LEGACY
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegularLegacy),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredRegular));
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredSpherical),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredSpherical));
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_unstructured),
      VKL_MAKE_TARGET_WIDTH_NAME(unstructured));
#endif

#if OPENVKL_DEVICE_CPU_VDB
  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_vdb),
                                  VKL_MAKE_TARGET_WIDTH_NAME(vdb));
#endif

#if OPENVKL_DEVICE_CPU_PARTICLE
  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_particle),
                                  VKL_MAKE_TARGET_WIDTH_NAME(particle));
#endif

  // support deprecated snake case names (a warning will be triggered if these
  // are used)
#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegular),
      VKL_MAKE_TARGET_WIDTH_NAME(structured_regular));
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_SPHERICAL
  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredSpherical),
      VKL_MAKE_TARGET_WIDTH_NAME(structured_spherical));
#endif
}
