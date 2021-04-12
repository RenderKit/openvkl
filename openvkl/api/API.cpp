// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

inline std::string getPidString()
{
  char s[100];
  sprintf(s, "(pid %i)", getpid());
  return s;
}

#define ASSERT_DEVICE()                                                      \
  if (!openvkl::api::deviceIsSet())                                          \
    throw std::runtime_error(                                                \
        "Open VKL not yet initialized (most likely this means you tried to " \
        "call an Open VKL API function before first setting a device)" +     \
        getPidString());

#define ASSERT_DEVICE_SUPPORTS_WIDTH(WIDTH)                                  \
  if (!openvkl::api::currentDevice().supportsWidth(WIDTH))                   \
    throw std::runtime_error(                                                \
        "the current Open VKL device does not support the requested vector " \
        "width " +                                                           \
        std::string(#WIDTH));

#define THROW_IF_NULL(obj, name)                         \
  if (obj == nullptr)                                    \
  throw std::runtime_error(std::string("null ") + name + \
                           std::string(" provided to ") + __FUNCTION__)

// convenience macros for repeated use of the above
#define THROW_IF_NULL_OBJECT(obj) THROW_IF_NULL(obj, "handle")
#define THROW_IF_NULL_STRING(str) THROW_IF_NULL(str, "string")

#define OPENVKL_CATCH_BEGIN try {
#define OPENVKL_CATCH_END(a)                                        \
  }                                                                 \
  catch (const std::bad_alloc &)                                    \
  {                                                                 \
    openvkl::handleError(VKL_OUT_OF_MEMORY,                         \
                         "Open VKL was unable to allocate memory"); \
    return a;                                                       \
  }                                                                 \
  catch (const std::exception &e)                                   \
  {                                                                 \
    openvkl::handleError(VKL_UNKNOWN_ERROR, e.what());              \
    return a;                                                       \
  }                                                                 \
  catch (...)                                                       \
  {                                                                 \
    openvkl::handleError(VKL_UNKNOWN_ERROR,                         \
                         "an unrecognized exception was caught");   \
    return a;                                                       \
  }

///////////////////////////////////////////////////////////////////////////////
// Data ///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLData vklNewData(size_t numItems,
                              VKLDataType dataType,
                              const void *source,
                              VKLDataCreationFlags dataCreationFlags,
                              size_t byteStride) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  VKLData data = openvkl::api::currentDevice().newData(
      numItems, dataType, source, dataCreationFlags, byteStride);
  return data;
}
OPENVKL_CATCH_END(nullptr)

///////////////////////////////////////////////////////////////////////////////
// Observer ///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLObserver vklNewVolumeObserver(VKLVolume volume, const char *type)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(volume);
  THROW_IF_NULL_OBJECT(type);
  VKLObserver observer =
      openvkl::api::currentDevice().newObserver(volume, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  return observer;
}
OPENVKL_CATCH_END(nullptr)

extern "C" VKLObserver vklNewSamplerObserver(
    VKLSampler sampler, const char *type) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(sampler);
  THROW_IF_NULL_OBJECT(type);
  VKLObserver observer =
      openvkl::api::currentDevice().newObserver(sampler, type);
  if (!observer)
    throw std::runtime_error(std::string("unsupported observer type: ") + type);
  return observer;
}
OPENVKL_CATCH_END(nullptr)

extern "C" const void *vklMapObserver(VKLObserver observer) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(observer);
  const void *mapped = openvkl::api::currentDevice().mapObserver(observer);
  return mapped;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklUnmapObserver(VKLObserver observer) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(observer);
  openvkl::api::currentDevice().unmapObserver(observer);
}
OPENVKL_CATCH_END()

extern "C" VKLDataType vklGetObserverElementType(VKLObserver observer)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(observer);
  VKLDataType type =
      openvkl::api::currentDevice().getObserverElementType(observer);
  return type;
}
OPENVKL_CATCH_END(VKL_UNKNOWN)

extern "C" size_t vklGetObserverElementSize(VKLObserver observer)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(observer);
  size_t size = openvkl::api::currentDevice().getObserverElementSize(observer);
  return size;
}
OPENVKL_CATCH_END(0)

extern "C" size_t vklGetObserverNumElements(VKLObserver observer)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(observer);
  size_t numElements =
      openvkl::api::currentDevice().getObserverNumElements(observer);
  return numElements;
}
OPENVKL_CATCH_END(0)

