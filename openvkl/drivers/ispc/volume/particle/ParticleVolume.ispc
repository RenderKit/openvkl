// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../common/export_util.h"
#include "ParticleVolume.ih"

inline void getParticleContributionGaussian(const VKLParticleVolume *uniform
                                                self,
                                            const uniform uint64 id,
                                            const vec3f &objectCoordinates,
                                            float &value,
                                            vec3f &distance)
{
  const uniform vec3f position = get_vec3f(self->positions, id);
  const uniform float radius   = get_float(self->radii, id);

  uniform float w = 1.f;

  if (valid(self->weights))
    w = get_float(self->weights, id);

  distance = objectCoordinates - position;

  if (length(distance) > radius * self->radiusSupportFactor) {
    value = 0.f;
    return;
  }

  value = w * expf(-0.5f * dot(distance, distance) / (radius * radius));
}

static bool intersectAndSampleParticle(const void *uniform userData,
                                       uniform uint64 id,
                                       float &result,
                                       vec3f samplePos)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)userData;

  float value;
  vec3f distance;
  getParticleContributionGaussian(self, id, samplePos, value, distance);

  result += value;

  if (self->clampMaxCumulativeValue > 0.f) {
    result = min(result, self->clampMaxCumulativeValue);
    return all(result == self->clampMaxCumulativeValue);
  }

  return false;
}

static bool intersectAndGradientParticle(const void *uniform userData,
                                         uniform uint64 id,
                                         vec3f &result,
                                         vec3f samplePos)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)userData;

  float value;
  vec3f distance;
  getParticleContributionGaussian(self, id, samplePos, value, distance);

  const uniform float radius = get_float(self->radii, id);

  result = result - distance * value / (radius * radius);

  return false;
}

inline varying float VKLParticleVolume_sample(
    const Volume *uniform _self, const varying vec3f &objectCoordinates)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)_self;

  float sampleResult = 0.f;

  traverseEmbree(self->super.bvhRoot,
                 _self,
                 intersectAndSampleParticle,
                 sampleResult,
                 objectCoordinates);

  return sampleResult;
}

inline varying vec3f VKLParticleVolume_computeGradient(
    const Volume *uniform _self, const varying vec3f &objectCoordinates)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)_self;

  vec3f gradientResult = make_vec3f(0.f);

  traverseEmbree(self->super.bvhRoot,
                 _self,
                 intersectAndGradientParticle,
                 gradientResult,
                 objectCoordinates);

  return gradientResult;
}

export void EXPORT_UNIQUE(VKLParticleVolume_sample_export,
                          uniform const int *uniform imask,
                          void *uniform _volume,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = VKLParticleVolume_sample((Volume *uniform)_volume, *objectCoordinates);
  }
}

export void EXPORT_UNIQUE(VKLParticleVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _volume,
                          const void *uniform _objectCoordinates,
                          void *uniform _gradients)
{
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients = VKLParticleVolume_computeGradient((Volume *uniform)_volume, *objectCoordinates);
  }
}

export void *uniform EXPORT_UNIQUE(VKLParticleVolume_Constructor)
{
  uniform VKLParticleVolume *uniform self =
      uniform new uniform VKLParticleVolume;
  memset(self, 0, sizeof(uniform VKLParticleVolume));

  self->super.super.computeSample_varying = VKLParticleVolume_sample;

  return self;
}

export void EXPORT_UNIQUE(VKLParticleVolume_Destructor, void *uniform _self)
{
  VKLParticleVolume *uniform volume = (VKLParticleVolume * uniform) _self;
  delete volume;
}

export void EXPORT_UNIQUE(VKLParticleVolume_set,
                          void *uniform _self,
                          const uniform box3f &_bbox,
                          const Data1D *uniform _positions,
                          const Data1D *uniform _radii,
                          const Data1D *uniform _weights,
                          const uniform float _radiusSupportFactor,
                          const uniform float _clampMaxCumulativeValue,
                          const void *uniform bvhRoot)
{
  uniform VKLParticleVolume *uniform self =
      (uniform VKLParticleVolume * uniform) _self;

  self->positions               = *_positions;
  self->radii                   = *_radii;
  self->weights                 = *_weights;
  self->radiusSupportFactor     = _radiusSupportFactor;
  self->clampMaxCumulativeValue = _clampMaxCumulativeValue;
  self->super.boundingBox       = _bbox;
  self->super.bvhRoot           = (uniform Node * uniform) bvhRoot;
}
