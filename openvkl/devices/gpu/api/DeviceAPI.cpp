// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/simd.h"
#include "AddDeviceAPIs.h"
#include "openvkl/common/ManagedObject.h"
#include "openvkl/openvkl.h"

#include "../compute/vklComputeSample.h"
#include "../compute/vklIterateInterval.h"
#include "../include/openvkl/device/openvkl.h"

using namespace openvkl;
using namespace openvkl::gpu_device;

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

extern "C" void openvkl_init_module_gpu_device();

extern "C" OPENVKL_DLLEXPORT void vklInit()
{
  openvkl_init_module_gpu_device();
}

///////////////////////////////////////////////////////////////////////////////
// Sampler ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT float vklComputeSample(
    const VKLSampler *sampler,
    const vkl_vec3f *objectCoordinates,
    unsigned int attributeIndex,
    float time)
{
  assert(sampler);
  const ispc::SamplerShared *samplerShared =
      static_cast<const ispc::SamplerShared *>(sampler->device);
  const ispc::SharedStructuredVolume *sharedStructuredVolume =
      reinterpret_cast<const ispc::SharedStructuredVolume *>(
          samplerShared->volume);

  return SharedStructuredVolume_computeSample_uniform(
      sharedStructuredVolume,
      *reinterpret_cast<const vec3f *>(objectCoordinates),
      samplerShared->filter,
      attributeIndex,
      time);
}

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

///////////////////////////////////////////////////////////////////////////////
// Iterator////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT VKLIntervalIterator
vklInitIntervalIterator(const VKLIntervalIteratorContext *context,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        float time,
                        void *buffer)
{
  // the provided buffer is guaranteed to be of size `space` below, but it may
  // be unaligned. so, we'll move to an appropriately aligned address inside the
  // provided buffer.
  size_t space =
      sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  void *alignedBuffer = std::align(alignof(GridAcceleratorIterator),
                                   sizeof(GridAcceleratorIterator),
                                   buffer,
                                   space);
  assert(alignedBuffer);

  GridAcceleratorIteratorU_Init(alignedBuffer,
                                context->device,
                                (void *)origin,
                                (void *)direction,
                                (void *)tRange,
                                &time);

  return (VKLIntervalIterator)alignedBuffer;
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT int vklIterateInterval(
    VKLIntervalIterator iterator, VKLInterval *interval)
{
  int result = 0;
  GridAcceleratorIterator_iterateInterval_uniform(iterator, interval, &result);
  return result;
}