///////////////////////////////////////////////////////////////////////////////
// Device /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLDevice vklNewDevice(const char *deviceName) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(deviceName);
  return (VKLDevice)openvkl::api::Device::createDevice(deviceName);
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklDeviceSetLogCallback(VKLDevice device,
                                        VKLLogCallback callback,
                                        void *userData) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  if (callback == nullptr) {
    object->logCallback = [](void *, const char *) {};
    object->logUserData = nullptr;
  } else {
    object->logCallback = callback;
    object->logUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetErrorCallback(VKLDevice device,
                                          VKLErrorCallback callback,
                                          void *userData) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  if (callback == nullptr) {
    object->errorCallback = [](void *, VKLError, const char *) {};
    object->errorUserData = nullptr;
  } else {
    object->errorCallback = callback;
    object->errorUserData = userData;
  }
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetInt(VKLDevice device,
                                const char *name,
                                int x) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  THROW_IF_NULL_STRING(name);

  object->setParam<int>(name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklDeviceSetString(VKLDevice device,
                                   const char *name,
                                   const char *s) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  THROW_IF_NULL_STRING(name);

  object->setParam<std::string>(name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklCommitDevice(VKLDevice device) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;
  object->commit();
}
OPENVKL_CATCH_END()

extern "C" void vklSetCurrentDevice(VKLDevice device) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  if (!object->isCommitted()) {
    throw std::runtime_error("You must commit the device before using it!");
  }

  openvkl::api::Device::current = object;
  object->refDec();
}
OPENVKL_CATCH_END()

extern "C" VKLDevice vklGetCurrentDevice() OPENVKL_CATCH_BEGIN
{
  return (VKLDevice)openvkl::api::Device::current.ptr;
}
OPENVKL_CATCH_END(nullptr)

extern "C" VKLError vklDeviceGetLastErrorCode(VKLDevice device)
    OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  return object->lastErrorCode;
}
OPENVKL_CATCH_END(VKL_NO_ERROR)

extern "C" const char *vklDeviceGetLastErrorMsg(VKLDevice device)
    OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_OBJECT(device);
  auto *object = (openvkl::api::Device *)device;

  return object->lastErrorMessage.c_str();
}
OPENVKL_CATCH_END(nullptr)

extern "C" int vklGetNativeSIMDWidth() OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return openvkl::api::currentDevice().getNativeSIMDWidth();
}
OPENVKL_CATCH_END(0)

extern "C" void vklCommit(VKLObject object) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  openvkl::api::currentDevice().commit(object);
}
OPENVKL_CATCH_END()

extern "C" void vklRelease(VKLObject object) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  openvkl::api::currentDevice().release(object);
}
OPENVKL_CATCH_END()

extern "C" void vklShutdown() OPENVKL_CATCH_BEGIN
{
  openvkl::api::Device::current = nullptr;
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" size_t vklGetIntervalIteratorSize(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return openvkl::api::currentDevice().getIntervalIteratorSize1(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize4(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return openvkl::api::currentDevice().getIntervalIteratorSize4(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize8(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return openvkl::api::currentDevice().getIntervalIteratorSize8(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize16(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  return openvkl::api::currentDevice().getIntervalIteratorSize16(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLIntervalIterator vklInitIntervalIterator(
    VKLSampler sampler,
    const vkl_vec3f *origin,
    const vkl_vec3f *direction,
    const vkl_range1f *tRange,
    VKLValueSelector valueSelector,
    void *buffer) OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().initIntervalIterator1(
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      valueSelector,
      buffer);
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitIntervalIteratorN(WIDTH)                        \
  extern "C" VKLIntervalIterator##WIDTH vklInitIntervalIterator##WIDTH( \
      const int *valid,                                                 \
      VKLSampler sampler,                                               \
      const vkl_vvec3f##WIDTH *origin,                                  \
      const vkl_vvec3f##WIDTH *direction,                               \
      const vkl_vrange1f##WIDTH *tRange,                                \
      VKLValueSelector valueSelector,                                   \
      void *buffer) OPENVKL_CATCH_BEGIN                                 \
  {                                                                     \
    return openvkl::api::currentDevice().initIntervalIterator##WIDTH(   \
        valid,                                                          \
        sampler,                                                        \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),              \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),           \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),            \
        valueSelector,                                                  \
        buffer);                                                        \
  }                                                                     \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitIntervalIteratorN(4);
__define_vklInitIntervalIteratorN(8);
__define_vklInitIntervalIteratorN(16);

