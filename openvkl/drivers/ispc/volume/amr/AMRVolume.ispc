// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ours
//#include "CellRef.ih"
// #include "AMRCommon.h"
#include "AMR.ih"
#include "AMRVolume.ih"
#include "common/export_util.h"
#include "sampler/Sampler.ih"

static float AMRVolume_computeSample(const Sampler *uniform sampler,
                                     const varying vec3f &worldCoordinates)
{
  const AMRVolume *uniform self = (const AMRVolume *uniform)sampler->volume;
  return self->computeSample_varying(self, worldCoordinates);
}

static vec3f AMRVolume_computeGradient(const Sampler *uniform sampler,
                                       const varying vec3f &pos)
{
  // Cast to the actual Volume subtype.
  const AMRVolume *uniform self = (const AMRVolume *uniform)sampler->volume;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep = make_vec3f(
      self->samplingStep * .1f);  // Carson TODO: determine correct offset

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  // Forward differences.

  // Sample at gradient location.
  float sample = self->computeSample_varying(self, pos);

  // Gradient magnitude in the X direction.
  gradient.x = self->computeSample_varying(
                   self, pos + make_vec3f(gradientStep.x, 0.0f, 0.0f)) -
               sample;

  // Gradient magnitude in the Y direction.
  gradient.y = self->computeSample_varying(
                   self, pos + make_vec3f(0.0f, gradientStep.y, 0.0f)) -
               sample;

  // Gradient magnitude in the Z direction.
  gradient.z = self->computeSample_varying(
                   self, pos + make_vec3f(0.0f, 0.0f, gradientStep.z)) -
               sample;

  // This approximation may yield image artifacts.
  return (gradient / gradientStep);
}

export void *uniform EXPORT_UNIQUE(AMRVolume_create, void *uniform cppE)
{
  AMRVolume *uniform self = uniform new uniform AMRVolume;
  memset(self, 0, sizeof(uniform AMRVolume));
  return self;
}

export void EXPORT_UNIQUE(AMRVolume_Destructor, void *uniform _self)
{
  AMRVolume *uniform volume = (AMRVolume * uniform) _self;
  delete volume;
}

export void EXPORT_UNIQUE(AMRVolume_computeValueRangeOfLeaf,
                          const void *uniform _self,
                          uniform int leafID)
{
  const AMRVolume *uniform self = (const AMRVolume *uniform)_self;

  const AMR *uniform amr      = &self->amr;
  AMRLeaf *uniform leaf       = amr->leaf + leafID;
  AMRBrick *uniform brick     = leaf->brickList[0];
  uniform float leafCellWidth = brick->cellWidth;
  uniform vec3f leafSize      = brick->bounds.upper - brick->bounds.lower;
  uniform vec3i leafCells =
      make_vec3i((leafSize + 0.5f * leafCellWidth) * rcp(leafCellWidth));

  uniform vec3i numSamplePoints = 2 * leafCells + 1;
  for (uniform int iz = 0; iz < numSamplePoints.z; iz++)
    for (uniform int iy = 0; iy < numSamplePoints.y; iy++)
      for (varying int ix = 0; ix < numSamplePoints.x; ix += programCount) {
        vec3f relPos    = make_vec3f(ix, iy, iz) / make_vec3f(numSamplePoints);
        vec3f samplePos = lerp(leaf->bounds, relPos);
        float sampleValue =
            self->computeSample_varying(self, samplePos);
        extend(leaf->valueRange, sampleValue);
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
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const AMRVolume *uniform self = (const AMRVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = self->computeSample_varying(self, *objectCoordinates);
  }
}

export void EXPORT_UNIQUE(AMRVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          void *uniform _gradients)
{
  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients = AMRVolume_computeGradient((const Sampler *uniform)_sampler,
                                           *objectCoordinates);
  }
}

export Sampler *uniform EXPORT_UNIQUE(AMRSampler_create, void *uniform _volume)
{
  Sampler *uniform sampler         = CALL_ISPC(Sampler_create, _volume);
  sampler->computeSample_varying   = AMRVolume_computeSample;
  sampler->computeGradient_varying = AMRVolume_computeGradient;
  return sampler;
}

export void EXPORT_UNIQUE(AMRSampler_destroy, void *uniform _sampler)
{
  CALL_ISPC(Sampler_destroy, _sampler);
}
