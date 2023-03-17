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
#include "../compute/vklIterateDefault.h"
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

static_assert(VKL_MAX_INTERVAL_ITERATOR_SIZE ==
              std::max(sizeof(GridAcceleratorIterator) +
                           alignof(GridAcceleratorIterator),
                       sizeof(UnstructuredIntervalIterator) +
                           alignof(UnstructuredIntervalIterator)));

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT size_t
vklGetIntervalIteratorSize(const VKLIntervalIteratorContext *context)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  // the sizes below include extra padding, so that we can still use unaligned
  // buffers allocated by the application
  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
    return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  case VOLUME_TYPE_UNSTRUCTURED:
    return sizeof(UnstructuredIntervalIterator) +
           alignof(UnstructuredIntervalIterator);

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

    GridAcceleratorIteratorU_Init(
        reinterpret_cast<GridAcceleratorIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        &time);

    return (VKLIntervalIterator)alignedBuffer;
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    size_t space = sizeof(UnstructuredIntervalIterator) +
                   alignof(UnstructuredIntervalIterator);

    void *alignedBuffer = std::align(alignof(UnstructuredIntervalIterator),
                                     sizeof(UnstructuredIntervalIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    // We use the same iterator implementation for both unstructured and
    // particle volumes. However, only unstructured volumes support elementary
    // cell iteration.
    constexpr bool elementaryCellIterationSupported = true;

    UnstructuredIntervalIterator_Init(
        reinterpret_cast<UnstructuredIntervalIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
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
    UnstructuredIntervalIterator_iterate(
        reinterpret_cast<UnstructuredIntervalIterator *>(iterator),
        reinterpret_cast<Interval *>(interval),
        &result);
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

static_assert(VKL_MAX_HIT_ITERATOR_SIZE ==
              std::max(sizeof(GridAcceleratorIterator) +
                           alignof(GridAcceleratorIterator),
                       sizeof(UnstructuredHitIterator) +
                           alignof(UnstructuredHitIterator)));

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT size_t
vklGetHitIteratorSize(const VKLHitIteratorContext *context)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  // the sizes below include extra padding, so that we can still use unaligned
  // buffers allocated by the application

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
    return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  case VOLUME_TYPE_UNSTRUCTURED:
    return sizeof(UnstructuredHitIterator) + alignof(UnstructuredHitIterator);

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

    GridAcceleratorIteratorU_Init(
        reinterpret_cast<GridAcceleratorIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        &time);

    return (VKLHitIterator)alignedBuffer;
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    size_t space =
        sizeof(UnstructuredHitIterator) + alignof(UnstructuredHitIterator);

    void *alignedBuffer = std::align(alignof(UnstructuredHitIterator),
                                     sizeof(UnstructuredHitIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    UnstructuredHitIterator_Init(
        reinterpret_cast<UnstructuredHitIterator *>(alignedBuffer),
        reinterpret_cast<const HitIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange));

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
  const HitIteratorShared *iter =
      reinterpret_cast<const HitIteratorShared *>(iterator);

  const DeviceVolumeType volumeType =
      iter->context->super.super.sampler->volume->type;

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY: {
    int result = 0;
    GridAcceleratorIterator_iterateHit_uniform(
        reinterpret_cast<GridAcceleratorIterator *>(iterator), hit, &result);
    return result;
  }

  case VOLUME_TYPE_UNSTRUCTURED: {
    int result = 0;
    UnstructuredHitIterator_iterateHit(
        reinterpret_cast<UnstructuredHitIterator *>(iterator), hit, &result);
    return result;
  }

  default:
    assert(false);
    return false;
  }
}