#undef __define_vklInitIntervalIteratorN

extern "C" int vklIterateInterval(VKLIntervalIterator iterator,
                                  VKLInterval *interval) OPENVKL_CATCH_BEGIN
{
  int result;
  openvkl::api::currentDevice().iterateInterval1(
      iterator, reinterpret_cast<vVKLIntervalN<1> &>(*interval), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateIntervalN(WIDTH)                  \
  extern "C" void vklIterateInterval##WIDTH(                 \
      const int *valid,                                      \
      VKLIntervalIterator##WIDTH iterator,                   \
      VKLInterval##WIDTH *interval,                          \
      int *result) OPENVKL_CATCH_BEGIN                       \
  {                                                          \
    openvkl::api::currentDevice().iterateInterval##WIDTH(    \
        valid,                                               \
        iterator,                                            \
        reinterpret_cast<vVKLIntervalN<WIDTH> &>(*interval), \
        result);                                             \
  }                                                          \
  OPENVKL_CATCH_END()

__define_vklIterateIntervalN(4);
__define_vklIterateIntervalN(8);
__define_vklIterateIntervalN(16);

#undef __define_vklIterateIntervalN

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" size_t vklGetHitIteratorSize(VKLSampler sampler) OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().getHitIteratorSize1(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize4(VKLSampler sampler) OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().getHitIteratorSize4(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize8(VKLSampler sampler) OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().getHitIteratorSize8(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize16(VKLSampler sampler)
    OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().getHitIteratorSize16(sampler);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLHitIterator vklInitHitIterator(VKLSampler sampler,
                                             const vkl_vec3f *origin,
                                             const vkl_vec3f *direction,
                                             const vkl_range1f *tRange,
                                             float time,
                                             VKLValueSelector valueSelector,
                                             void *buffer) OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().initHitIterator1(
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      time,
      valueSelector,
      buffer);
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitHitIteratorN(WIDTH)                      \
  extern "C" VKLHitIterator##WIDTH vklInitHitIterator##WIDTH(    \
      const int *valid,                                          \
      VKLSampler sampler,                                        \
      const vkl_vvec3f##WIDTH *origin,                           \
      const vkl_vvec3f##WIDTH *direction,                        \
      const vkl_vrange1f##WIDTH *tRange,                         \
      const float *times,                                        \
      VKLValueSelector valueSelector,                            \
      void *buffer) OPENVKL_CATCH_BEGIN                          \
  {                                                              \
    return openvkl::api::currentDevice().initHitIterator##WIDTH( \
        valid,                                                   \
        sampler,                                                 \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),       \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),    \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),     \
        times,                                                   \
        valueSelector,                                           \
        buffer);                                                 \
  }                                                              \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitHitIteratorN(4);
__define_vklInitHitIteratorN(8);
__define_vklInitHitIteratorN(16);

#undef __define_vklInitHitIteratorN

extern "C" int vklIterateHit(VKLHitIterator iterator,
                             VKLHit *hit) OPENVKL_CATCH_BEGIN
{
  int result;
  openvkl::api::currentDevice().iterateHit1(
      iterator, reinterpret_cast<vVKLHitN<1> &>(*hit), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateHitN(WIDTH)                                       \
  extern "C" void vklIterateHit##WIDTH(const int *valid,                     \
                                       VKLHitIterator##WIDTH iterator,       \
                                       VKLHit##WIDTH *hit,                   \
                                       int *result) OPENVKL_CATCH_BEGIN      \
  {                                                                          \
    openvkl::api::currentDevice().iterateHit##WIDTH(                         \
        valid, iterator, reinterpret_cast<vVKLHitN<WIDTH> &>(*hit), result); \
  }                                                                          \
  OPENVKL_CATCH_END()

__define_vklIterateHitN(4);
__define_vklIterateHitN(8);
__define_vklIterateHitN(16);

#undef __define_vklIterateHitN

///////////////////////////////////////////////////////////////////////////////
// Module /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLError vklLoadModule(const char *moduleName) OPENVKL_CATCH_BEGIN
{
  THROW_IF_NULL_STRING(moduleName);
  if (openvkl::api::deviceIsSet()) {
    return (VKLError)openvkl::api::currentDevice().loadModule(moduleName);
  } else {
    return openvkl::loadLocalModule(moduleName);
  }
}
OPENVKL_CATCH_END(VKL_UNKNOWN_ERROR)

