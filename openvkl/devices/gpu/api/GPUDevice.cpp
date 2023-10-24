// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
using namespace ispc;

#include "../../cpu/common/export_util.h"
#include "../../cpu/common/ispc_isa.h"
#include "../../cpu/iterator/Iterator.h"
#include "../../cpu/sampler/Sampler.h"
#include "../../cpu/volume/Volume.h"
#include "../common/Data.h"
#include "../common/ObjectFactory.h"
#include "../compute/vklCompute.h"
#include "../compute/vklIterators.h"
#include "GPUDevice.h"
#include "rkcommon/utility/getEnvVar.h"

namespace openvkl {
  namespace gpu_device {

    ///////////////////////////////////////////////////////////////////////////
    // GPUDevice //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    AllocType GPUDevice<W>::getAllocationType(const void *ptr) const
    {
      sycl::usm::alloc allocType = sycl::get_pointer_type(ptr, syclContext);

      switch (allocType) {
      case sycl::usm::alloc::host:
        return OPENVKL_ALLOC_TYPE_HOST;

      case sycl::usm::alloc::device:
        return OPENVKL_ALLOC_TYPE_DEVICE;

      case sycl::usm::alloc::shared:
        return OPENVKL_ALLOC_TYPE_SHARED;

      default:
        return OPENVKL_ALLOC_TYPE_UNKNOWN;
      }
    }

    template <int W>
    void *GPUDevice<W>::allocateSharedMemory(size_t numBytes,
                                             size_t alignment) const
    {
      return (void *)sycl::aligned_alloc_shared<char>(
          alignment, numBytes, syclDevice, syclContext);
    }

    template <int W>
    void GPUDevice<W>::freeSharedMemory(void *ptr) const
    {
      sycl::free(ptr, syclContext);
    }

    template <int W>
    char *GPUDevice<W>::copyDeviceBufferToHost(size_t numItems,
                                               VKLDataType dataType,
                                               const void *source,
                                               size_t byteStride)
    {
      // verify pointer type
      sycl::usm::alloc allocType = sycl::get_pointer_type(source, syclContext);

      if (allocType != sycl::usm::alloc::device) {
        throw std::runtime_error("GPUDevice: pointer is NOT a device pointer");
      }

      // get device from pointer
      sycl::device syclDevice;

      try {
        syclDevice = sycl::get_pointer_device(source, syclContext);
      } catch (std::exception &e) {
        throw std::runtime_error(
            "could not determine SYCL device used for 'source'");
      }

      sycl::queue queue = sycl::queue(syclDevice);

      char *destination = new char[numItems * sizeOf(dataType)];

      const bool isCompact = byteStride == sizeOf(dataType);

      if (isCompact) {
        queue.memcpy(destination, source, numItems * sizeof(dataType));
        queue.wait();
      } else {
        throw std::runtime_error("not implemented");
      }

      return destination;
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

      sycl::context *c =
          (sycl::context *)getParam<const void *>("syclContext", nullptr);

      if (c == nullptr) {
        throw std::runtime_error(
            "SYCL device type can't be used without 'syclContext' parameter");
      }

      // SYCL contexts are normally stored in the application by-value, so
      // save a copy here in case it disappears from the application's stack
      syclContext = *c;

      sycl::device *device =
          (sycl::device *)getParam<const void *>("syclDevice", nullptr);

      if (device == nullptr) {
        // take first device as default device
        auto devices = syclContext.get_devices();
        if (devices.size() == 0)
          throw std::runtime_error("SYCL context contains no device");
        syclDevice = devices[0];
      } else {
        syclDevice = *device;
      }
    }

    template <int W>
    void GPUDevice<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = static_cast<ManagedObject *>(object.host);
      managedObject->commit();
    }

    template <int W>
    void GPUDevice<W>::release(VKLObject object)
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

      VKLData d;
      d.host   = data;
      d.device = nullptr;

      return d;
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

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLHitIteratorContext GPUDevice<W>::newHitIteratorContext(
        VKLSampler sampler)
    {
      auto &samplerObject =
          referenceFromHandle<openvkl::cpu_device::Sampler<W>>(sampler.host);
      auto iteratorContext =
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
    VKLFeatureFlagsInternal GPUDevice<W>::getFeatureFlags(VKLSampler sampler)
    {
      auto &samplerObject =
          referenceFromHandle<openvkl::cpu_device::Sampler<W>>(sampler.host);

      return samplerObject.getFeatureFlags();
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

VKL_WRAP_VOLUME_REGISTRATION(
    VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredSpherical))

VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_unstructured))

VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_particle))

VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_vdb))

VKL_WRAP_VOLUME_REGISTRATION(VKL_MAKE_TARGET_WIDTH_NAME(internal_amr))

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

  // on GPU, we still build the C++ code with a templated width, but we only
  // build _one_ device library (instead of several like for the CPU device).
  // Given this, also register this device as "gpu" without a width suffix,
  // allowing vklNewDevice("gpu") to work.
  VKL_REGISTER_DEVICE_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_gpu),
                                  gpu);

  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredRegularLegacy),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredRegular));

  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_structuredSpherical),
      VKL_MAKE_TARGET_WIDTH_NAME(structuredSpherical));

  VKL_REGISTER_VOLUME_FACTORY_FCN(
      VKL_MAKE_TARGET_WIDTH_NAME(internal_unstructured),
      VKL_MAKE_TARGET_WIDTH_NAME(unstructured));

  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_particle),
                                  VKL_MAKE_TARGET_WIDTH_NAME(particle));

  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_vdb),
                                  VKL_MAKE_TARGET_WIDTH_NAME(vdb));

  VKL_REGISTER_VOLUME_FACTORY_FCN(VKL_MAKE_TARGET_WIDTH_NAME(internal_amr),
                                  VKL_MAKE_TARGET_WIDTH_NAME(amr));
}
