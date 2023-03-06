// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../../../common/IteratorBase.h"
#include "../common/ManagedObject.h"
#include "../common/simd.h"
#include "AddDeviceAPIs.h"
#include "openvkl/openvkl.h"

using namespace openvkl;
using namespace openvkl::cpu_device;

///////////////////////////////////////////////////////////////////////////////
// Macro helpers //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// Note that these are different than those in the top-level API.cpp

#define OPENVKL_CATCH_BEGIN_UNSAFE(deviceSource)                      \
  {                                                                   \
    assert(deviceSource != nullptr);                                  \
    openvkl::ManagedObject *managedObject =                           \
        static_cast<openvkl::ManagedObject *>(deviceSource->host);    \
    AddDeviceAPIs *deviceObj =                                        \
        reinterpret_cast<AddDeviceAPIs *>(managedObject->device.ptr); \
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
// Device initialization //////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" void openvkl_init_module_cpu_device();

extern "C" OPENVKL_DLLEXPORT void vklInit()
{
  openvkl_init_module_cpu_device();
}

///////////////////////////////////////////////////////////////////////////////
// Sampler ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" OPENVKL_DLLEXPORT float vklComputeSample(
    const VKLSampler *sampler,
    const vkl_vec3f *objectCoordinates,
    unsigned int attributeIndex,
    float time) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  float sample;
  deviceObj->computeSample1(
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
  extern "C" OPENVKL_DLLEXPORT void vklComputeSample##WIDTH(          \
      const int *valid,                                               \
      const VKLSampler *sampler,                                      \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeSample##WIDTH(                                  \
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

extern "C" OPENVKL_DLLEXPORT void vklComputeSampleN(
    const VKLSampler *sampler,
    unsigned int N,
    const vkl_vec3f *objectCoordinates,
    float *samples,
    unsigned int attributeIndex,
    const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeSampleN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

extern "C" OPENVKL_DLLEXPORT void vklComputeSampleM(
    const VKLSampler *sampler,
    const vkl_vec3f *objectCoordinates,
    float *samples,
    unsigned int M,
    const unsigned int *attributeIndices,
    float time) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  deviceObj->computeSampleM1(
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
  extern "C" OPENVKL_DLLEXPORT void vklComputeSampleM##WIDTH(         \
      const int *valid,                                               \
      const VKLSampler *sampler,                                      \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      float *samples,                                                 \
      unsigned int M,                                                 \
      const unsigned int *attributeIndices,                           \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeSampleM##WIDTH(                                 \
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

extern "C" OPENVKL_DLLEXPORT void vklComputeSampleMN(
    const VKLSampler *sampler,
    unsigned int N,
    const vkl_vec3f *objectCoordinates,
    float *samples,
    unsigned int M,
    const unsigned int *attributeIndices,
    const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeSampleMN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      samples,
      M,
      attributeIndices,
      times);
}
OPENVKL_CATCH_END()

extern "C" OPENVKL_DLLEXPORT vkl_vec3f
vklComputeGradient(const VKLSampler *sampler,
                   const vkl_vec3f *objectCoordinates,
                   unsigned int attributeIndex,
                   float time) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  constexpr int valid = 1;
  vkl_vec3f gradient;
  deviceObj->computeGradient1(
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
  extern "C" OPENVKL_DLLEXPORT void vklComputeGradient##WIDTH(        \
      const int *valid,                                               \
      const VKLSampler *sampler,                                      \
      const vkl_vvec3f##WIDTH *objectCoordinates,                     \
      vkl_vvec3f##WIDTH *gradients,                                   \
      unsigned int attributeIndex,                                    \
      const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)         \
  {                                                                   \
    deviceObj->computeGradient##WIDTH(                                \
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

extern "C" OPENVKL_DLLEXPORT void vklComputeGradientN(
    const VKLSampler *sampler,
    unsigned int N,
    const vkl_vec3f *objectCoordinates,
    vkl_vec3f *gradients,
    unsigned int attributeIndex,
    const float *times) OPENVKL_CATCH_BEGIN_UNSAFE(sampler)
{
  deviceObj->computeGradientN(
      sampler,
      N,
      reinterpret_cast<const vvec3fn<1> *>(objectCoordinates),
      reinterpret_cast<vvec3fn<1> *>(gradients),
      attributeIndex,
      times);
}
OPENVKL_CATCH_END()

///////////////////////////////////////////////////////////////////////////////
// Iterator helpers ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// helpers for iterators. note that iterators are not managed objects, and that
// we can't use std::is_convertible<> without augmenting the public (opaque)
// iterator types
template <typename IteratorT>
typename std::enable_if<
    std::is_same<IteratorT, IntervalIterator>::value ||
        std::is_same<IteratorT, IntervalIterator4>::value ||
        std::is_same<IteratorT, IntervalIterator8>::value ||
        std::is_same<IteratorT, IntervalIterator16>::value ||
        std::is_same<IteratorT, HitIterator>::value ||
        std::is_same<IteratorT, HitIterator4>::value ||
        std::is_same<IteratorT, HitIterator8>::value ||
        std::is_same<IteratorT, HitIterator16>::value,
    AddDeviceAPIs *>::type inline deviceFrom(IteratorT *it)
{
  return static_cast<AddDeviceAPIs *>(
      reinterpret_cast<openvkl::IteratorBase *>(it)->device);
}

template <typename IteratorT>
typename std::enable_if<
    std::is_same<IteratorT, IntervalIterator>::value ||
        std::is_same<IteratorT, IntervalIterator4>::value ||
        std::is_same<IteratorT, IntervalIterator8>::value ||
        std::is_same<IteratorT, IntervalIterator16>::value ||
        std::is_same<IteratorT, HitIterator>::value ||
        std::is_same<IteratorT, HitIterator4>::value ||
        std::is_same<IteratorT, HitIterator8>::value ||
        std::is_same<IteratorT, HitIterator16>::value,
    void>::type inline deviceAttach(Device *device, IteratorT *it)
{
  auto itObj    = reinterpret_cast<openvkl::IteratorBase *>(it);
  itObj->device = device;
}

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define OPENVKL_CATCH_BEGIN_ITER_UNSAFE(deviceSource)    \
  {                                                      \
    assert(deviceSource != nullptr);                     \
    AddDeviceAPIs *deviceObj = deviceFrom(deviceSource); \
    try {
extern "C" size_t vklGetIntervalIteratorSize(
    const VKLIntervalIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getIntervalIteratorSize1(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize4(
    const VKLIntervalIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getIntervalIteratorSize4(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize8(
    const VKLIntervalIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getIntervalIteratorSize8(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetIntervalIteratorSize16(
    const VKLIntervalIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getIntervalIteratorSize16(context);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLIntervalIterator vklInitIntervalIterator(
    const VKLIntervalIteratorContext *context,
    const vkl_vec3f *origin,
    const vkl_vec3f *direction,
    const vkl_range1f *tRange,
    float time,
    void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  auto it = deviceObj->initIntervalIterator1(
      context,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      time,
      buffer);
  deviceAttach(deviceObj, it);
  return it;
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitIntervalIteratorN(WIDTH)                        \
  extern "C" VKLIntervalIterator##WIDTH vklInitIntervalIterator##WIDTH( \
      const int *valid,                                                 \
      const VKLIntervalIteratorContext *context,                        \
      const vkl_vvec3f##WIDTH *origin,                                  \
      const vkl_vvec3f##WIDTH *direction,                               \
      const vkl_vrange1f##WIDTH *tRange,                                \
      const float *times,                                               \
      void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(context)                 \
  {                                                                     \
    auto it = deviceObj->initIntervalIterator##WIDTH(                   \
        valid,                                                          \
        context,                                                        \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),              \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction),           \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),            \
        times,                                                          \
        buffer);                                                        \
    deviceAttach(deviceObj, it);                                        \
    return it;                                                          \
  }                                                                     \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitIntervalIteratorN(4);
__define_vklInitIntervalIteratorN(8);
__define_vklInitIntervalIteratorN(16);

#undef __define_vklInitIntervalIteratorN

extern "C" int vklIterateInterval(VKLIntervalIterator iterator,
                                  VKLInterval *interval)
    OPENVKL_CATCH_BEGIN_ITER_UNSAFE(iterator)
{
  int result;
  deviceObj->iterateInterval1(
      iterator, reinterpret_cast<vVKLIntervalN<1> &>(*interval), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateIntervalN(WIDTH)                  \
  extern "C" void vklIterateInterval##WIDTH(                 \
      const int *valid,                                      \
      VKLIntervalIterator##WIDTH iterator,                   \
      VKLInterval##WIDTH *interval,                          \
      int *result) OPENVKL_CATCH_BEGIN_ITER_UNSAFE(iterator) \
  {                                                          \
    deviceObj->iterateInterval##WIDTH(                       \
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

extern "C" size_t vklGetHitIteratorSize(const VKLHitIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getHitIteratorSize1(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize4(const VKLHitIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getHitIteratorSize4(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize8(const VKLHitIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getHitIteratorSize8(context);
}
OPENVKL_CATCH_END(0u)

extern "C" size_t vklGetHitIteratorSize16(const VKLHitIteratorContext *context)
    OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  return deviceObj->getHitIteratorSize16(context);
}
OPENVKL_CATCH_END(0u)

extern "C" VKLHitIterator vklInitHitIterator(
    const VKLHitIteratorContext *context,
    const vkl_vec3f *origin,
    const vkl_vec3f *direction,
    const vkl_range1f *tRange,
    float time,
    void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(context)
{
  auto it = deviceObj->initHitIterator1(
      context,
      reinterpret_cast<const vvec3fn<1> &>(*origin),
      reinterpret_cast<const vvec3fn<1> &>(*direction),
      reinterpret_cast<const vrange1fn<1> &>(*tRange),
      time,
      buffer);
  deviceAttach(deviceObj, it);
  return it;
}
OPENVKL_CATCH_END(nullptr)

#define __define_vklInitHitIteratorN(WIDTH)                   \
  extern "C" VKLHitIterator##WIDTH vklInitHitIterator##WIDTH( \
      const int *valid,                                       \
      const VKLHitIteratorContext *context,                   \
      const vkl_vvec3f##WIDTH *origin,                        \
      const vkl_vvec3f##WIDTH *direction,                     \
      const vkl_vrange1f##WIDTH *tRange,                      \
      const float *times,                                     \
      void *buffer) OPENVKL_CATCH_BEGIN_UNSAFE(context)       \
  {                                                           \
    auto it = deviceObj->initHitIterator##WIDTH(              \
        valid,                                                \
        context,                                              \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*origin),    \
        reinterpret_cast<const vvec3fn<WIDTH> &>(*direction), \
        reinterpret_cast<const vrange1fn<WIDTH> &>(*tRange),  \
        times,                                                \
        buffer);                                              \
    deviceAttach(deviceObj, it);                              \
    return it;                                                \
  }                                                           \
  OPENVKL_CATCH_END(nullptr)

__define_vklInitHitIteratorN(4);
__define_vklInitHitIteratorN(8);
__define_vklInitHitIteratorN(16);

#undef __define_vklInitHitIteratorN

extern "C" int vklIterateHit(VKLHitIterator iterator, VKLHit *hit)
    OPENVKL_CATCH_BEGIN_ITER_UNSAFE(iterator)
{
  int result;
  deviceObj->iterateHit1(
      iterator, reinterpret_cast<vVKLHitN<1> &>(*hit), &result);
  return result;
}
OPENVKL_CATCH_END(false)

#define __define_vklIterateHitN(WIDTH)                                       \
  extern "C" void vklIterateHit##WIDTH(const int *valid,                     \
                                       VKLHitIterator##WIDTH iterator,       \
                                       VKLHit##WIDTH *hit,                   \
                                       int *result)                          \
      OPENVKL_CATCH_BEGIN_ITER_UNSAFE(iterator)                              \
  {                                                                          \
    deviceObj->iterateHit##WIDTH(                                            \
        valid, iterator, reinterpret_cast<vVKLHitN<WIDTH> &>(*hit), result); \
  }                                                                          \
  OPENVKL_CATCH_END()

__define_vklIterateHitN(4);
__define_vklIterateHitN(8);
__define_vklIterateHitN(16);

#undef __define_vklIterateHitN