///////////////////////////////////////////////////////////////////////////////
// Parameters /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void vklSetBool(VKLObject object,
                           const char *name,
                           int b) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setBool(object, name, static_cast<bool>(b));
}
OPENVKL_CATCH_END()

extern "C" void vklSetFloat(VKLObject object,
                            const char *name,
                            float x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().set1f(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3f(VKLObject object,
                            const char *name,
                            float x,
                            float y,
                            float z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setVec3f(object, name, vec3f(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetInt(VKLObject object,
                          const char *name,
                          int x) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().set1i(object, name, x);
}
OPENVKL_CATCH_END()

extern "C" void vklSetVec3i(
    VKLObject object, const char *name, int x, int y, int z) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setVec3i(object, name, vec3i(x, y, z));
}
OPENVKL_CATCH_END()

extern "C" void vklSetData(VKLObject object,
                           const char *name,
                           VKLData data) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setObject(object, name, (VKLObject)data);
}
OPENVKL_CATCH_END()

extern "C" void vklSetString(VKLObject object,
                             const char *name,
                             const char *s) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setString(object, name, std::string(s));
}
OPENVKL_CATCH_END()

extern "C" void vklSetVoidPtr(VKLObject object,
                              const char *name,
                              void *v) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_OBJECT(object);
  THROW_IF_NULL_STRING(name);
  openvkl::api::currentDevice().setVoidPtr(object, name, v);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Value selector /////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLValueSelector vklNewValueSelector(VKLVolume volume)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  VKLValueSelector valueSelector =
      openvkl::api::currentDevice().newValueSelector(volume);
  if (valueSelector == nullptr) {
    postLogMessage(VKL_LOG_ERROR) << "could not create value selector";
  }

  return valueSelector;
}
OPENVKL_CATCH_END(nullptr)

extern "C" void vklValueSelectorSetRanges(VKLValueSelector valueSelector,
                                          size_t numRanges,
                                          const vkl_range1f *ranges)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  openvkl::api::currentDevice().valueSelectorSetRanges(
      valueSelector,
      utility::ArrayView<const range1f>(
          reinterpret_cast<const range1f *>(ranges), numRanges));
}
OPENVKL_CATCH_END()

extern "C" void vklValueSelectorSetValues(VKLValueSelector valueSelector,
                                          size_t numValues,
                                          const float *values)
    OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  openvkl::api::currentDevice().valueSelectorSetValues(
      valueSelector,
      utility::ArrayView<const float>(reinterpret_cast<const float *>(values),
                                      numValues));
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Sampler ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLSampler vklNewSampler(VKLVolume volume) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  VKLSampler sampler = openvkl::api::currentDevice().newSampler(volume);
  if (sampler == nullptr) {
    postLogMessage(VKL_LOG_ERROR) << "could not create sampler";
  }

  return sampler;
}
OPENVKL_CATCH_END(nullptr)

extern "C" float vklComputeSample(VKLSampler sampler,
                                  const vkl_vec3f *objectCoordinates,
                                  unsigned int attributeIndex,
                                  float time) OPENVKL_CATCH_BEGIN
{
  constexpr int valid = 1;
  float sample;
  openvkl::api::currentDevice().computeSample1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      &sample,
      attributeIndex,
      &time);
  return sample;
}
OPENVKL_CATCH_END(rkcommon::math::nan)

#define __define_vklComputeSampleN(WIDTH)                             \
  extern "C" void vklComputeSample##WIDTH(                            \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN                         \
  {                                                                   \
    openvkl::api::currentDevice().computeSample##WIDTH(               \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        samples,                                                      \
        attributeIndex,                                               \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeSampleN(4);
__define_vklComputeSampleN(8);
__define_vklComputeSampleN(16);

#undef __define_vklComputeSampleN

extern "C" void vklComputeSampleN(VKLSampler sampler,
                                  unsigned int N,
                                  const vkl_vec3f *objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex,
                                  const float *times) OPENVKL_CATCH_BEGIN
{
  openvkl::api::currentDevice().computeSampleN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

extern "C" void vklComputeSampleM(VKLSampler sampler,
                                  const vkl_vec3f *objectCoordinates,
                                  float *samples,
                                  unsigned int M,
                                  const unsigned int *attributeIndices,
                                  float time) OPENVKL_CATCH_BEGIN
{
  constexpr int valid = 1;
  openvkl::api::currentDevice().computeSampleM1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      samples,
      M,
      attributeIndices,
      &time);
}
OPENVKL_CATCH_END()

#define __define_vklComputeSampleMN(WIDTH)                            \
  extern "C" void vklComputeSampleM##WIDTH(                           \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int M,                                                 \
      const unsigned int *attributeIndices,                           \
      const float *times) OPENVKL_CATCH_BEGIN                         \
  {                                                                   \
    openvkl::api::currentDevice().computeSampleM##WIDTH(              \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        samples,                                                      \
        M,                                                            \
        attributeIndices,                                             \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeSampleMN(4);
__define_vklComputeSampleMN(8);
__define_vklComputeSampleMN(16);

#undef __define_vklComputeSampleMN

extern "C" void vklComputeSampleMN(VKLSampler sampler,
                                   unsigned int N,
                                   const vkl_vec3f *objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices,
                                   const float *times) OPENVKL_CATCH_BEGIN
{
  openvkl::api::currentDevice().computeSampleMN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      M,
      attributeIndices,
      times);
}
OPENVKL_CATCH_END()

