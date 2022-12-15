// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "GPUDevice.h"
#include "../../cpu/common/export_util.h"
#include "../../cpu/common/ispc_isa.h"
#include "../../cpu/iterator/Iterator.h"
#include "../../cpu/sampler/Sampler.h"
#include "../../cpu/volume/Volume.h"
#include "../common/Data.h"
#include "../common/ObjectFactory.h"
#include "../compute/vklComputeSample.h"
#include "../compute/vklIterateInterval.h"

namespace openvkl {
  namespace gpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // GPUDevice //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    GPUDevice<W>::~GPUDevice()
    {
      delete (ispcrt::Context *)context;
    }

    template <int W>
    api::memstate *GPUDevice<W>::allocateBytes(size_t numBytes) const
    {
      api::memstate *container = new api::memstate;
      container->privateManagement =
          (void *)BufferSharedCreate((Device *)this, numBytes + 16);
      void *buffer =
          ispcrtSharedPtr((ISPCRTMemoryView)container->privateManagement);
      container->allocatedBuffer = buffer;
      return container;
    }

    template <int W>
    void GPUDevice<W>::freeMemState(api::memstate *container) const
    {
      delete (int *)container->privateManagement;
      delete container;
    }

    template <int W>
    bool GPUDevice<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    int GPUDevice<W>::getNativeSIMDWidth()
    {
      return W;
    }

    template <int W>
    void GPUDevice<W>::commit()
    {
      Device::commit();

      sycl::context *syclContext =
          (sycl::context *)getParam<const void *>("syclContext", nullptr);

      if (syclContext == nullptr) {
        throw std::runtime_error(
            "GPU device type can't be used without 'syclContext' param");
      }

      // nativeContext is a pointer - it's owned by syclContext so no need to
      // care about life cycle for this variable here.
      ze_context_handle_t nativeContext =
          sycl::get_native<sycl::backend::ext_oneapi_level_zero>(*syclContext);

      if (context) {
        delete (ispcrt::Context *)context;
      }
      context = new ispcrt::Context(ISPCRT_DEVICE_TYPE_GPU, nativeContext);
    }

