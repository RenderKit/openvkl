// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
using namespace ispc;

#include "AddDeviceAPIs.h"
#include "openvkl/common/ManagedObject.h"
#include "openvkl/openvkl.h"

#include "../include/openvkl/device/openvkl.h"

#include "../compute/vklCompute.h"
#include "../compute/vklComputeUnstructured.h"
#include "../compute/vklIterateUnstructured.h"
#include "../compute/vklIterators.h"

using namespace openvkl;
using namespace openvkl::gpu_device;

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
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
  case VOLUME_TYPE_STRUCTURED_SPHERICAL: {
    const SharedStructuredVolume *v =
        reinterpret_cast<const SharedStructuredVolume *>(samplerShared->volume);

    return SharedStructuredVolume_computeSample_uniform(
        v,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        samplerShared->filter,
        attributeIndex,
        time);
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    return UnstructuredVolume_sample(
        samplerShared, *reinterpret_cast<const vec3f *>(objectCoordinates));
  }

  default:
    assert(false);
    return -1.f;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT void vklComputeSampleM(
    const VKLSampler *sampler,
    const vkl_vec3f *objectCoordinates,
    float *samples,
    unsigned int M,
    const unsigned int *attributeIndices,
    float time)
{
  assert(sampler);
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
  case VOLUME_TYPE_STRUCTURED_SPHERICAL:
    SharedStructuredVolume_sampleM_uniform(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        M,
        attributeIndices,
        time,
        samples);
    break;

  case VOLUME_TYPE_UNSTRUCTURED: {
    UnstructuredVolume_sampleM(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        M,
        attributeIndices,
        samples);
    break;
  }

  default:
    assert(false);
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT vkl_vec3f
vklComputeGradient(const VKLSampler *sampler,
                   const vkl_vec3f *objectCoordinates,
                   unsigned int attributeIndex,
                   float time)
{
  assert(sampler);
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
  case VOLUME_TYPE_STRUCTURED_SPHERICAL: {
    const SharedStructuredVolume *v =
        reinterpret_cast<const SharedStructuredVolume *>(samplerShared->volume);

    return SharedStructuredVolume_computeGradient_bbox_checks(
        v,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        samplerShared->filter,
        attributeIndex,
        time);
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    return UnstructuredVolume_computeGradient(
        samplerShared, *reinterpret_cast<const vec3f *>(objectCoordinates));
  }

  default:
    assert(false);
    return vkl_vec3f{-1.f, -1.f, -1.f};
  }
}

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// for now, with just one volume (iterator) type we guarantee equality. later
// this will be a >= check.
static_assert(VKL_MAX_INTERVAL_ITERATOR_SIZE ==
              sizeof(GridAcceleratorIterator) +
                  alignof(GridAcceleratorIterator));

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT size_t
vklGetIntervalIteratorSize(const VKLIntervalIteratorContext *context)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
    // the size includes extra padding, so that we can still use an unaligned
    // buffer allocated by the application
    return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  case VOLUME_TYPE_UNSTRUCTURED:
    return sizeof(UnstructuredIterator) + alignof(UnstructuredIterator);

  default:
    assert(false);
    return 0;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT VKLIntervalIterator
vklInitIntervalIterator(const VKLIntervalIteratorContext *context,
                        const vkl_vec3f *origin,
                        const vkl_vec3f *direction,
                        const vkl_range1f *tRange,
                        float time,
                        void *buffer)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY: {
    // the provided buffer is guaranteed to be of size `space` below, but it may
    // be unaligned. so, we'll move to an appropriately aligned address inside
    // the provided buffer.
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

  case VOLUME_TYPE_UNSTRUCTURED: {
    size_t space = sizeof(UnstructuredIterator) + alignof(UnstructuredIterator);

    void *alignedBuffer = std::align(alignof(UnstructuredIterator),
                                     sizeof(UnstructuredIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    // We use the same iterator implementation for both unstructured and
    // particle volumes. However, only unstructured volumes support elementary
    // cell iteration.
    constexpr bool elementaryCellIterationSupported = true;

    UnstructuredIterator_Initialize(alignedBuffer,
                                    context->device,
                                    (void *)origin,
                                    (void *)direction,
                                    (void *)tRange,
                                    elementaryCellIterationSupported);

    return (VKLIntervalIterator)alignedBuffer;
  }

  default:
    assert(false);
    return nullptr;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT int vklIterateInterval(
    VKLIntervalIterator iterator, VKLInterval *interval)
{
  assert(iterator);
  const IntervalIteratorShared *iter =
      reinterpret_cast<const IntervalIteratorShared *>(iterator);

  const DeviceVolumeType volumeType =
      iter->context->super.sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY: {
    int result = 0;
    GridAcceleratorIterator_iterateInterval_uniform(
        iterator, interval, &result);
    return result;
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    int result = 0;
    UnstructuredIterator_iterateInterval(iterator, interval, &result);
    return result;
  }

  default:
    assert(false);
    return false;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// for now, with just one volume (iterator) type we guarantee equality. later
// this will be a >= check.
static_assert(VKL_MAX_HIT_ITERATOR_SIZE ==
              sizeof(GridAcceleratorIterator) +
                  alignof(GridAcceleratorIterator));

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT size_t
vklGetHitIteratorSize(const VKLHitIteratorContext *context)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
    // the size includes extra padding, so that we can still use an unaligned
    // buffer allocated by the application
    return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  default:
    assert(false);
    return 0;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT VKLHitIterator
vklInitHitIterator(const VKLHitIteratorContext *context,
                   const vkl_vec3f *origin,
                   const vkl_vec3f *direction,
                   const vkl_range1f *tRange,
                   float time,
                   void *buffer)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY: {
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

    return (VKLHitIterator)alignedBuffer;
  }

  default:
    assert(false);
    return nullptr;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT int vklIterateHit(
    VKLHitIterator iterator, VKLHit *hit)
{
  assert(iterator);
  const IntervalIteratorShared *iter =
      reinterpret_cast<const IntervalIteratorShared *>(iterator);

  const DeviceVolumeType volumeType =
      iter->context->super.sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY: {
    int result = 0;
    GridAcceleratorIterator_iterateHit_uniform(iterator, hit, &result);
    return result;
  }

  default:
    assert(false);
    return false;
  }
}