extern "C" vkl_vec3f vklComputeGradient(VKLSampler sampler,
                                        const vkl_vec3f *objectCoordinates,
                                        unsigned int attributeIndex,
                                        float time) OPENVKL_CATCH_BEGIN
{
  constexpr int valid = 1;
  vkl_vec3f gradient;
  openvkl::api::currentDevice().computeGradient1(
      &valid,
      sampler,
      reinterpret_cast<const vvec3fn<1> &>(*objectCoordinates),
      reinterpret_cast<vvec3fn<1> &>(gradient),
      attributeIndex,
      &time);
  return gradient;
}
OPENVKL_CATCH_END(vkl_vec3f{rkcommon::math::nan})

#define __define_vklComputeGradientN(WIDTH)                           \
  extern "C" void vklComputeGradient##WIDTH(                          \
      const int *valid,                                               \
      VKLSampler sampler,                                             \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      vkl_vvec3f##WIDTH *gradients,                                   \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN                         \
  {                                                                   \
    openvkl::api::currentDevice().computeGradient##WIDTH(             \
        valid,                                                        \
        sampler,                                                      \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*objectCoordinates), \
        reinterpret_cast<vvec3fn<WIDTH> &>(*gradients),               \
        attributeIndex,                                               \
        times);                                                       \
  }                                                                   \
  OPENVKL_CATCH_END()

__define_vklComputeGradientN(4);
__define_vklComputeGradientN(8);
__define_vklComputeGradientN(16);

#undef __define_vklComputeGradientN

extern "C" void vklComputeGradientN(VKLSampler sampler,
                                    unsigned int N,
                                    const vkl_vec3f *objectCoordinates,
                                    vkl_vec3f *gradients,
                                    unsigned int attributeIndex,
                                    const float *times) OPENVKL_CATCH_BEGIN
{
  openvkl::api::currentDevice().computeGradientN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      reinterpret_cast<vvec3fn<1> *>(gradients),
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Volume /////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" VKLVolume vklNewVolume(const char *type) OPENVKL_CATCH_BEGIN
{
  ASSERT_DEVICE();
  THROW_IF_NULL_STRING(type);
  VKLVolume volume = openvkl::api::currentDevice().newVolume(type);
  if (volume == nullptr) {
    postLogMessage(VKL_LOG_ERROR) << "could not create volume '" << type << "'";
  }

  return volume;
}
OPENVKL_CATCH_END(nullptr)

extern "C" vkl_box3f vklGetBoundingBox(VKLVolume volume) OPENVKL_CATCH_BEGIN
{
  const box3f result = openvkl::api::currentDevice().getBoundingBox(volume);
  return reinterpret_cast<const vkl_box3f &>(result);
}
OPENVKL_CATCH_END(vkl_box3f{rkcommon::math::nan})

extern "C" unsigned int vklGetNumAttributes(VKLVolume volume)
    OPENVKL_CATCH_BEGIN
{
  return openvkl::api::currentDevice().getNumAttributes(volume);
}
OPENVKL_CATCH_END(0)

extern "C" vkl_range1f vklGetValueRange(VKLVolume volume) OPENVKL_CATCH_BEGIN
{
  const range1f result = openvkl::api::currentDevice().getValueRange(volume);
  return reinterpret_cast<const vkl_range1f &>(result);
}
OPENVKL_CATCH_END(vkl_range1f{rkcommon::math::nan})
