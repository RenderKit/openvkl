// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"

#include "../UnstructuredSamplerShared.h"
#include "AMR.ih"
#include "AMRVolumeShared.h"
#include "../../common/export_util.h"
#include "../../sampler/Sampler.ih"

static vec3f AMRVolume_computeGradient(const SamplerShared *uniform sampler,
                                       const varying vec3f &pos)
{
  // Cast to the actual Volume subtype.
  const AMRVolume *uniform volume = (const AMRVolume *uniform)sampler->volume;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep = make_vec3f(
      volume->samplingStep * .1f);  // Carson TODO: determine correct offset

  // Fixed time.
  varying float time = 0.f;

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  // Forward differences.

  // Sample at gradient location.
  float sample = sampler->computeSample_varying(sampler, pos, 0, time);

  // Gradient magnitude in the X direction.
  gradient.x =
      sampler->computeSample_varying(
          sampler, pos + make_vec3f(gradientStep.x, 0.0f, 0.0f), 0, time) -
      sample;

  // Gradient magnitude in the Y direction.
  gradient.y =
      sampler->computeSample_varying(
          sampler, pos + make_vec3f(0.0f, gradientStep.y, 0.0f), 0, time) -
      sample;

  // Gradient magnitude in the Z direction.
  gradient.z =
      sampler->computeSample_varying(
          sampler, pos + make_vec3f(0.0f, 0.0f, gradientStep.z), 0, time) -
      sample;

  // This approximation may yield image artifacts.
  return (gradient / gradientStep);
}

export void EXPORT_UNIQUE(AMRVolume_Constructor, void *uniform _self)
{
  uniform AMRVolume *uniform self = (uniform AMRVolume * uniform) _self;
  memset(self, 0, sizeof(uniform AMRVolume));
}

export void EXPORT_UNIQUE(AMRVolume_Destructor, void *uniform _self) {}

export void EXPORT_UNIQUE(AMRVolume_computeValueRangeOfLeaf,
                          const void *uniform _self,
                          uniform int leafID)
{
  const AMRVolume *uniform self = (const AMRVolume *uniform)_self;

  const AMR *uniform amr  = &self->amr;
  AMRLeaf *uniform leaf   = amr->leaf + leafID;
  AMRBrick *uniform brick = leaf->brickList[0];

  assert(brick->value.dataType == VKL_FLOAT);

  for (uniform uint64 i = 0; i < brick->value.numItems; i++) {
    extend(leaf->valueRange, get_float(brick->value, i));
  }
}

export void EXPORT_UNIQUE(AMRVolume_setAMR,
                          void *uniform _self,
                          uniform int numNodes,
                          void *uniform _node,
                          uniform int numLeaves,
                          void *uniform _leaf,
                          uniform int numLevels,
                          void *uniform _level,
                          const uniform int voxelType,
                          const uniform box3f &worldBounds)
{
  AMRVolume *uniform self = (AMRVolume * uniform) _self;

  self->amr.worldBounds          = worldBounds;
  self->amr.maxValidPos          = nextafter(worldBounds.upper, -1.f);
  self->amr.node                 = (KDTreeNode * uniform) _node;
  self->amr.numNodes             = numNodes;
  self->amr.leaf                 = (AMRLeaf * uniform) _leaf;
  self->amr.numLeaves            = numLeaves;
  self->amr.level                = (AMRLevel * uniform) _level;
  self->amr.finestLevel          = self->amr.level + numLevels - 1;
  self->amr.numLevels            = numLevels;
  self->amr.finestLevelCellWidth = self->amr.level[numLevels - 1].cellWidth;

  if (voxelType == VKL_UCHAR) {
    self->amr.getVoxel = AMR_getVoxel_uint8_32;
  } else if (voxelType == VKL_SHORT) {
    self->amr.getVoxel = AMR_getVoxel_int16_32;
  } else if (voxelType == VKL_USHORT) {
    self->amr.getVoxel = AMR_getVoxel_uint16_32;
  } else if (voxelType == VKL_FLOAT) {
    self->amr.getVoxel = AMR_getVoxel_float_32;
  } else if (voxelType == VKL_DOUBLE) {
    self->amr.getVoxel = AMR_getVoxel_double_32;
  } else {
    print("#osp:amrVolume unsupported voxelType");
    return;
  }
}

export void EXPORT_UNIQUE(AMRVolume_setBvh,
                          void *uniform _self,
                          const void *uniform bvhRoot)
{
  AMRVolume *uniform self = (AMRVolume * uniform) _self;

  // sets members of VKLUnstructuredBase required for interval iteration
  self->super.boundingBox = self->boundingBox;
  self->super.bvhRoot     = (uniform Node * uniform) bvhRoot;
}

export void EXPORT_UNIQUE(AMRVolume_set,
                          void *uniform _self,
                          uniform box3f &worldBounds,
                          uniform float samplingStep,
                          const uniform vec3f &gridOrigin,
                          const uniform vec3f &gridSpacing)
{
  AMRVolume *uniform self = (AMRVolume * uniform) _self;
  self->boundingBox =
      make_box3f(worldBounds.lower + gridOrigin,
                 worldBounds.lower + gridOrigin +
                     (worldBounds.upper - worldBounds.lower) * gridSpacing);
  self->samplingStep = samplingStep;
  self->gridSpacing  = gridSpacing;
  self->gridOrigin   = gridOrigin;
}

export void EXPORT_UNIQUE(AMRVolume_sample_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _samples)
{
  const UnstructuredSamplerShared *uniform usampler =
      (const UnstructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = &usampler->super.super;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    float time                     = 0.f;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples =
        sampler->computeSample_varying(sampler, *objectCoordinates, 0, time);
  }
}

export void EXPORT_UNIQUE(AMRVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _gradients)
{
  const UnstructuredSamplerShared *uniform usampler =
      (const UnstructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = &usampler->super.super;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients = AMRVolume_computeGradient(sampler, *objectCoordinates);
  }
}

export void EXPORT_UNIQUE(AMRSampler_create,
                          void *uniform _volume,
                          void *uniform _sampler)
{
  UnstructuredSamplerShared *uniform usampler =
      (UnstructuredSamplerShared * uniform) _sampler;
  memset(usampler, 0, sizeof(uniform UnstructuredSamplerShared));
  SamplerShared *uniform sampler = &usampler->super.super;

  sampler->volume = (const VolumeShared *uniform)_volume;

  // computeSample functions set in AMR_install* functions

  sampler->computeGradient_varying = AMRVolume_computeGradient;
}

export void EXPORT_UNIQUE(AMRSampler_destroy, void *uniform _sampler) {}
