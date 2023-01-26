// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/IteratorBase.h"
#include "../common/ManagedObject.h"
#include "../common/logging.h"
#include "../common/simd.h"
#include "Device.h"
#include "openvkl/openvkl.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/ArrayView.h"
#include "rkcommon/utility/OnScopeExit.h"

#ifdef _WIN32
#include <process.h>  // for getpid
#endif

using namespace openvkl;

using namespace rkcommon::math;

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define THROW_IF_NULL(obj)                                            \
  if (obj == nullptr)                                                 \
  throw std::runtime_error(std::string("null ") + std::string(#obj) + \
                           std::string(" provided to ") + __FUNCTION__)

#define OPENVKL_CATCH_BEGIN_NO_DEVICE \
  {                                   \
    Device *deviceObj = nullptr;      \
    try {
#define OPENVKL_CATCH_BEGIN_SAFE(deviceSource)    \
  {                                               \
    THROW_IF_NULL(deviceSource);                  \
    Device *deviceObj = deviceFrom(deviceSource); \
    try {
#define OPENVKL_CATCH_BEGIN_SAFE2(deviceSource)                   \
  {                                                               \
    THROW_IF_NULL(deviceSource.host);                             \
    openvkl::ManagedObject *managedObject =                       \
        static_cast<openvkl::ManagedObject *>(deviceSource.host); \
    Device *deviceObj = managedObject->device.ptr;                \
    try {
#define OPENVKL_CATCH_BEGIN_UNSAFE(deviceSource)  \
  {                                               \
    assert(deviceSource != nullptr);              \
    Device *deviceObj = deviceFrom(deviceSource); \
    try {
#define OPENVKL_CATCH_BEGIN_UNSAFE2(deviceSource)                 \
  {                                                               \
    assert(deviceSource.host != nullptr);                         \
    openvkl::ManagedObject *managedObject =                       \
        static_cast<openvkl::ManagedObject *>(deviceSource.host); \
    Device *deviceObj = managedObject->device.ptr;                \
    try {
#define OPENVKL_CATCH_BEGIN_UNSAFE3(deviceSource)                  \
  {                                                                \
    assert(deviceSource != nullptr);                               \
    openvkl::ManagedObject *managedObject =                        \
        static_cast<openvkl::ManagedObject *>(deviceSource->host); \
    Device *deviceObj = managedObject->device.ptr;                 \
    try {
#define OPENVKL_CATCH_END_NO_DEVICE(a)                                         \
  }                                                                            \
  catch (const std::bad_alloc &)                                               \
  {                                                                            \
    openvkl::handleError(deviceObj,                                            \
                         VKL_OUT_OF_MEMORY,                                    \
                         "Open VKL was unable to allocate memory");            \
    return a;                                                                  \
  }                                                                            \
  catch (const std::exception &e)                                              \
  {                                                                            \
    openvkl::handleError(deviceObj, VKL_UNKNOWN_ERROR, e.what());              \
    return a;                                                                  \
  }                                                                            \
  catch (...)                                                                  \
  {                                                                            \
    openvkl::handleError(                                                      \
        deviceObj, VKL_UNKNOWN_ERROR, "an unrecognized exception was caught"); \
    return a;                                                                  \
  }                                                                            \
  }

#define OPENVKL_CATCH_END(a) \
  assert(deviceObj);         \
  OPENVKL_CATCH_END_NO_DEVICE(a)

///////////////////////////////////////////////////////////////////////////////
// Device helpers /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline Device *deviceFrom(VKLObject object)
{
  return reinterpret_cast<openvkl::ManagedObject *>(object)->device.ptr;
}

inline Device *deviceFrom(VKLDevice device)
{
  return reinterpret_cast<Device *>(device);
}

///////////////////////////////////////////////////////////////////////////////
// Data ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLData vklNewData(VKLDevice device,
                              size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags,
                              size_t byteStride)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (!deviceObj->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  VKLData data = deviceObj->newData(
      numItems, dataType, source, dataCreationFlags, byteStride);
  return data;
}
OPENVKL_CATCH_END(VKLData())

///////////////////////////////////////////////////////////////////////////////
// Observer ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLObserver vklNewVolumeObserver(VKLVolume volume, const char *type)
    OPENVKL_CATCH_BEGIN_SAFE2(volume)
{
  THROW_IF_NULL(type);
  VKLObserver observer = deviceObj->newVolumeObserver(volume, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  return observer;
}
OPENVKL_CATCH_END(VKLObserver())

extern "C" VKLObserver vklNewSamplerObserver(VKLSampler sampler,
                                             const char *type)
    OPENVKL_CATCH_BEGIN_SAFE2(sampler)
{
  THROW_IF_NULL(type);
  VKLObserver observer = deviceObj->newSamplerObserver(sampler, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  return observer;
}
OPENVKL_CATCH_END(VKLObserver())

extern "C" const void *vklMapObserver(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE2(observer)
{
  const void *mapped = deviceObj->mapObserver(observer);
  return mapped;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklUnmapObserver(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE2(observer)
{
  deviceObj->unmapObserver(observer);
}
OPENVKL_CATCH_END()

extern "C" VKLDataType vklGetObserverElementType(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE2(observer)
{
  VKLDataType type = deviceObj->getObserverElementType(observer);
  return type;
}
OPENVKL_CATCH_END(VKL_UNKNOWN)

extern "C" size_t vklGetObserverElementSize(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE2(observer)
{
  size_t size = deviceObj->getObserverElementSize(observer);
  return size;
}
OPENVKL_CATCH_END(0)

extern "C" size_t vklGetObserverNumElements(VKLObserver observer)
    OPENVKL_CATCH_BEGIN_SAFE2(observer)
{
  size_t numElements = deviceObj->getObserverNumElements(observer);
  return numElements;
}
OPENVKL_CATCH_END(0)

///////////////////////////////////////////////////////////////////////////////
// Device /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLDevice vklNewDevice(const char *deviceName)
    OPENVKL_CATCH_BEGIN_NO_DEVICE
{
  THROW_IF_NULL(deviceName);
  return (VKLDevice)Device::createDevice(deviceName);
}
OPENVKL_CATCH_END_NO_DEVICE(nullptr)

extern "C" void vklDeviceSetLogCallback(VKLDevice device,
                                        VKLLogCallback callback,
                                        void *userData)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (callback == nullptr) {
    deviceObj->logCallback = [](void *, const char *) {};
    deviceObj->logUserData = nullptr;
  } else {
    deviceObj->logCallback = callback;
    deviceObj->logUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetErrorCallback(VKLDevice device,
                                          VKLErrorCallback callback,
                                          void *userData)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  if (callback == nullptr) {
    deviceObj->errorCallback = [](void *, VKLError, const char *) {};
    deviceObj->errorUserData = nullptr;
  } else {
    deviceObj->errorCallback = callback;
    deviceObj->errorUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetInt(VKLDevice device, const char *name, int x)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(name);
  deviceObj->setParam<int>(name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetString(VKLDevice device,
                                   const char *name,
                                   const char *s)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(name);
  deviceObj->setParam<std::string>(name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetVoidPtr(VKLDevice device,
                                    const char *name,
                                    const void *a)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(name);
  deviceObj->setParam<const void *>(name, a);
}
OPENVKL_CATCH_END()

extern "C" void vklCommitDevice(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  deviceObj->commit();
}
OPENVKL_CATCH_END()

extern "C" VKLError vklDeviceGetLastErrorCode(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->lastErrorCode;
}
OPENVKL_CATCH_END(VKL_NO_ERROR)

extern "C" const char *vklDeviceGetLastErrorMsg(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->lastErrorMessage.c_str();
}
OPENVKL_CATCH_END(nullptr)

extern "C" int vklGetNativeSIMDWidth(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  return deviceObj->getNativeSIMDWidth();
}
OPENVKL_CATCH_END(0)

extern "C" void vklCommit(VKLObject object) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  deviceObj->commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklCommit2(APIObject object) OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  deviceObj->commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease(VKLObject object) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  deviceObj->release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease2(APIObject object) OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  deviceObj->release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklReleaseDevice(VKLDevice device)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  auto *object = (Device *)device;
  object->refDec();
}
OPENVKL_CATCH_END_NO_DEVICE()

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLIntervalIteratorContext vklNewIntervalIteratorContext(
    VKLSampler sampler) OPENVKL_CATCH_BEGIN_UNSAFE2(sampler)
{
  VKLIntervalIteratorContext context =
      deviceObj->newIntervalIteratorContext(sampler);
  if (!context) {
    postLogMessage(deviceObj, VKL_LOG_ERROR)
        << "could not create interval iterator context";
  }
  return context;
}
OPENVKL_CATCH_END(VKLIntervalIteratorContext())

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLHitIteratorContext vklNewHitIteratorContext(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN_UNSAFE2(sampler)
{
  VKLHitIteratorContext context = deviceObj->newHitIteratorContext(sampler);
  if (!context) {
    postLogMessage(deviceObj, VKL_LOG_ERROR)
        << "could not create hit iterator context";
  }
  return context;
}
OPENVKL_CATCH_END(VKLHitIteratorContext())

///////////////////////////////////////////////////////////////////////////////
// Parameters /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklSetBool(VKLObject object, const char *name, int b)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setBool(object, name, static_cast<bool>(b));
}
OPENVKL_CATCH_END()

extern "C" void vklSetFloat(VKLObject object, const char *name, float x)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3f(VKLObject object,
                            const char *name,
                            float x,
                            float y,
                            float z) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetInt(VKLObject object, const char *name, int x)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3i(VKLObject object,
                            const char *name,
                            int x,
                            int y,
                            int z) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData(VKLObject object, const char *name, VKLData data)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString(VKLObject object, const char *name, const char *s)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr(VKLObject object, const char *name, void *v)
    OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

extern "C" void vklSetParam(VKLObject object,
                            const char *name,
                            VKLDataType dataType,
                            const void *mem) OPENVKL_CATCH_BEGIN_SAFE(object)
{
  THROW_IF_NULL(name);
  deviceObj->setObjectParam(object, name, dataType, mem);
}
OPENVKL_CATCH_END()

// New parameter setters for APIObject ////////////////////////////////////////

extern "C" void vklSetBool2(APIObject object, const char *name, int b)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setBool(object, name, static_cast<bool>(b));
}
OPENVKL_CATCH_END()

extern "C" void vklSetFloat2(APIObject object, const char *name, float x)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3f2(APIObject object,
                             const char *name,
                             float x,
                             float y,
                             float z) OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetInt2(APIObject object, const char *name, int x)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3i2(APIObject object,
                             const char *name,
                             int x,
                             int y,
                             int z) OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData2(APIObject object, const char *name, VKLData data)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString2(APIObject object, const char *name, const char *s)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr2(APIObject object, const char *name, void *v)
    OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

extern "C" void vklSetParam2(APIObject object,
                             const char *name,
                             VKLDataType dataType,
                             const void *mem) OPENVKL_CATCH_BEGIN_SAFE2(object)
{
  THROW_IF_NULL(name);
  deviceObj->setObjectParam(object, name, dataType, mem);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Sampler ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLSampler vklNewSampler(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE2(volume)
{
  VKLSampler sampler = deviceObj->newSampler(volume);
  if (!sampler) {
    postLogMessage(deviceObj, VKL_LOG_ERROR) << "could not create sampler";
  }
  return sampler;
}
OPENVKL_CATCH_END(VKLSampler())

///////////////////////////////////////////////////////////////////////////////
// Volume /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLVolume vklNewVolume(VKLDevice device, const char *type)
    OPENVKL_CATCH_BEGIN_SAFE(device)
{
  THROW_IF_NULL(type);

  if (!deviceObj->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  VKLVolume volume = deviceObj->newVolume(type);
  if (!volume) {
    postLogMessage(deviceObj, VKL_LOG_ERROR)
        << "could not create volume '" << type << "'";
  }
  return volume;
}
OPENVKL_CATCH_END(VKLVolume())

extern "C" vkl_box3f vklGetBoundingBox(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE2(volume)
{
  const box3f result = deviceObj->getBoundingBox(volume);
  return reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END(vkl_box3f{rkcommon::math::nan})

extern "C" unsigned int vklGetNumAttributes(VKLVolume volume)
    OPENVKL_CATCH_BEGIN_UNSAFE2(volume)
{
  return deviceObj->getNumAttributes(volume);
}
OPENVKL_CATCH_END(0)

extern "C" vkl_range1f vklGetValueRange(VKLVolume volume,
                                        unsigned int attributeIndex)
    OPENVKL_CATCH_BEGIN_UNSAFE2(volume)
{
  const range1f result = deviceObj->getValueRange(volume, attributeIndex);
  return reinterpret_cast<const vkl_range1f &>(result);
}
OPENVKL_CATCH_END(vkl_range1f{rkcommon::math::nan})

// For use from ISPC.
//
// We need to avoid returning structs from extern functions called from ISPC,
// see: https://github.com/ispc/ispc/issues/1590 and
// https://github.com/ispc/ispc/issues/2106

extern "C" void vklGetBoundingBoxRef(const VKLVolume *volume,
                                     vkl_box3f *boundingBox)
    OPENVKL_CATCH_BEGIN_UNSAFE2((*volume))
{
  THROW_IF_NULL(boundingBox);
  const box3f result = deviceObj->getBoundingBox(*volume);
  *boundingBox       = reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END()

extern "C" unsigned int vklGetNumAttributesRef(const VKLVolume *volume)
    OPENVKL_CATCH_BEGIN_UNSAFE2((*volume))
{
  return deviceObj->getNumAttributes(*volume);
}
OPENVKL_CATCH_END(0)

extern "C" void vklGetValueRangeRef(const VKLVolume *volume,
                                    unsigned int attributeIndex,
                                    vkl_range1f *valueRange)
    OPENVKL_CATCH_BEGIN_UNSAFE2((*volume))
{
  THROW_IF_NULL(valueRange);
  const range1f result = deviceObj->getValueRange(*volume, attributeIndex);
  *valueRange          = reinterpret_cast<const vkl_range1f &>(result);
}
OPENVKL_CATCH_END()
