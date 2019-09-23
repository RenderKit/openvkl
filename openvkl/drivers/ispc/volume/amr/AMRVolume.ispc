// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

// ours
//#include "CellRef.ih"
// #include "AMRCommon.h"
#include "AMR.ih"
#include "AMRVolume.ih"

// ------------------------------------------------------------------
//! The current level at the given sample location in world coordinates.
static varying float AMR_level(const void *uniform _self,
                               const varying vec3f &pos)
{
  const AMRVolume *uniform self = (const AMRVolume *uniform)_self;
  return self->computeSampleLevel(self, pos);
}

// ------------------------------------------------------------------
//! The gradient at the given sample location in world coordinates.
static varying vec3f AMR_gradient(const void *uniform _self,
                                  const varying vec3f &pos)
{
  // Cast to the actual Volume subtype.
  const AMRVolume *uniform volume = (const AMRVolume *uniform)_self;

  // Gradient step in each dimension (world coordinates).
  const uniform vec3f gradientStep =
      make_vec3f(volume->samplingStep *
                 .1f);  // Carson TODO: determine correct offset

  // The gradient will be computed using central differences.
  varying vec3f gradient;

  // Forward differences.

  // Sample at gradient location.
  float sample = volume->super.computeSample(volume, pos);

  // Gradient magnitude in the X direction.
  gradient.x = volume->super.computeSample(
                   volume, pos + make_vec3f(gradientStep.x, 0.0f, 0.0f)) -
               sample;

  // Gradient magnitude in the Y direction.
  gradient.y = volume->super.computeSample(
                   volume, pos + make_vec3f(0.0f, gradientStep.y, 0.0f)) -
               sample;

  // Gradient magnitude in the Z direction.
  gradient.z = volume->super.computeSample(
                   volume, pos + make_vec3f(0.0f, 0.0f, gradientStep.z)) -
               sample;

  // This approximation may yield image artifacts.
  return (gradient / gradientStep);
}

export void *uniform AMRVolume_create(void *uniform cppE)
{
  AMRVolume *uniform self = uniform new uniform AMRVolume;
  return self;
}

export void AMRVolume_computeValueRangeOfLeaf(const void *uniform _self,
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
        float sampleValue = self->super.computeSample(_self, samplePos);
        extend(leaf->valueRange, sampleValue);
      }
}

inline void AMRVolume_transformLocalToWorld(
    const AMRVolume *uniform volume,
    const varying vec3f &localCoordinates,
    varying vec3f &worldCoordinates)
{
  worldCoordinates =
      volume->gridOrigin + localCoordinates * volume->gridSpacing;
}

inline void AMRVolume_transformWorldToLocal(
    const AMRVolume *uniform volume,
    const varying vec3f &worldCoordinates,
    varying vec3f &localCoordinates)
{
  localCoordinates =
      rcp(volume->gridSpacing) * (worldCoordinates - volume->gridOrigin);
}

export void AMRVolume_setAMR(void *uniform _self,
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

export void AMRVolume_set(void *uniform _self,
                          uniform box3f &worldBounds,
                          uniform float samplingStep,
                          const uniform vec3f &gridOrigin,
                          const uniform vec3f &gridSpacing)
{
  AMRVolume *uniform self = (AMRVolume * uniform) _self;

  self->boundingBox =
      make_box3f(gridOrigin + worldBounds.lower,
                 worldBounds.lower + gridOrigin +
                     (worldBounds.upper - worldBounds.lower) * gridSpacing);
  self->samplingStep          = samplingStep;
  self->computeGradient       = &AMR_gradient;
  self->transformLocalToWorld = AMRVolume_transformLocalToWorld;
  self->transformWorldToLocal = AMRVolume_transformWorldToLocal;

  self->gridSpacing = gridSpacing;
  self->gridOrigin  = gridOrigin;
}

export void AMRVolume_sample_export(uniform const int *uniform imask,
                                    void *uniform _self,
                                    const void *uniform _objectCoordinates,
                                    void *uniform _samples)
{
  AMRVolume *uniform self = (AMRVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = self->super.computeSample(self, *objectCoordinates);
  }
}
