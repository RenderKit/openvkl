// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Dda.ih"

inline int safe_sign(float v)
{
  return ((int)(0 < v)) - ((int)(v < 0));
}

inline vec3i safe_sign(const vec3f &v)
{
  return make_vec3i(safe_sign(v.x), safe_sign(v.y), safe_sign(v.z));
}

inline float safe_rcp(float v)
{
  return (v == -0) ? -inf : (v == 0) ? inf : rcp(v);
}

inline vec3f safe_rcp(const vec3f &v)
{
  return make_vec3f(safe_rcp(v.x), safe_rcp(v.y), safe_rcp(v.z));
}

/*
 * Compare __vkl_vdb_map_offset_to_voxel in VdbUtil.ih.
 */
inline vec3i clampToCell(
    const vec3f &foffset,
    int resolution)  // ... of a single cell on the current level.
{
  // Offsets are non-negative, but numerical errors might cause problems.
  const vec3i offset = make_vec3i(((int)floor(max(foffset.x, 0))),
                                  ((int)floor(max(foffset.y, 0))),
                                  ((int)floor(max(foffset.z, 0))));

  // We may map a voxel coordinate to the origin of a voxel with resolution
  // logVoxelRes using this simple mask because resolutions are powers of two.
  assert(popcnt((int)resolution) == 1);
  const int mask = ~((resolution)-1);
  return make_vec3i(offset.x & mask, offset.y & mask, offset.z & mask);
}

/*
 * Intersect a set of three axis-aligned hyperplanes.
 */
inline vec3f intersect_planes(const vec3f &rayOrg,
                              const vec3f &rayInvDir,
                              const vec3f &planes)
{
  return (planes - rayOrg) * rayInvDir;
}

/*
 * Intersect an axis-aligned box with the given ray.
 */
inline void intersect_box(const DdaRayState &rayState,
                          const vec3f &boxMin,
                          const vec3f &boxMax,
                          float &tEnter,
                          float &tExit)
{
  const vec3f pmins =
      intersect_planes(rayState.rayOrigin, rayState.iDir, boxMin);
  const vec3f pmaxs =
      intersect_planes(rayState.rayOrigin, rayState.iDir, boxMax);
  const vec3f mins = min(pmins, pmaxs);
  const vec3f maxs = max(pmins, pmaxs);
  tEnter = max(mins.x, max(mins.y, max(mins.z, rayState.tRange.lower)));
  tExit  = min(maxs.x, min(maxs.y, min(maxs.z, rayState.tRange.upper)));
}

void ddaInitRay(const vec3f &rayOrg,
                const vec3f &rayDir,
                const box1f &tRange,
                DdaRayState &rayState)
{
  assert(tRange.lower >= 0.f);
  assert(tRange.lower <= tRange.upper);

  rayState.rayOrigin = rayOrg;
  rayState.rayDir    = rayDir;
  rayState.tRange    = tRange;

  // We need the inverse direction for both the bbox intersection and to find
  // the distance between hyperplane intersection in each direction.
  rayState.iDir    = safe_rcp(rayDir);
  rayState.dirSign = safe_sign(rayDir);
}

void ddaInitLevel(const DdaRayState &rayState,
                  uniform unsigned int logCellRes,
                  uniform unsigned int logDomainRes,
                  DdaLevelState &levelState)
{
  assert(logDomainRes < 32);
  assert(logCellRes < 32);
  assert(logCellRes <= logDomainRes);
  levelState.domainRes = (1 << logDomainRes);
  levelState.cellRes   = (1 << logCellRes);
  // In each step, we will advance one dimension of the current index by this
  // amount.
  levelState.idxDelta = rayState.dirSign * levelState.cellRes;

  // tDelta is the distance, along the ray, between two hyperplane
  // intersections: Let cellRes be the distance between two hyperplanes in
  // x-direction.
  levelState.tDelta = ((float)levelState.cellRes) * abs(rayState.iDir);
}

/*
 * DDA optimized for hierarchical grids, where levels have resolutions that are
 * powers of two.
 */
void ddaInitSegment(
    const DdaRayState &rayState,
    const DdaLevelState &levelState,
    const vec3i &cellOffset,  // The iteration cell starts at this offset.
    DdaSegmentState &segmentState)
{
  // The index-space bounding box of the region we are going to traverse.
  segmentState.domainBegin = cellOffset;
  segmentState.domainEnd   = segmentState.domainBegin + levelState.domainRes;
  const vec3f bboxMin      = make_vec3f(segmentState.domainBegin.x,
                                   segmentState.domainBegin.y,
                                   segmentState.domainBegin.z);
  const vec3f bboxMax      = make_vec3f(segmentState.domainEnd.x,
                                   segmentState.domainEnd.y,
                                   segmentState.domainEnd.z);
  float tEnter             = 0;
  float tExit              = 0;
  intersect_box(rayState, bboxMin, bboxMax, tEnter, tExit);

  if (tEnter > tExit) {
    segmentState.t    = inf;
    segmentState.tMax = 0;
    assert(ddaStateHasExited(segmentState));
  } else {
    segmentState.t    = tEnter;
    segmentState.tMax = tExit;

    // Indices of the voxel where the ray enters the grid.
    const vec3f pEnter = rayState.rayOrigin + tEnter * rayState.rayDir;
    segmentState.idx   = clampToCell(pEnter, levelState.cellRes);
    // We know that pEnter is somewhere inside our domain, or on the domain
    // surface. If it is exactly on the boundary, idx might be just outside the
    // domain. We clamp to fix this problem.
    segmentState.idx = min(max(segmentState.domainBegin, segmentState.idx),
                           segmentState.domainEnd - levelState.cellRes);

    // We are currently somewhere in the interval [segmentState.idx,
    // segmentState.idx-idxDelta]. Intersect these planes to determine tNext.
    const vec3i dirPositive = make_vec3i((int)(rayState.dirSign.x >= 0),
                                         (int)(rayState.dirSign.y >= 0),
                                         (int)(rayState.dirSign.z >= 0));
    const vec3i exitPlane =
        segmentState.idx + dirPositive * levelState.idxDelta;
    segmentState.tNext =
        intersect_planes(rayState.rayOrigin,
                         rayState.iDir,
                         make_vec3f(exitPlane.x, exitPlane.y, exitPlane.z));
  }
}
