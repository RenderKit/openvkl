// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/simd.h"
#include "AddDeviceAPIs.h"
#include "openvkl/common/ManagedObject.h"
#include "openvkl/openvkl.h"

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