    template <int W>
    void GPUDevice<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void GPUDevice<W>::release(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    template <int W>
    void GPUDevice<W>::commit(APIObject object)
    {
      ManagedObject *managedObject = static_cast<ManagedObject *>(object.host);
      managedObject->commit();
    }

    template <int W>
    void GPUDevice<W>::release(APIObject object)
    {
      ManagedObject *managedObject = static_cast<ManagedObject *>(object.host);
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLData GPUDevice<W>::newData(size_t numItems,
                                  VKLDataType dataType,
                                  const void *source,
                                  VKLDataCreationFlags dataCreationFlags,
                                  size_t byteStride)
    {
      Data *data = new Data(
          this, numItems, dataType, source, dataCreationFlags, byteStride);
      return (VKLData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLIntervalIteratorContext GPUDevice<W>::newIntervalIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject =
          referenceFromHandle<openvkl::cpu_device::Sampler<W>>(sampler.host);
      auto iteratorContext =
          samplerObject.getIntervalIteratorFactory().newContext(samplerObject);
      VKLIntervalIteratorContext ic;
      ic.host   = static_cast<void *>(iteratorContext);
      ic.device = static_cast<void *>(iteratorContext->getSh());
      return ic;
    }

    template <int W>
    size_t GPUDevice<W>::getIntervalIteratorSize1(
        const VKLIntervalIteratorContext *context) const
    {
      // the size includes extra padding, so that we can still use an unaligned
      // buffer allocated by the application
      return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Sampler ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLSampler GPUDevice<W>::newSampler(VKLVolume volume)
    {
      auto &volumeObject =
          referenceFromHandle<openvkl::cpu_device::Volume<W>>(volume.host);
      openvkl::cpu_device::Sampler<W> *sampler = volumeObject.newSampler();
      VKLSampler s;
      s.host   = static_cast<void *>(sampler);
      s.device = static_cast<void *>(sampler->getSh());
      return s;
    }

    template <int W>
    void GPUDevice<W>::computeSampleM1(const int *valid,
                                       const VKLSampler *sampler,
                                       const vvec3fn<1> &objectCoordinates,
                                       float *samples,
                                       unsigned int M,
                                       const unsigned int *attributeIndices,
                                       const float *time)
    {
      auto &samplerObject =
          referenceFromHandle<openvkl::cpu_device::Sampler<W>>(sampler->host);
      vfloatn<1> timeW(time, 1);
      samplerObject.computeSampleM(
          objectCoordinates, samples, M, attributeIndices, timeW);
    }

    template <int W>
    void GPUDevice<W>::computeGradient1(const int *valid,
                                        const VKLSampler *sampler,
                                        const vvec3fn<1> &objectCoordinates,
                                        vvec3fn<1> &gradients,
                                        unsigned int attributeIndex,
                                        const float *times)
    {
      computeGradientAnyWidth<1>(
          valid, sampler, objectCoordinates, gradients, attributeIndex, times);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume GPUDevice<W>::newVolume(const char *type)
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

      openvkl::cpu_device::Volume<W> *volume =
          openvkl::cpu_device::Volume<W>::createInstance(this, ss.str());
      VKLVolume v;
      v.host   = static_cast<void *>(volume);
      v.device = static_cast<void *>(volume->getSh());
      return v;
    }

    template <int W>
    box3f GPUDevice<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject =
          referenceFromHandle<openvkl::cpu_device::Volume<W>>(volume.host);
      return volumeObject.getBoundingBox();
    }

    template <int W>
    unsigned int GPUDevice<W>::getNumAttributes(VKLVolume volume)
    {
      auto &volumeObject =
          referenceFromHandle<openvkl::cpu_device::Volume<W>>(volume.host);
      return volumeObject.getNumAttributes();
    }

    template <int W>
    range1f GPUDevice<W>::getValueRange(VKLVolume volume,
                                        unsigned int attributeIndex)
    {
      auto &volumeObject =
          referenceFromHandle<openvkl::cpu_device::Volume<W>>(volume.host);
      return volumeObject.getValueRange(attributeIndex);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW < W), void>::type
    GPUDevice<W>::computeGradientAnyWidth(const int *valid,
                                          const VKLSampler *sampler,
                                          const vvec3fn<OW> &objectCoordinates,
                                          vvec3fn<OW> &gradients,
                                          unsigned int attributeIndex,
                                          const float *times)
    {
      auto &samplerObject =
          referenceFromHandle<openvkl::cpu_device::Sampler<W>>(sampler->host);

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

    VKL_REGISTER_DEVICE(GPUDevice<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_gpu_, VKL_TARGET_WIDTH))

  }  // namespace gpu_device
}  // namespace openvkl

#define VKL_MAKE_TARGET_WIDTH_NAME(name) \
  CONCAT1(name, CONCAT1(_, VKL_TARGET_WIDTH))

#define VKL_WRAP_DEVICE_REGISTRATION(internal_name)           \
  extern "C" OPENVKL_DLLEXPORT openvkl::api::Device *CONCAT1( \
      openvkl_create_device__, internal_name)();

#define VKL_WRAP_VOLUME_REGISTRATION(internal_name)                          \
  extern "C" OPENVKL_DLLEXPORT openvkl::cpu_device::Volume<VKL_TARGET_WIDTH> \
      *CONCAT1(openvkl_create_volume__,                                      \
               internal_name)(openvkl::api::Device * context);

VKL_WRAP_DEVICE_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_gpu))

VKL_WRAP_VOLUME_REGISTRATION(
    VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegularLegacy))

#define VKL_REGISTER_DEVICE_FACTORY_FCN(internal_name, external_name) \
  openvkl::Device::registerDevice(                                    \
      TOSTRING(external_name),                                        \
      CONCAT1(openvkl_create_device__, internal_name))

#define VKL_REGISTER_VOLUME_FACTORY_FCN(internal_name, external_name) \
  openvkl::cpu_device::Volume<VKL_TARGET_WIDTH>::registerType(        \
      TOSTRING(external_name),                                        \
      CONCAT1(openvkl_create_volume__, internal_name))

extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_gpu_device()
{
  VKL_REGISTER_DEVICE_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_gpu),
                                  VKL_MAKE_TARGET_WIDTH_NAME(gpu));

  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegularLegacy),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredRegular));
}
