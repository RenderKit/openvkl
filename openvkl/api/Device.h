// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <functional>
#include <string>
#include "../common/ObjectFactory.h"
#include "../common/VKLCommon.h"
#include "../common/VKLFeatureFlagsInternal.h"
#include "../common/simd.h"
#include "openvkl/openvkl.h"
#include "rkcommon/memory/IntrusivePtr.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/ParameterizedObject.h"

#ifdef NDEBUG
#define LOG_LEVEL_DEFAULT VKL_LOG_INFO
#else
#define LOG_LEVEL_DEFAULT VKL_LOG_DEBUG
#endif

namespace openvkl {

  enum DeviceType
  {
    OPENVKL_DEVICE_TYPE_UNKNOWN = 0,
    OPENVKL_DEVICE_TYPE_CPU,
    OPENVKL_DEVICE_TYPE_GPU,
  };

  enum AllocType
  {
    OPENVKL_ALLOC_TYPE_UNKNOWN = 0,
    OPENVKL_ALLOC_TYPE_HOST,
    OPENVKL_ALLOC_TYPE_DEVICE,
    OPENVKL_ALLOC_TYPE_SHARED,
  };

  namespace api {

    typedef struct
    {
      void *allocatedBuffer;
      void *privateManagement;
    } memstate;

    struct OPENVKL_CORE_INTERFACE Device
        : public rkcommon::memory::RefCountedObject,
          public rkcommon::utility::ParameterizedObject
    {
      Device();
      Device(const Device &)            = delete;
      Device &operator=(const Device &) = delete;
      virtual ~Device() override        = default;

      static Device *createDevice(const std::string &deviceName);
      static void registerDevice(const std::string &type,
                                 FactoryDeviceFcn<Device> f);

      // error tracking
      VKLError lastErrorCode       = VKL_NO_ERROR;
      std::string lastErrorMessage = "no error";

      virtual bool supportsWidth(int width) = 0;

      virtual int getNativeSIMDWidth() = 0;

      virtual void commit();
      bool isCommitted();

      virtual void commit(VKLObject object)  = 0;
      virtual void release(VKLObject object) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Device parameters (updated on commit()) //////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      VKLLogLevel logLevel = LOG_LEVEL_DEFAULT;

      std::function<void(void *, const char *)> logCallback{
          [](void *, const char *) {}};
      void *logUserData{nullptr};

      std::function<void(void *, VKLError, const char *)> errorCallback{
          [](void *, VKLError, const char *) {}};
      void *errorUserData{nullptr};

      /////////////////////////////////////////////////////////////////////////
      // Data /////////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLData newData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags,
                              size_t byteStride) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Observer /////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLObserver newVolumeObserver(VKLVolume volume,
                                            const char *type)  = 0;
      virtual VKLObserver newSamplerObserver(VKLSampler sampler,
                                             const char *type) = 0;
      virtual const void *mapObserver(VKLObserver observer)    = 0;
      virtual void unmapObserver(VKLObserver observer)         = 0;
      virtual VKLDataType getObserverElementType(
          VKLObserver observer) const                                   = 0;
      virtual size_t getObserverElementSize(VKLObserver observer) const = 0;
      virtual size_t getObserverNumElements(VKLObserver observer) const = 0;

      /////////////////////////////////////////////////////////////////////////
      // Interval iterator ////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLIntervalIteratorContext newIntervalIteratorContext(
          VKLSampler sampler) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Hit iterator /////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLHitIteratorContext newHitIteratorContext(
          VKLSampler sampler) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Parameters ///////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      void setBool(VKLObject object, const char *name, const bool b);
      void set1f(VKLObject object, const char *name, const float x);
      void set1i(VKLObject object, const char *name, const int x);
      void setVec3f(VKLObject object, const char *name, const vec3f &v);
      void setVec3i(VKLObject object, const char *name, const vec3i &v);
      void setObject(VKLObject object,
                     const char *name,
                     ManagedObject *setObject);
      void setString(VKLObject object, const char *name, const std::string &s);
      void setVoidPtr(VKLObject object, const char *name, void *v);

      void setObjectParam(VKLObject object,
                          const char *name,
                          VKLDataType dataType,
                          const void *mem);

      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLSampler newSampler(VKLVolume volume) = 0;

      virtual VKLFeatureFlagsInternal getFeatureFlags(VKLSampler sampler) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Volume ///////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual VKLVolume newVolume(const char *type) = 0;

      virtual box3f getBoundingBox(VKLVolume volume) = 0;

      virtual unsigned int getNumAttributes(VKLVolume volume) = 0;

      virtual range1f getValueRange(VKLVolume volume,
                                    unsigned int attributeIndex) = 0;

      /////////////////////////////////////////////////////////////////////////
      // Hardware facilities //////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual DeviceType getDeviceType() const = 0;

      virtual AllocType getAllocationType(const void *ptr) const = 0;

      virtual void *getContext() const = 0;

      virtual memstate *allocateBytes(size_t numBytes) const = 0;
      virtual void freeMemState(memstate *) const            = 0;

      virtual char *copyDeviceBufferToHost(size_t numItems,
                                           VKLDataType dataType,
                                           const void *source,
                                           size_t byteStride)
      {
        throw std::runtime_error("not implemented for this device");
      }

     private:
      bool committed = false;
    };

#define VKL_REGISTER_DEVICE(InternalClass, external_name) \
  VKL_REGISTER_DEVICEOBJECT(                              \
      ::openvkl::api::Device, device, InternalClass, external_name)

  }  // namespace api
}  // namespace openvkl
