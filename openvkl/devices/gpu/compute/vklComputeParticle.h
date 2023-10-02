// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../cpu/volume/particle/ParticleVolumeShared.h"

namespace ispc {

  inline void getParticleContributionsGaussian(const VKLParticleVolume *self,
                                               const uint64 ids,
                                               const vec3f &objectCoordinates,
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

  inline bool intersectAndGradientParticle(const void *userData,
                                           uint64 numIds,
                                           uint64 *ids,
                                           vec3f &result,
                                           vec3f samplePos)
  {
    const VKLParticleVolume *self = (const VKLParticleVolume *uniform)userData;

    for (uint64 i = 0; i < numIds; i++) {
      float value;
      vec3f delta;
      getParticleContributionsGaussian(self, ids[i], samplePos, value, delta);

      const float radius = get_float(self->radii, ids[i]);

      const vec3f g = delta * value / (radius * radius);

      result = result - g;
    }

    return false;
  }

  inline bool intersectAndSampleParticle(const void *userData,
                                         uint64 numIds,
                                         uint64 *ids,
                                         float &result,
                                         vec3f samplePos)
  {
    const VKLParticleVolume *self = (const VKLParticleVolume *)userData;

    for (uint64 i = 0; i < numIds; i++) {
      float value;
      vec3f delta;
      getParticleContributionsGaussian(self, ids[i], samplePos, value, delta);

      result += value;
    }

    if (self->clampMaxCumulativeValue > 0.f) {
      result = min(result, self->clampMaxCumulativeValue);
      return (result == self->clampMaxCumulativeValue);
    }

    return false;
  }

  template <typename resultType, auto IntersectFunction>
  inline void traverseBVHMulti(Node *root,
                               const void *userPtr,
                               resultType &result,
                               const vec3f &samplePos)
  {
    Node *node = root;
    Node *nodeStack[32]; /* xxx */
    int stackPtr = 0;

    while (1) {
      bool isLeaf = (node->nominalLength.x < 0);
      if (isLeaf) {
        LeafNodeMulti *leaf = (LeafNodeMulti *)node;
        if (pointInAABBTest(leaf->super.bounds, samplePos)) {
          if (IntersectFunction(
                  userPtr, leaf->numCells, leaf->cellIDs, result, samplePos)) {
            return;
          }
        }
      } else {
        InnerNode *inner = (InnerNode *)node;
        const bool in0   = pointInAABBTest(inner->bounds[0], samplePos);
        const bool in1   = pointInAABBTest(inner->bounds[1], samplePos);

        if (in0) {
          if (in1) {
            nodeStack[stackPtr++] = inner->children[1];
            node                  = inner->children[0];
            continue;
          } else {
            node = inner->children[0];
            continue;
          }
        } else {
          if (in1) {
            node = inner->children[1];
            continue;
          } else {
            /* Do nothing, just pop. */
          }
        }
      }
      if (stackPtr == 0) {
        return;
      }
      node = nodeStack[--stackPtr];
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  // API entrypoints /////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////

  inline float VKLParticleVolume_sample(const SamplerShared *sampler,
                                        const vec3f &objectCoordinates,
                                        const float &_time,
                                        const uint32_t &_attributeIndex,
                                        const VKLFeatureFlags _featureFlags)
  {
    const VKLParticleVolume *self = (const VKLParticleVolume *)sampler->volume;

    if (!box_contains(self->super.boundingBox, objectCoordinates)) {
      return self->super.super.background[0];
    }

    float sampleResult = 0.f;

    traverseBVHMulti<float, intersectAndSampleParticle>(
        self->super.bvhRoot, sampler->volume, sampleResult, objectCoordinates);

    return sampleResult;
  }

  inline vkl_vec3f VKLParticleVolume_computeGradient(
      const SamplerShared *sampler, const vec3f &objectCoordinates)
  {
    const VKLParticleVolume *self = (const VKLParticleVolume *)sampler->volume;

    vec3f gradientResult = vec3f(0.f);

    traverseBVHMulti<vec3f, intersectAndGradientParticle>(self->super.bvhRoot,
                                                          sampler->volume,
                                                          gradientResult,
                                                          objectCoordinates);

    return vkl_vec3f{gradientResult.x, gradientResult.y, gradientResult.z};
  }

  inline void VKLParticleVolume_sampleM(const SamplerShared *sampler,
                                        const vec3f &objectCoordinates,
                                        const uint32_t M,
                                        const uint32_t *attributeIndices,
                                        float *samples,
                                        const VKLFeatureFlags _featureFlags)
  {
    const VKLParticleVolume *self = (const VKLParticleVolume *)sampler->volume;

    for (uint32_t i = 0; i < M; i++) {
      // we still support this API, but particle volumes only support a
      // single attribute
      assert(attributeIndices[i] == 0);

      samples[i] = VKLParticleVolume_sample(
          sampler, objectCoordinates, 0.f, 0, _featureFlags);
    }
  }

}  // namespace ispc