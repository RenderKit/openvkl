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
#include "../compute/vklComputeAMR.h"
#include "../compute/vklComputeUnstructured.h"
#include "../compute/vklComputeVdb.h"
#include "../compute/vklIterateDefault.h"
#include "../compute/vklIterateUnstructured.h"
#include "../compute/vklIterateVdb.h"
#include "../compute/vklIterators.h"

#include "../compute/vklComputeParticle.h"

using namespace openvkl;
using namespace openvkl::gpu_device;

///////////////////////////////////////////////////////////////////////////////
// Helpers ////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline void assertValidTime(const float time)
{
#ifndef NDEBUG
  assert(time >= 0.f && time <= 1.0f);
#endif
}

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
    float time,
    const VKLFeatureFlags featureFlags)
{
  assert(sampler);
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  if ((volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
       featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) ||
      (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
       featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME)) {
    const SharedStructuredVolume *v =
        reinterpret_cast<const SharedStructuredVolume *>(samplerShared->volume);

    return SharedStructuredVolume_computeSample_uniform(
        v,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        samplerShared->filter,
        attributeIndex,
        time);
  }

  else if (volumeType == VOLUME_TYPE_UNSTRUCTURED &&
           featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) {
    return UnstructuredVolume_sample(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        0.f,
        0,
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_PARTICLE &&
           featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) {
    return VKLParticleVolume_sample(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        0.f,
        0,
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_AMR &&
           featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME) {
    return AMRVolume_sample(samplerShared,
                            *reinterpret_cast<const vec3f *>(objectCoordinates),
                            0.f,
                            0,
                            featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    assert(attributeIndex <
           reinterpret_cast<const VdbSamplerShared *>(sampler->device)
               ->grid->numAttributes);
#ifndef NDEBUG
    assertValidTime(time);
#endif
    return VdbSampler_computeSample_uniform(
        reinterpret_cast<const SamplerShared *>(sampler->device),
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        time,
        attributeIndex,
        featureFlags);
  }

  else {
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
    float time,
    const VKLFeatureFlags featureFlags)
{
  assert(sampler);
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  if ((volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
       featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) ||
      (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
       featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME)) {
    SharedStructuredVolume_sampleM_uniform(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        M,
        attributeIndices,
        time,
        samples);
  }

  else if (volumeType == VOLUME_TYPE_UNSTRUCTURED &&
           featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) {
    UnstructuredVolume_sampleM(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        M,
        attributeIndices,
        samples,
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_PARTICLE &&
           featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) {
    UnstructuredVolume_sampleM(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        M,
        attributeIndices,
        samples,
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_AMR &&
           featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME) {
    AMRVolume_sampleM(samplerShared,
                      *reinterpret_cast<const vec3f *>(objectCoordinates),
                      M,
                      attributeIndices,
                      samples,
                      featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
#ifndef NDEBUG
    for (unsigned int i = 0; i < M; i++)
      assert(attributeIndices[i] <
             reinterpret_cast<const VdbSamplerShared *>(sampler->device)
                 ->grid->numAttributes);
    assertValidTime(time);
#endif
    return VdbSampler_computeSampleM_uniform(
        reinterpret_cast<const VdbSamplerShared *>(sampler->device),
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        time,
        M,
        attributeIndices,
        samples,
        featureFlags);
  }

  else {
    assert(false);
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT vkl_vec3f
vklComputeGradient(const VKLSampler *sampler,
                   const vkl_vec3f *objectCoordinates,
                   unsigned int attributeIndex,
                   float time,
                   const VKLFeatureFlags featureFlags)
{
  assert(sampler);
  const SamplerShared *samplerShared =
      static_cast<const SamplerShared *>(sampler->device);

  const DeviceVolumeType volumeType = samplerShared->volume->type;

  if (volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
      featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) {
    const SharedStructuredVolume *v =
        reinterpret_cast<const SharedStructuredVolume *>(samplerShared->volume);

    return SharedStructuredVolume_computeGradient_bbox_checks(
        v,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        samplerShared->filter,
        attributeIndex,
        time);
  }

  else if (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
           featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME) {
    const SharedStructuredVolume *v =
        reinterpret_cast<const SharedStructuredVolume *>(samplerShared->volume);

    return SharedStructuredVolume_computeGradient_NaN_checks(
        v,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        samplerShared->filter,
        attributeIndex,
        time);
  }

  else if (volumeType == VOLUME_TYPE_UNSTRUCTURED &&
           featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) {
    return UnstructuredVolume_computeGradient(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_PARTICLE &&
           featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) {
    return VKLParticleVolume_computeGradient(
        samplerShared, *reinterpret_cast<const vec3f *>(objectCoordinates));
  }

  else if (volumeType == VOLUME_TYPE_AMR &&
           featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME) {
    return AMRVolume_computeGradient(
        samplerShared,
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        featureFlags);
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    assert(attributeIndex <
           reinterpret_cast<const VdbSamplerShared *>(sampler->device)
               ->grid->numAttributes);
#ifndef NDEBUG
    assertValidTime(time);
#endif
    return VdbSampler_computeGradient_uniform(
        reinterpret_cast<const VdbSamplerShared *>(sampler->device),
        *reinterpret_cast<const vec3f *>(objectCoordinates),
        time,
        attributeIndex,
        featureFlags);
  }

  else {
    assert(false);
    return vkl_vec3f{-1.f, -1.f, -1.f};
  }
}

///////////////////////////////////////////////////////////////////////////////
// Interval iterator //////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static_assert(
    VKL_MAX_INTERVAL_ITERATOR_SIZE ==
    std::max(sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator),
             std::max(sizeof(DefaultIntervalIterator) +
                          alignof(DefaultIntervalIterator),
                      std::max(sizeof(UnstructuredIntervalIterator) +
                                   alignof(UnstructuredIntervalIterator),
                               sizeof(VdbIntervalIterator) +
                                   alignof(VdbIntervalIterator)))));

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

  case VOLUME_TYPE_STRUCTURED_SPHERICAL:
    return sizeof(DefaultIntervalIterator) + alignof(DefaultIntervalIterator);

  case VOLUME_TYPE_UNSTRUCTURED:
  case VOLUME_TYPE_PARTICLE:
  case VOLUME_TYPE_AMR:
    return sizeof(UnstructuredIntervalIterator) +
           alignof(UnstructuredIntervalIterator);

  case VOLUME_TYPE_VDB:
    return sizeof(VdbIntervalIterator) + alignof(VdbIntervalIterator);

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
                        void *buffer,
                        const VKLFeatureFlags featureFlags)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  if (volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
      featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) {
    // the provided buffer is guaranteed to be of size `space` below, but it
    // may be unaligned. so, we'll move to an appropriately aligned address
    // inside the provided buffer.
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

  else if (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
           featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME) {
    size_t space =
        sizeof(DefaultIntervalIterator) + alignof(DefaultIntervalIterator);

    void *alignedBuffer = std::align(alignof(DefaultIntervalIterator),
                                     sizeof(DefaultIntervalIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    DefaultIntervalIterator_Init(
        reinterpret_cast<DefaultIntervalIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        false);

    return (VKLIntervalIterator)alignedBuffer;
  }

  else if ((volumeType == VOLUME_TYPE_UNSTRUCTURED &&
            featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) ||
           (volumeType == VOLUME_TYPE_PARTICLE &&
            featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) ||
           (volumeType == VOLUME_TYPE_AMR &&
            featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME)) {
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
    const bool elementaryCellIterationSupported =
        (volumeType == VOLUME_TYPE_UNSTRUCTURED);

    UnstructuredIntervalIterator_Init(
        reinterpret_cast<UnstructuredIntervalIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        elementaryCellIterationSupported);

    return (VKLIntervalIterator)alignedBuffer;
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    size_t space = sizeof(VdbIntervalIterator) + alignof(VdbIntervalIterator);

    void *alignedBuffer = std::align(alignof(VdbIntervalIterator),
                                     sizeof(VdbIntervalIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    VdbIntervalIterator_Init(
        reinterpret_cast<VdbIntervalIterator *>(alignedBuffer),
        reinterpret_cast<const IntervalIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange));

    return (VKLIntervalIterator)alignedBuffer;
  }

  else {
    assert(false);
    return nullptr;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT int vklIterateInterval(
    VKLIntervalIterator iterator,
    VKLInterval *interval,
    const VKLFeatureFlags featureFlags)
{
  assert(iterator);
  const IntervalIteratorShared *iter =
      reinterpret_cast<const IntervalIteratorShared *>(iterator);

  const DeviceVolumeType volumeType =
      iter->context->super.sampler->volume->type;

  if (volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
      featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) {
    int result = 0;
    GridAcceleratorIterator_iterateInterval_uniform(
        iterator, interval, &result);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
           featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME) {
    int result = 0;
    DefaultIntervalIterator_iterateIntervalInternal(
        reinterpret_cast<DefaultIntervalIterator *>(iterator),
        reinterpret_cast<Interval *>(interval),
        iter->context->super.valueRanges,
        false,
        &result);
    return result;
  }

  else if ((volumeType == VOLUME_TYPE_UNSTRUCTURED &&
            featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) ||
           (volumeType == VOLUME_TYPE_PARTICLE &&
            featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) ||
           (volumeType == VOLUME_TYPE_AMR &&
            featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME)) {
    int result = 0;
    UnstructuredIntervalIterator_iterate(
        reinterpret_cast<UnstructuredIntervalIterator *>(iterator),
        reinterpret_cast<Interval *>(interval),
        &result);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    int result = 0;
    VdbIntervalIterator_iterate(
        reinterpret_cast<VdbIntervalIterator *>(iterator),
        reinterpret_cast<Interval *>(interval),
        &result);
    return result;
  }

  else {
    assert(false);
    return false;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Hit iterator ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

static_assert(
    VKL_MAX_HIT_ITERATOR_SIZE ==
    std::max(
        sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator),
        std::max(sizeof(SphericalHitIterator) + alignof(SphericalHitIterator),
                 std::max(sizeof(UnstructuredHitIterator) +
                              alignof(UnstructuredHitIterator),
                          std::max(sizeof(ParticleHitIterator) +
                                       alignof(ParticleHitIterator),
                                   std::max(sizeof(AMRHitIterator) +
                                                alignof(AMRHitIterator),
                                            sizeof(VdbHitIterator) +
                                                alignof(VdbHitIterator)))))));

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT size_t
vklGetHitIteratorSize(const VKLHitIteratorContext *context)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  // the sizes below include extra padding, so that we can still
  // use unaligned buffers allocated by the application

  switch (volumeType) {
  case VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY:
    return sizeof(GridAcceleratorIterator) + alignof(GridAcceleratorIterator);

  case VOLUME_TYPE_STRUCTURED_SPHERICAL:
    return sizeof(SphericalHitIterator) + alignof(SphericalHitIterator);

  case VOLUME_TYPE_UNSTRUCTURED:
    return sizeof(UnstructuredHitIterator) + alignof(UnstructuredHitIterator);

  case VOLUME_TYPE_PARTICLE:
    return sizeof(ParticleHitIterator) + alignof(ParticleHitIterator);

  case VOLUME_TYPE_AMR:
    return sizeof(AMRHitIterator) + alignof(AMRHitIterator);

  case VOLUME_TYPE_VDB:
    return sizeof(VdbHitIterator) + alignof(VdbHitIterator);

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
                   void *buffer,
                   const VKLFeatureFlags featureFlags)
{
  assert(context);
  const IteratorContext *ic =
      static_cast<const IteratorContext *>(context->device);

  const DeviceVolumeType volumeType = ic->sampler->volume->type;

  if (volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
      featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) {
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

  else if (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
           featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME) {
    size_t space = sizeof(SphericalHitIterator) + alignof(SphericalHitIterator);

    void *alignedBuffer = std::align(alignof(SphericalHitIterator),
                                     sizeof(SphericalHitIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    SphericalHitIterator_Init(
        reinterpret_cast<SphericalHitIterator *>(alignedBuffer),
        reinterpret_cast<const HitIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        time);

    return (VKLHitIterator)alignedBuffer;
  }

  else if (volumeType == VOLUME_TYPE_UNSTRUCTURED &&
           featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) {
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
        reinterpret_cast<const box1f *>(tRange),
        time);

    return (VKLHitIterator)alignedBuffer;
  }

  else if (volumeType == VOLUME_TYPE_PARTICLE &&
           featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) {
    size_t space = sizeof(ParticleHitIterator) + alignof(ParticleHitIterator);

    void *alignedBuffer = std::align(alignof(ParticleHitIterator),
                                     sizeof(ParticleHitIterator),
                                     buffer,
                                     space);
    assert(alignedBuffer);

    ParticleHitIterator_Init(
        reinterpret_cast<ParticleHitIterator *>(alignedBuffer),
        reinterpret_cast<const HitIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        time);

    return (VKLHitIterator)alignedBuffer;
  }

  else if (volumeType == VOLUME_TYPE_AMR &&
           featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME) {
    size_t space = sizeof(AMRHitIterator) + alignof(AMRHitIterator);

    void *alignedBuffer = std::align(
        alignof(AMRHitIterator), sizeof(AMRHitIterator), buffer, space);
    assert(alignedBuffer);

    AMRHitIterator_Init(
        reinterpret_cast<AMRHitIterator *>(alignedBuffer),
        reinterpret_cast<const HitIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        time);

    return (VKLHitIterator)alignedBuffer;
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    size_t space = sizeof(VdbHitIterator) + alignof(VdbHitIterator);

    void *alignedBuffer = std::align(
        alignof(VdbHitIterator), sizeof(VdbHitIterator), buffer, space);
    assert(alignedBuffer);

    VdbHitIterator_Init(
        reinterpret_cast<VdbHitIterator *>(alignedBuffer),
        reinterpret_cast<const HitIteratorContext *>(context->device),
        reinterpret_cast<const vec3f *>(origin),
        reinterpret_cast<const vec3f *>(direction),
        reinterpret_cast<const box1f *>(tRange),
        time);

    return (VKLHitIterator)alignedBuffer;
  }

  else {
    assert(false);
    return nullptr;
  }
}

extern "C" SYCL_EXTERNAL OPENVKL_DLLEXPORT int vklIterateHit(
    VKLHitIterator iterator, VKLHit *hit, const VKLFeatureFlags featureFlags)
{
  assert(iterator);
  const HitIteratorShared *iter =
      reinterpret_cast<const HitIteratorShared *>(iterator);

  const DeviceVolumeType volumeType =
      iter->context->super.super.sampler->volume->type;

  if (volumeType == VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY &&
      featureFlags & VKL_FEATURE_FLAG_STRUCTURED_REGULAR_VOLUME) {
    int result = 0;
    GridAcceleratorIterator_iterateHit_uniform(
        reinterpret_cast<GridAcceleratorIterator *>(iterator), hit, &result);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_STRUCTURED_SPHERICAL &&
           featureFlags & VKL_FEATURE_FLAG_STRUCTURED_SPHERICAL_VOLUME) {
    int result = 0;
    SphericalHitIterator_iterateHit(
        reinterpret_cast<SphericalHitIterator *>(iterator),
        hit,
        &result,
        featureFlags);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_UNSTRUCTURED &&
           featureFlags & VKL_FEATURE_FLAG_UNSTRUCTURED_VOLUME) {
    int result = 0;
    UnstructuredHitIterator_iterateHit(
        reinterpret_cast<UnstructuredHitIterator *>(iterator),
        hit,
        &result,
        featureFlags);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_PARTICLE &&
           featureFlags & VKL_FEATURE_FLAG_PARTICLE_VOLUME) {
    int result = 0;
    ParticleHitIterator_iterateHit(
        reinterpret_cast<ParticleHitIterator *>(iterator),
        hit,
        &result,
        featureFlags);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_AMR &&
           featureFlags & VKL_FEATURE_FLAG_AMR_VOLUME) {
    int result = 0;
    AMRHitIterator_iterateHit(reinterpret_cast<AMRHitIterator *>(iterator),
                              hit,
                              &result,
                              featureFlags);
    return result;
  }

  else if (volumeType == VOLUME_TYPE_VDB &&
           featureFlags & VKL_FEATURE_FLAG_VDB_VOLUME) {
    int result = 0;
    VdbHitIterator_iterateHit(reinterpret_cast<VdbHitIterator *>(iterator),
                              hit,
                              &result,
                              featureFlags);
    return result;
  }

  else {
    assert(false);
    return false;
  }
}