// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/simd.h"
#include "AddDeviceAPIs.h"
#include "openvkl/common/ManagedObject.h"
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
