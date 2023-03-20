// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbSampler_leafHandler.h"
#include "VdbSampler_packedHandler.h"

#include "openvkl_vdb/VdbSamplerDispatchInner.h"

// ---------------------------------------------------------------------------
// Find the origin of the leaf node containing domainOffset.
// Note: Performance is significantly better when these functions return
//       the leaf origin as opposed to having a reference parameter.
// ---------------------------------------------------------------------------

inline vec3ui VdbSampler_toLeafOrigin(const vec3ui &domainOffset)
{
  static const uint32 originMask = ~(((uint32)(VKL_VDB_RES_LEAF - 1)));
  return make_vec3ui(domainOffset.x & originMask,
                     domainOffset.y & originMask,
                     domainOffset.z & originMask);
}

// ---------------------------------------------------------------------------
// Map the given index space coordinate to a (root-relative) domain offset.
// Note that we do not check if the ic-rootOrigin is positive here; instead
// we rely on the fact that negative numbers in the subtraction below will be
// mapped very large values in the conversion.
// ---------------------------------------------------------------------------

inline vec3ui VdbSampler_toDomainOffset(const vec3i &indexCoord,
                                        const vec3i &rootOrigin)
{
  return make_vec3ui(indexCoord.x - rootOrigin.x,
                     indexCoord.y - rootOrigin.y,
                     indexCoord.z - rootOrigin.z);
}

// ---------------------------------------------------------------------------

inline bool VdbSampler_isInDomain(const vec3ui &activeSize,
                                  const vec3ui &domainOffset)
{
  return domainOffset.x < activeSize.x && domainOffset.y < activeSize.y &&
         domainOffset.z < activeSize.z;
}

// ---------------------------------------------------------------------------
// Clip the index space coordinate to the grid bounding box, and traverse
// the hierarchy to find the correct voxel.
// ---------------------------------------------------------------------------

// Uniform traversal.
inline void VdbSampler_traverse(const VdbSamplerShared *sampler,
                                const vec3i &ic,
                                uint64 &voxel,
                                vec3ui &domainOffset)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);
  assert(!sampler->grid->dense);

  voxel        = openvkl::cpu_device::vklVdbVoxelMakeEmpty();
  domainOffset = VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);

  if (VdbSampler_isInDomain(sampler->grid->activeSize, domainOffset)) {
    const vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);
    VdbSampler_dispatchInner_uniform_uniform_0(sampler, 0ul, leafOrigin, voxel);
  }
}

// ---------------------------------------------------------------------------
// Given a domain relative index space coordinate and a voxel code, sample the
// leaf node.
// ---------------------------------------------------------------------------

// These helper functions handle all legal combinations of sampling uniform /
// varying voxels at uniform / varying offsets, and are used just below.
inline float VdbSampler_sample_inner(const VdbSamplerShared *sampler,
                                     const uint64 &voxel,
                                     const vec3ui &domainOffset,
                                     const float &time,
                                     const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);
  float sample        = 0.f;
  const VdbGrid *grid = sampler->grid;

  if (openvkl::cpu_device::vklVdbVoxelIsLeafPtr(voxel)) {
    const vkl_uint32 voxelType = sampler->grid->attributeTypes[attributeIndex];
    const uint64 leafIndex =
        openvkl::cpu_device::vklVdbVoxelLeafGetIndex(voxel);
    if (sampler->grid->nodesPackedDense) {
      __vkl_vdb_packed_handler(
          sample = VdbSampler_sample_uniform_uniform_packed,
          grid->packedAddressing32,
          voxelType,
          openvkl::cpu_device::vklVdbVoxelLeafGetFormat(voxel),
          grid,
          leafIndex,
          attributeIndex,
          domainOffset,
          time)
    } else {
      __vkl_vdb_leaf_handler(
          sample = VdbSampler_sample_uniform_uniform,
          voxelType,
          openvkl::cpu_device::vklVdbVoxelLeafGetFormat(voxel),
          grid->allLeavesConstant,
          openvkl::cpu_device::vklVdbVoxelLeafGetTemporalFormat(voxel),
          grid,
          leafIndex,
          openvkl::cpu_device::vklVdbGetLeafDataIndex(
              grid, leafIndex, attributeIndex),
          domainOffset,
          time)
    }
  } else if (openvkl::cpu_device::vklVdbVoxelIsError(voxel)) {
    uint8 level;
    uint32 voxelOffset;
    openvkl::cpu_device::vklVdbVoxelErrorGet(voxel, level, voxelOffset);
    const range1f valueRange =
        grid->levels[level]
            .valueRange[voxelOffset * grid->numAttributes + attributeIndex];
    sample = 0.5f * (valueRange.lower + valueRange.upper);
  } else if (!VdbSampler_isInDomain(grid->activeSize, domainOffset)) {
    sample = sampler->super.super.volume->background[attributeIndex];
  }
  return sample;
}

// Uniform sampling.
// Note: This function may seem unnecessary in the uniform path. However,
// keeping this thin wrapper ensures that downstream code is as uniform as
// possible.
inline float VdbSampler_sample(const VdbSamplerShared *sampler,
                               const uint64 voxel,
                               const vec3ui &domainOffset,
                               const float time,
                               const uint32 attributeIndex)
{
  assert(!sampler->grid->dense);

  return VdbSampler_sample_inner(
      sampler, voxel, domainOffset, time, attributeIndex);
}

// ---------------------------------------------------------------------------
// Traverse and sample in a single call.
//
// This is generally more efficient than calling traverse() and sample()
// separately if only a single element needs to be looked up.
// ---------------------------------------------------------------------------

inline float VdbSampler_traverseAndSample(const VdbSamplerShared *sampler,
                                          const vec3i &ic,
                                          const float time,
                                          const uint32 attributeIndex)
{
  assert(sampler);
  assert(sampler->grid);
  assert(sampler->grid->levels[0].numNodes == 1);
  assert(!sampler->grid->dense);

  const vec3ui domainOffset =
      VdbSampler_toDomainOffset(ic, sampler->grid->rootOrigin);

  if (VdbSampler_isInDomain(sampler->grid->activeSize, domainOffset)) {
    const vec3ui leafOrigin = VdbSampler_toLeafOrigin(domainOffset);
    uint64 voxelU           = openvkl::cpu_device::vklVdbVoxelMakeEmpty();
    VdbSampler_dispatchInner_uniform_uniform_0(
        sampler, 0ul, leafOrigin, voxelU);
    return VdbSampler_sample_inner(
        sampler, voxelU, domainOffset, time, attributeIndex);
  }

  return sampler->super.super.volume->background[attributeIndex];
}
