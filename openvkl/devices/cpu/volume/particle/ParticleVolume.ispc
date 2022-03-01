// Copyright 2020-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../common/export_util.h"
#include "../UnstructuredSamplerBase.ih"
#include "ParticleVolume.ih"

inline void getParticleContributionsGaussian(
    const VKLParticleVolume *uniform self,
    const uint64 ids,
    const uniform vec3f &objectCoordinates,
    float &value,
    vec3f &delta)
{
  const vec3f position = get_vec3f(self->positions, ids);
  const float radius   = get_float(self->radii, ids);

  float w = 1.f;

  if (isValid(self->weights))
    w = get_float(self->weights, ids);

  delta = objectCoordinates - position;

  if (length(delta) > radius * self->radiusSupportFactor) {
    value = 0.f;
    return;
  }

  value = w * expf(-0.5f * dot(delta, delta) / (radius * radius));
}

static bool intersectAndSampleParticle(const void *uniform userData,
                                       uniform uint64 numIds,
                                       uniform uint64 *uniform ids,
                                       float &result,
                                       vec3f samplePos)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)userData;

  foreach_active(index)
  {
    uniform vec3f samplePosU = make_vec3f(extract(samplePos.x, index),
                                          extract(samplePos.y, index),
                                          extract(samplePos.z, index));

    uniform float resultU = 0.f;

    foreach (i = 0 ... numIds) {
      float value;
      vec3f delta;
      getParticleContributionsGaussian(self, ids[i], samplePosU, value, delta);

      resultU += reduce_add(value);
    }

    result += resultU;
  }

  if (self->clampMaxCumulativeValue > 0.f) {
    result = min(result, self->clampMaxCumulativeValue);
    return all(result == self->clampMaxCumulativeValue);
  }

  return false;
}

static bool intersectAndGradientParticle(const void *uniform userData,
                                         uniform uint64 numIds,
                                         uniform uint64 *uniform ids,
                                         vec3f &result,
                                         vec3f samplePos)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)userData;

  foreach_active(index)
  {
    uniform vec3f samplePosU = make_vec3f(extract(samplePos.x, index),
                                          extract(samplePos.y, index),
                                          extract(samplePos.z, index));

    uniform vec3f resultU = make_vec3f(0.f);

    foreach (i = 0 ... numIds) {
      float value;
      vec3f delta;
      getParticleContributionsGaussian(self, ids[i], samplePosU, value, delta);

      const float radius = get_float(self->radii, ids[i]);

      const vec3f g = delta * value / (radius * radius);

      resultU = resultU -
                make_vec3f(reduce_add(g.x), reduce_add(g.y), reduce_add(g.z));
    }

    result = result + resultU;
  }

  return false;
}

inline varying float VKLParticleVolume_sample(
    const Sampler *uniform sampler,
    const varying vec3f &objectCoordinates,
    const uniform uint32 _attributeIndex,
    const varying float &_time)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)sampler->volume;

  if (!box_contains(self->super.boundingBox, objectCoordinates)) {
    return self->super.super.background[0];
  }

  float sampleResult = 0.f;

  traverseBVHMulti(self->super.bvhRoot,
                   sampler->volume,
                   intersectAndSampleParticle,
                   sampleResult,
                   objectCoordinates);

  return sampleResult;
}

inline varying vec3f VKLParticleVolume_computeGradient(
    const Sampler *uniform sampler, const varying vec3f &objectCoordinates)
{
  const VKLParticleVolume *uniform self =
      (const VKLParticleVolume *uniform)sampler->volume;

  vec3f gradientResult = make_vec3f(0.f);

  traverseBVHMulti(self->super.bvhRoot,
                   sampler->volume,
                   intersectAndGradientParticle,
                   gradientResult,
                   objectCoordinates);

  return gradientResult;
}

export void EXPORT_UNIQUE(VKLParticleVolume_sample_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float time             = 0.f;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = VKLParticleVolume_sample(
        (const Sampler *uniform)_sampler, *objectCoordinates, 0, time);
  }
}

export void EXPORT_UNIQUE(VKLParticleVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _gradients)
{
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients = VKLParticleVolume_computeGradient(
        (const Sampler *uniform)_sampler, *objectCoordinates);
  }
}

export void *uniform EXPORT_UNIQUE(VKLParticleVolume_Constructor)
{
  uniform VKLParticleVolume *uniform self =
      uniform new uniform VKLParticleVolume;
  memset(self, 0, sizeof(uniform VKLParticleVolume));

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

export UnstructuredSamplerBase *uniform
EXPORT_UNIQUE(VKLParticleSampler_Constructor, void *uniform _volume)
{
  UnstructuredSamplerBase *uniform sampler =
      uniform new UnstructuredSamplerBase;
  memset(sampler, 0, sizeof(uniform UnstructuredSamplerBase));

  sampler->super.volume                  = (const Volume *uniform)_volume;
  sampler->super.computeSample_varying   = VKLParticleVolume_sample;
  sampler->super.computeGradient_varying = VKLParticleVolume_computeGradient;

  return sampler;
}

export void EXPORT_UNIQUE(VKLParticleSampler_Destructor, void *uniform _sampler)
{
  UnstructuredSamplerBase *uniform sampler =
      (UnstructuredSamplerBase * uniform) _sampler;
  delete sampler;
}
