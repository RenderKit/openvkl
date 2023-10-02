// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "GridAccelerator.ih"
#include "SharedStructuredVolume.ih"
#include "StructuredSamplerShared.h"

// #define PRINT_DEBUG_ENABLE
#include "../common/print_debug.ih"

///////////////////////////////////////////////////////////////////////////////
// Macro helpers //////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// declares a macro for all voxel types
#define declare_all_voxel_types(macro) \
  macro(uint8);                        \
  macro(int16);                        \
  macro(uint16);                       \
  macro(half);                         \
  macro(float);                        \
  macro(double);

// declares a macro for all voxel type and uniform / varying combinations
#define declare_all_voxel_types_univary(macro) \
  macro(uint8, varying);                       \
  macro(int16, varying);                       \
  macro(uint16, varying);                      \
  macro(half, varying);                        \
  macro(float, varying);                       \
  macro(double, varying);                      \
                                               \
  macro(uint8, uniform);                       \
  macro(int16, uniform);                       \
  macro(uint16, uniform);                      \
  macro(half, uniform);                        \
  macro(float, uniform);                       \
  macro(double, uniform);

///////////////////////////////////////////////////////////////////////////////
// Motion blur helper functions ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline uniform bool hasStructuredTimeData(
    const SharedStructuredVolume *uniform self)
{
  return self->temporallyStructuredNumTimesteps > 1;
}

inline uniform bool hasUnstructuredTimeData(
    const SharedStructuredVolume *uniform self)
{
  return isValid(self->temporallyUnstructuredIndices) &&
         self->temporallyUnstructuredIndices.numItems > 0;
}

// Supports 32-bit and 64/32-bit addressing modes on
// temporallyUnstructuredIndices
#define template_interpolateVoxelTUV(type, univary)                          \
  inline univary float interpolateVoxelTUV_##type##_##univary(               \
      const SharedStructuredVolume *uniform self,                            \
      const uniform uint32 attributeIndex,                                   \
      const uniform uint64 fixed0Ofs,                                        \
      const univary uint32 timeOfs,                                          \
      const univary float &time)                                             \
  {                                                                          \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];   \
    const uniform Data1D indices   = self->temporallyUnstructuredIndices;    \
    const uniform Data1D times     = self->temporallyUnstructuredTimes;      \
                                                                             \
    /* unconditionally use 64-bit indices on voxelData and times; this       \
       could be improved... */                                               \
    univary uint64 voxelOfs, nextOfs;                                        \
    if (indices.dataType == VKL_UINT) {                                      \
      voxelOfs = get_uint32(indices, fixed0Ofs, timeOfs);                    \
      nextOfs  = get_uint32(indices, fixed0Ofs + 1, timeOfs);                \
    } else {                                                                 \
      voxelOfs = get_uint64(indices, fixed0Ofs, timeOfs);                    \
      nextOfs  = get_uint64(indices, fixed0Ofs + 1, timeOfs);                \
    }                                                                        \
                                                                             \
    assert(nextOfs > voxelOfs);                                              \
    const univary float t0 = get_float(times, voxelOfs);                     \
    if (time <= t0)                                                          \
      return get_##type(voxelData, voxelOfs);                                \
                                                                             \
    const univary float t1 = get_float(times, nextOfs - 1);                  \
    if (time >= t1)                                                          \
      return get_##type(voxelData, nextOfs - 1);                             \
                                                                             \
    const univary uint64 numTimeSamples = nextOfs - voxelOfs;                \
    if (numTimeSamples == 2) {                                               \
      const univary float w = (time - t0) * rcp(t1 - t0);                    \
      return (1.f - w) * get_##type(voxelData, voxelOfs) +                   \
             w * get_##type(voxelData, voxelOfs + 1);                        \
    }                                                                        \
                                                                             \
    univary uint64 lower = 0;                                                \
    univary uint64 upper = numTimeSamples;                                   \
    while (lower <= upper) {                                                 \
      const univary uint64 mid     = (lower + upper) / 2;                    \
      const univary float timesMid = get_float(times, voxelOfs + mid);       \
      if (time >= timesMid &&                                                \
          time <= get_float(times, voxelOfs + mid + 1)) {                    \
        lower = voxelOfs + mid;                                              \
        upper = lower + 1;                                                   \
        break;                                                               \
      } else if (time < timesMid) {                                          \
        upper = mid;                                                         \
      } else {                                                               \
        lower = mid;                                                         \
      }                                                                      \
    }                                                                        \
                                                                             \
    const univary float timesLow  = get_float(times, lower);                 \
    const univary float timesHigh = get_float(times, upper);                 \
    const univary float interpolant =                                        \
        (time - timesLow) * rcp(timesHigh - timesLow);                       \
                                                                             \
    return (1.f - interpolant) * get_##type(voxelData, lower) +              \
           interpolant * get_##type(voxelData, upper);                       \
  }

declare_all_voxel_types_univary(template_interpolateVoxelTUV);

#undef template_interpolateVoxelTUV

// Supports 64-bit addressing mode on temporallyUnstructuredIndices
#define template_interpolateVoxelTUV(type, univary)                          \
  inline univary float interpolateVoxelTUV_##type##_##univary(               \
      const SharedStructuredVolume *uniform self,                            \
      const uniform uint32 attributeIndex,                                   \
      const univary uint64 timeOfs,                                          \
      const univary float &time)                                             \
  {                                                                          \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];   \
    const uniform Data1D indices   = self->temporallyUnstructuredIndices;    \
    const uniform Data1D times     = self->temporallyUnstructuredTimes;      \
                                                                             \
    assert(indices.dataType == VKL_ULONG);                                   \
    univary uint64 voxelOfs = get_uint64(indices, timeOfs);                  \
    univary uint64 nextOfs  = get_uint64(indices, timeOfs + 1);              \
                                                                             \
    assert(nextOfs > voxelOfs);                                              \
    const univary float t0 = get_float(times, voxelOfs);                     \
    if (time <= t0)                                                          \
      return get_##type(voxelData, voxelOfs);                                \
                                                                             \
    const univary float t1 = get_float(times, nextOfs - 1);                  \
    if (time >= t1)                                                          \
      return get_##type(voxelData, nextOfs - 1);                             \
                                                                             \
    const univary uint64 numTimeSamples = nextOfs - voxelOfs;                \
    if (numTimeSamples == 2) {                                               \
      const univary float w = (time - t0) * rcp(t1 - t0);                    \
      return (1.f - w) * get_##type(voxelData, voxelOfs) +                   \
             w * get_##type(voxelData, voxelOfs + 1);                        \
    }                                                                        \
                                                                             \
    univary uint64 lower = 0;                                                \
    univary uint64 upper = numTimeSamples;                                   \
    while (lower <= upper) {                                                 \
      const univary uint64 mid     = (lower + upper) / 2;                    \
      const univary float timesMid = get_float(times, voxelOfs + mid);       \
      if (time >= timesMid &&                                                \
          time <= get_float(times, voxelOfs + mid + 1)) {                    \
        lower = voxelOfs + mid;                                              \
        upper = lower + 1;                                                   \
        break;                                                               \
      } else if (time < timesMid) {                                          \
        upper = mid;                                                         \
      } else {                                                               \
        lower = mid;                                                         \
      }                                                                      \
    }                                                                        \
                                                                             \
    const univary float timesLow  = get_float(times, lower);                 \
    const univary float timesHigh = get_float(times, upper);                 \
    const univary float interpolant =                                        \
        (time - timesLow) * rcp(timesHigh - timesLow);                       \
                                                                             \
    return (1.f - interpolant) * get_##type(voxelData, lower) +              \
           interpolant * get_##type(voxelData, upper);                       \
  }

declare_all_voxel_types_univary(template_interpolateVoxelTUV);

#undef template_interpolateVoxelTUV

///////////////////////////////////////////////////////////////////////////////
// computeVoxelRange functions ////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define template_computeVoxelRange(type)                                       \
  /* temporally constant */                                                    \
  inline range1f SSV_computeVoxelRange_##type(                                 \
      const SharedStructuredVolume *uniform self,                              \
      const vec3i &localCoordinates,                                           \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const uint64 index64 =                                                     \
        (uint64)localCoordinates.x +                                           \
        self->dimensions.x *                                                   \
            ((uint64)localCoordinates.y +                                      \
             self->dimensions.y * ((uint64)localCoordinates.z));               \
    return make_box1f(                                                         \
        get_##type(self->attributesData[attributeIndex], index64));            \
  }                                                                            \
  /* temporally unstructured */                                                \
  inline range1f SSV_computeVoxelRange_TUV_##type(                             \
      const SharedStructuredVolume *uniform self,                              \
      const vec3i &localCoordinates,                                           \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const uniform Data1D indices = self->temporallyUnstructuredIndices;        \
                                                                               \
    const uint64 index64 =                                                     \
        (uint64)localCoordinates.x +                                           \
        self->dimensions.x *                                                   \
            ((uint64)localCoordinates.y +                                      \
             self->dimensions.y * ((uint64)localCoordinates.z));               \
                                                                               \
    uint64 voxelOfs, nextOfs;                                                  \
    if (indices.dataType == VKL_UINT) {                                        \
      voxelOfs = get_uint32(indices, index64);                                 \
      nextOfs  = get_uint32(indices, index64 + 1);                             \
    } else {                                                                   \
      voxelOfs = get_uint64(indices, index64);                                 \
      nextOfs  = get_uint64(indices, index64 + 1);                             \
    }                                                                          \
                                                                               \
    range1f range = make_box1f(                                                \
        get_##type(self->attributesData[attributeIndex], voxelOfs));           \
                                                                               \
    for (uint64 i = voxelOfs + 1; i < nextOfs; i++) {                          \
      extend(range, get_##type(self->attributesData[attributeIndex], i));      \
    }                                                                          \
                                                                               \
    return range;                                                              \
  }                                                                            \
  /* temporally structured */                                                  \
  inline range1f SSV_computeVoxelRange_MB_##type(                              \
      const SharedStructuredVolume *uniform self,                              \
      const vec3i &localCoordinates,                                           \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const uint64 index64 =                                                     \
        self->temporallyStructuredNumTimesteps *                               \
        ((uint64)localCoordinates.x +                                          \
         self->dimensions.x *                                                  \
             ((uint64)localCoordinates.y +                                     \
              self->dimensions.y * ((uint64)localCoordinates.z)));             \
                                                                               \
    range1f range =                                                            \
        make_box1f(get_##type(self->attributesData[attributeIndex], index64)); \
                                                                               \
    for (uint64 i = 1; i < self->temporallyStructuredNumTimesteps; i++) {      \
      extend(range,                                                            \
             get_##type(self->attributesData[attributeIndex], index64 + i));   \
    }                                                                          \
                                                                               \
    return range;                                                              \
  }

declare_all_voxel_types(template_computeVoxelRange);

#undef template_computeVoxelRange

///////////////////////////////////////////////////////////////////////////////
// Sampling functions /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#define template_clampedLocalCoordinates(univary)                      \
  inline void clampedLocalCoordinates_##univary(                       \
      const SharedStructuredVolume *uniform self,                      \
      const univary vec3f &objectCoordinates,                          \
      univary vec3f &clampedLocalCoordinates,                          \
      univary bool &inBounds)                                          \
  {                                                                    \
    inBounds = true;                                                   \
                                                                       \
    transformObjectToLocal_##univary##_dispatch(                       \
        self, objectCoordinates, clampedLocalCoordinates);             \
                                                                       \
    if (clampedLocalCoordinates.x < 0.f ||                             \
        clampedLocalCoordinates.x > self->dimensions.x - 1.f ||        \
        clampedLocalCoordinates.y < 0.f ||                             \
        clampedLocalCoordinates.y > self->dimensions.y - 1.f ||        \
        clampedLocalCoordinates.z < 0.f ||                             \
        clampedLocalCoordinates.z > self->dimensions.z - 1.f) {        \
      inBounds = false;                                                \
      return;                                                          \
    }                                                                  \
                                                                       \
    clampedLocalCoordinates = clamp(clampedLocalCoordinates,           \
                                    make_vec3f(0.0f),                  \
                                    self->localCoordinatesUpperBound); \
  }

template_clampedLocalCoordinates(varying);
template_clampedLocalCoordinates(uniform);
#undef template_clampedLocalCoordinates

#define template_sample(univary)                                       \
  inline univary float SharedStructuredVolume_computeSample_##univary( \
      const SharedStructuredVolume *uniform self,                      \
      const univary vec3f &objectCoordinates,                          \
      const uniform VKLFilter filter,                                  \
      const uniform uint32 attributeIndex,                             \
      const univary float &time)                                       \
  {                                                                    \
    univary vec3f clampedLocalCoordinates;                             \
                                                                       \
    /* computing clampedLocalCoordinates directly here rather than     \
     * using the above clampedLocalCoordinates_univary() function      \
     * produces more efficient code; clampedLocalCoordinates_univary() \
     * is still used for multi-attribute sampling functions... */      \
    transformObjectToLocal_##univary##_dispatch(                       \
        self, objectCoordinates, clampedLocalCoordinates);             \
                                                                       \
    if (clampedLocalCoordinates.x < 0.f ||                             \
        clampedLocalCoordinates.x > self->dimensions.x - 1.f ||        \
        clampedLocalCoordinates.y < 0.f ||                             \
        clampedLocalCoordinates.y > self->dimensions.y - 1.f ||        \
        clampedLocalCoordinates.z < 0.f ||                             \
        clampedLocalCoordinates.z > self->dimensions.z - 1.f) {        \
      return self->super.background[attributeIndex];                   \
    }                                                                  \
                                                                       \
    clampedLocalCoordinates = clamp(clampedLocalCoordinates,           \
                                    make_vec3f(0.0f),                  \
                                    self->localCoordinatesUpperBound); \
                                                                       \
    return self->computeSamplesInner_##univary[attributeIndex](        \
        self, clampedLocalCoordinates, filter, attributeIndex, time);  \
  }

template_sample(varying);
template_sample(uniform);
#undef template_sample

///////////////////////////////////////////////////////////////////////////////
// Sampling inner functions for all addressing / voxel type combinations //////
///////////////////////////////////////////////////////////////////////////////

// perform trilinear interpolation for given sample. unlike old way of doing
// this (a single computesample on the StructuredVolume level that calls the
// virtual 'getSample()' of the volume layout) this function will directly do
// all the addressing for the getSample (inlined), and thus be about 50% faster
// (wall-time, meaning even much faster in pure sample speed)
#define template_sample_inner_32(type, univary)                              \
  inline univary float SSV_sample_inner_##type##_##univary##_32(             \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3f &clampedLocalCoordinates,                          \
      const uniform VKLFilter filter,                                        \
      const uniform uint32 attributeIndex,                                   \
      const univary float &_time)                                            \
  {                                                                          \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];   \
                                                                             \
    /* lower corner of the box straddling the voxels to be interpolated. */  \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);      \
                                                                             \
    const univary uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +     \
                                    voxelIndex_0.y * self->voxelOfs_dy +     \
                                    voxelIndex_0.z * self->voxelOfs_dz;      \
                                                                             \
    univary float val = 0.f;                                                 \
    switch (filter) {                                                        \
    case VKL_FILTER_NEAREST: {                                               \
      val = get_##type(voxelData, voxelOfs);                                 \
      break;                                                                 \
    }                                                                        \
    case VKL_FILTER_LINEAR: {                                                \
      /* fractional coordinates within the lower corner voxel used during    \
       * interpolation. */                                                   \
      const univary vec3f frac =                                             \
          clampedLocalCoordinates - to_float(voxelIndex_0);                  \
                                                                             \
      const uniform uint64 ofs000 = 0;                                       \
      const uniform uint64 ofs001 = self->voxelOfs_dx;                       \
      const univary float val000  = get_##type(voxelData, ofs000, voxelOfs); \
      const univary float val001  = get_##type(voxelData, ofs001, voxelOfs); \
      const univary float val00   = val000 + frac.x * (val001 - val000);     \
                                                                             \
      const uniform uint64 ofs010 = self->voxelOfs_dy;                       \
      const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;   \
      const univary float val010  = get_##type(voxelData, ofs010, voxelOfs); \
      const univary float val011  = get_##type(voxelData, ofs011, voxelOfs); \
      const univary float val01   = val010 + frac.x * (val011 - val010);     \
                                                                             \
      const uniform uint64 ofs100 = self->voxelOfs_dz;                       \
      const uniform uint64 ofs101 = ofs100 + ofs001;                         \
      const univary float val100  = get_##type(voxelData, ofs100, voxelOfs); \
      const univary float val101  = get_##type(voxelData, ofs101, voxelOfs); \
      const univary float val10   = val100 + frac.x * (val101 - val100);     \
                                                                             \
      const uniform uint64 ofs110 = ofs100 + ofs010;                         \
      const uniform uint64 ofs111 = ofs100 + ofs011;                         \
      const univary float val110  = get_##type(voxelData, ofs110, voxelOfs); \
      const univary float val111  = get_##type(voxelData, ofs111, voxelOfs); \
      const univary float val11   = val110 + frac.x * (val111 - val110);     \
                                                                             \
      const univary float val0 = val00 + frac.y * (val01 - val00);           \
      const univary float val1 = val10 + frac.y * (val11 - val10);           \
      val                      = val0 + frac.z * (val1 - val0);              \
      break;                                                                 \
    }                                                                        \
    }                                                                        \
                                                                             \
    return val;                                                              \
  }

declare_all_voxel_types_univary(template_sample_inner_32);

#undef template_sample_inner_32

#define template_sample_inner_TUV_32(type, univary)                         \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_32(        \
      const SharedStructuredVolume *uniform self,                           \
      const univary vec3f &clampedLocalCoordinates,                         \
      const uniform VKLFilter filter,                                       \
      const uniform uint32 attributeIndex,                                  \
      const univary float &time)                                            \
  {                                                                         \
    /* lower corner of the box straddling the voxels to be interpolated. */ \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);     \
                                                                            \
    const univary uint32 timeOfs = voxelIndex_0.x * self->voxelOfs_dx +     \
                                   voxelIndex_0.y * self->voxelOfs_dy +     \
                                   voxelIndex_0.z * self->voxelOfs_dz;      \
                                                                            \
    univary float val = 0.f;                                                \
    switch (filter) {                                                       \
    case VKL_FILTER_NEAREST: {                                              \
      val = interpolateVoxelTUV_##type##_##univary(                         \
          self, attributeIndex, 0, timeOfs, time);                          \
      break;                                                                \
    }                                                                       \
    case VKL_FILTER_LINEAR: {                                               \
      /* fractional coordinates within the lower corner voxel used during   \
       * interpolation. */                                                  \
      const univary vec3f frac =                                            \
          clampedLocalCoordinates - to_float(voxelIndex_0);                 \
      const uniform uint64 ofs000 = 0;                                      \
      const uniform uint64 ofs001 = self->voxelOfs_dx;                      \
      const univary float val000  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs000, timeOfs, time);                     \
      const univary float val001 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs001, timeOfs, time);                     \
                                                                            \
      const univary float val00 = val000 + frac.x * (val001 - val000);      \
                                                                            \
      const uniform uint64 ofs010 = self->voxelOfs_dy;                      \
      const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;  \
      const univary float val010  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs010, timeOfs, time);                     \
      const univary float val011 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs011, timeOfs, time);                     \
                                                                            \
      const univary float val01 = val010 + frac.x * (val011 - val010);      \
                                                                            \
      const uniform uint64 ofs100 = self->voxelOfs_dz;                      \
      const uniform uint64 ofs101 = ofs100 + ofs001;                        \
      const univary float val100  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs100, timeOfs, time);                     \
      const univary float val101 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs101, timeOfs, time);                     \
                                                                            \
      const univary float val10 = val100 + frac.x * (val101 - val100);      \
                                                                            \
      const uniform uint64 ofs110 = ofs100 + ofs010;                        \
      const uniform uint64 ofs111 = ofs100 + ofs011;                        \
      const univary float val110  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs110, timeOfs, time);                     \
      const univary float val111 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs111, timeOfs, time);                     \
                                                                            \
      const univary float val11 = val110 + frac.x * (val111 - val110);      \
                                                                            \
      const univary float val0 = val00 + frac.y * (val01 - val00);          \
      const univary float val1 = val10 + frac.y * (val11 - val10);          \
      val                      = val0 + frac.z * (val1 - val0);             \
      break;                                                                \
    }                                                                       \
    }                                                                       \
                                                                            \
    return val;                                                             \
  }

declare_all_voxel_types_univary(template_sample_inner_TUV_32);

#undef template_sample_inner_TUV_32

#define template_sample_inner_MB_32(type, univary)                            \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_32(           \
      const SharedStructuredVolume *uniform self,                             \
      const univary vec3f &clampedLocalCoordinates,                           \
      const uniform VKLFilter filter,                                         \
      const uniform uint32 attributeIndex,                                    \
      const univary float &time)                                              \
  {                                                                           \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];    \
    const uniform uint32 numTimeSamples =                                     \
        self->temporallyStructuredNumTimesteps;                               \
                                                                              \
    /* lower corner of the box straddling the voxels to be interpolated. */   \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);       \
                                                                              \
    const uniform uint64 voxelOfs_dx = numTimeSamples * self->voxelOfs_dx;    \
    const uniform uint64 voxelOfs_dy = numTimeSamples * self->voxelOfs_dy;    \
    const uniform uint64 voxelOfs_dz = numTimeSamples * self->voxelOfs_dz;    \
    const univary uint32 voxelOfs_0  = voxelIndex_0.x * voxelOfs_dx +         \
                                      voxelIndex_0.y * voxelOfs_dy +          \
                                      voxelIndex_0.z * voxelOfs_dz;           \
                                                                              \
    const univary uint32 localVoxelOfs = time * (numTimeSamples - 1);         \
    const univary float interpolant =                                         \
        time * (float)(numTimeSamples - 1) - (float)localVoxelOfs;            \
                                                                              \
    const univary uint32 voxelOfs = voxelOfs_0 + localVoxelOfs;               \
                                                                              \
    /* if interpolant is zero, we don't need to access the next time entry.   \
     * this also prevents illegal accesses for the time == 1.f case at the    \
     * volume boundary. */                                                    \
    const univary uint32 nextOfs =                                            \
        interpolant == 0.f ? voxelOfs : voxelOfs + 1;                         \
                                                                              \
    univary float val = 0.f;                                                  \
    switch (filter) {                                                         \
    case VKL_FILTER_NEAREST: {                                                \
      val = (1.f - interpolant) * get_##type(voxelData, voxelOfs) +           \
            interpolant * get_##type(voxelData, nextOfs);                     \
      break;                                                                  \
    }                                                                         \
    case VKL_FILTER_LINEAR: {                                                 \
      /* fractional coordinates within the lower corner voxel used during     \
       * interpolation. */                                                    \
      const univary vec3f frac =                                              \
          clampedLocalCoordinates - to_float(voxelIndex_0);                   \
                                                                              \
      const uniform uint64 ofs000 = 0;                                        \
      const uniform uint64 ofs001 = voxelOfs_dx;                              \
      const univary float val000 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs000, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs000, nextOfs);               \
      const univary float val001 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs001, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs001, nextOfs);               \
      const univary float val00 = val000 + frac.x * (val001 - val000);        \
                                                                              \
      const uniform uint64 ofs010 = voxelOfs_dy;                              \
      const uniform uint64 ofs011 = voxelOfs_dy + voxelOfs_dx;                \
      const univary float val010 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs010, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs010, nextOfs);               \
      const univary float val011 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs011, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs011, nextOfs);               \
      const univary float val01 = val010 + frac.x * (val011 - val010);        \
                                                                              \
      const uniform uint64 ofs100 = voxelOfs_dz;                              \
      const uniform uint64 ofs101 = ofs100 + ofs001;                          \
      const univary float val100 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs100, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs100, nextOfs);               \
      const univary float val101 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs101, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs101, nextOfs);               \
      const univary float val10 = val100 + frac.x * (val101 - val100);        \
                                                                              \
      const uniform uint64 ofs110 = ofs100 + ofs010;                          \
      const uniform uint64 ofs111 = ofs100 + ofs011;                          \
      const univary float val110 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs110, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs110, nextOfs);               \
      const univary float val111 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs111, voxelOfs) +     \
          interpolant * get_##type(voxelData, ofs111, nextOfs);               \
      const univary float val11 = val110 + frac.x * (val111 - val110);        \
                                                                              \
      const univary float val0 = val00 + frac.y * (val01 - val00);            \
      const univary float val1 = val10 + frac.y * (val11 - val10);            \
      val                      = val0 + frac.z * (val1 - val0);               \
      break;                                                                  \
    }                                                                         \
    }                                                                         \
                                                                              \
    return val;                                                               \
  }

declare_all_voxel_types_univary(template_sample_inner_MB_32);

#undef template_sample_inner_MB_32

// used below in template_sample_inner_64_32
#define process_sliceID(univary) process_sliceID_##univary
#define process_sliceID_varying foreach_unique(sliceID in voxelIndex_0.z)
#define process_sliceID_uniform uniform int sliceID = voxelIndex_0.z;

#define template_sample_inner_64_32(type, univary)                           \
  inline univary float SSV_sample_inner_##type##_##univary##_64_32(          \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3f &clampedLocalCoordinates,                          \
      const uniform VKLFilter filter,                                        \
      const uniform uint32 attributeIndex,                                   \
      const univary float &_time)                                            \
  {                                                                          \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];   \
                                                                             \
    /* lower corner of the box straddling the voxels to be interpolated. */  \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);      \
                                                                             \
    univary float ret = 0.f;                                                 \
    process_sliceID(univary)                                                 \
    {                                                                        \
      const uniform uint64 sliceOfs =                                        \
          (uint64)sliceID * self->dimensions.x * self->dimensions.y;         \
      const univary uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +   \
                                      voxelIndex_0.y * self->voxelOfs_dy;    \
                                                                             \
      switch (filter) {                                                      \
      case VKL_FILTER_NEAREST: {                                             \
        ret = get_##type(voxelData, sliceOfs, voxelOfs);                     \
        break;                                                               \
      }                                                                      \
      case VKL_FILTER_LINEAR: {                                              \
        /* fractional coordinates within the lower corner voxel used during  \
         * interpolation. */                                                 \
        const univary vec3f frac =                                           \
            clampedLocalCoordinates - to_float(voxelIndex_0);                \
                                                                             \
        const uniform uint64 ofs000 = 0;                                     \
        const uniform uint64 ofs001 = self->voxelOfs_dx;                     \
        const univary float val000 =                                         \
            get_##type(voxelData, sliceOfs + ofs000, voxelOfs);              \
        const univary float val001 =                                         \
            get_##type(voxelData, sliceOfs + ofs001, voxelOfs);              \
        const univary float val00 = val000 + frac.x * (val001 - val000);     \
                                                                             \
        const uniform uint64 ofs010 = self->voxelOfs_dy;                     \
        const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx; \
        const univary float val010 =                                         \
            get_##type(voxelData, sliceOfs + ofs010, voxelOfs);              \
        const univary float val011 =                                         \
            get_##type(voxelData, sliceOfs + ofs011, voxelOfs);              \
        const univary float val01 = val010 + frac.x * (val011 - val010);     \
                                                                             \
        const uniform uint64 ofs100 = self->voxelOfs_dz;                     \
        const uniform uint64 ofs101 = ofs100 + ofs001;                       \
        const univary float val100 =                                         \
            get_##type(voxelData, sliceOfs + ofs100, voxelOfs);              \
        const univary float val101 =                                         \
            get_##type(voxelData, sliceOfs + ofs101, voxelOfs);              \
        const univary float val10 = val100 + frac.x * (val101 - val100);     \
                                                                             \
        const uniform uint64 ofs110 = ofs100 + ofs010;                       \
        const uniform uint64 ofs111 = ofs100 + ofs011;                       \
        const univary float val110 =                                         \
            get_##type(voxelData, sliceOfs + ofs110, voxelOfs);              \
        const univary float val111 =                                         \
            get_##type(voxelData, sliceOfs + ofs111, voxelOfs);              \
        const univary float val11 = val110 + frac.x * (val111 - val110);     \
                                                                             \
        const univary float val0 = val00 + frac.y * (val01 - val00);         \
        const univary float val1 = val10 + frac.y * (val11 - val10);         \
        const univary float val  = val0 + frac.z * (val1 - val0);            \
        ret                      = val;                                      \
        break;                                                               \
      }                                                                      \
      }                                                                      \
    }                                                                        \
    return ret;                                                              \
  }

declare_all_voxel_types_univary(template_sample_inner_64_32);

#undef template_sample_inner_64_32

#define template_sample_inner_TUV_64_32(type, univary)                        \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_64_32(       \
      const SharedStructuredVolume *uniform self,                             \
      const univary vec3f &clampedLocalCoordinates,                           \
      const uniform VKLFilter filter,                                         \
      const uniform uint32 attributeIndex,                                    \
      const univary float &time)                                              \
  {                                                                           \
    /* lower corner of the box straddling the voxels to be interpolated. */   \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);       \
                                                                              \
    univary float val = 0.f;                                                  \
    process_sliceID(univary)                                                  \
    {                                                                         \
      const uniform uint64 sliceOfs =                                         \
          (uint64)sliceID * self->dimensions.x * self->dimensions.y;          \
      const univary uint32 timeOfs = voxelIndex_0.x * self->voxelOfs_dx +     \
                                     voxelIndex_0.y * self->voxelOfs_dy;      \
                                                                              \
      switch (filter) {                                                       \
      case VKL_FILTER_NEAREST: {                                              \
        val = interpolateVoxelTUV_##type##_##univary(                         \
            self, attributeIndex, sliceOfs, timeOfs, time);                   \
        break;                                                                \
      }                                                                       \
      case VKL_FILTER_LINEAR: {                                               \
        /* fractional coordinates within the lower corner voxel used during   \
         * interpolation. */                                                  \
        const univary vec3f frac =                                            \
            clampedLocalCoordinates - to_float(voxelIndex_0);                 \
                                                                              \
        const uniform uint64 ofs000 = 0;                                      \
        const uniform uint64 ofs001 = self->voxelOfs_dx;                      \
        const univary float val000  = interpolateVoxelTUV_##type##_##univary( \
            self, attributeIndex, sliceOfs + ofs000, timeOfs, time);          \
        const univary float val001 = interpolateVoxelTUV_##type##_##univary(  \
            self, attributeIndex, sliceOfs + ofs001, timeOfs, time);          \
                                                                              \
        const univary float val00 = val000 + frac.x * (val001 - val000);      \
                                                                              \
        const uniform uint64 ofs010 = self->voxelOfs_dy;                      \
        const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;  \
        const univary float val010  = interpolateVoxelTUV_##type##_##univary( \
            self, attributeIndex, sliceOfs + ofs010, timeOfs, time);          \
        const univary float val011 = interpolateVoxelTUV_##type##_##univary(  \
            self, attributeIndex, sliceOfs + ofs011, timeOfs, time);          \
                                                                              \
        const univary float val01 = val010 + frac.x * (val011 - val010);      \
                                                                              \
        const uniform uint64 ofs100 = self->voxelOfs_dz;                      \
        const uniform uint64 ofs101 = ofs100 + ofs001;                        \
        const univary float val100  = interpolateVoxelTUV_##type##_##univary( \
            self, attributeIndex, sliceOfs + ofs100, timeOfs, time);          \
        const univary float val101 = interpolateVoxelTUV_##type##_##univary(  \
            self, attributeIndex, sliceOfs + ofs101, timeOfs, time);          \
                                                                              \
        const univary float val10 = val100 + frac.x * (val101 - val100);      \
                                                                              \
        const uniform uint64 ofs110 = ofs100 + ofs010;                        \
        const uniform uint64 ofs111 = ofs100 + ofs011;                        \
        const univary float val110  = interpolateVoxelTUV_##type##_##univary( \
            self, attributeIndex, sliceOfs + ofs110, timeOfs, time);          \
        const univary float val111 = interpolateVoxelTUV_##type##_##univary(  \
            self, attributeIndex, sliceOfs + ofs111, timeOfs, time);          \
                                                                              \
        const univary float val11 = val110 + frac.x * (val111 - val110);      \
                                                                              \
        const univary float val0 = val00 + frac.y * (val01 - val00);          \
        const univary float val1 = val10 + frac.y * (val11 - val10);          \
        val                      = val0 + frac.z * (val1 - val0);             \
        break;                                                                \
      }                                                                       \
      }                                                                       \
    }                                                                         \
                                                                              \
    return val;                                                               \
  }

declare_all_voxel_types_univary(template_sample_inner_TUV_64_32);

#undef template_sample_inner_TUV_64_32

#define template_sample_inner_MB_64_32(type, univary)                         \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_64_32(        \
      const SharedStructuredVolume *uniform self,                             \
      const univary vec3f &clampedLocalCoordinates,                           \
      const uniform VKLFilter filter,                                         \
      const uniform uint32 attributeIndex,                                    \
      const univary float &time)                                              \
  {                                                                           \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];    \
    const uniform uint32 numTimeSamples =                                     \
        self->temporallyStructuredNumTimesteps;                               \
                                                                              \
    /* lower corner of the box straddling the voxels to be interpolated. */   \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);       \
                                                                              \
    const uniform uint64 voxelOfs_dx = numTimeSamples * self->voxelOfs_dx;    \
    const uniform uint64 voxelOfs_dy = numTimeSamples * self->voxelOfs_dy;    \
    const uniform uint64 voxelOfs_dz = numTimeSamples * self->voxelOfs_dz;    \
                                                                              \
    univary float val = 0.f;                                                  \
    process_sliceID(univary)                                                  \
    {                                                                         \
      const uniform uint64 sliceOfs = (uint64)sliceID * voxelOfs_dz;          \
                                                                              \
      const univary uint32 voxelOfs_0 =                                       \
          voxelIndex_0.x * voxelOfs_dx + voxelIndex_0.y * voxelOfs_dy;        \
                                                                              \
      const univary uint32 localVoxelOfs = time * (numTimeSamples - 1);       \
      const univary float interpolant =                                       \
          time * (float)(numTimeSamples - 1) - (float)localVoxelOfs;          \
                                                                              \
      const univary uint32 voxelOfs = voxelOfs_0 + localVoxelOfs;             \
                                                                              \
      /* if interpolant is zero, we don't need to access the next time entry. \
       * this also prevents illegal accesses for the time == 1.f case at the  \
       * volume boundary. */                                                  \
      const univary uint32 nextOfs =                                          \
          interpolant == 0.f ? voxelOfs : voxelOfs + 1;                       \
                                                                              \
      switch (filter) {                                                       \
      case VKL_FILTER_NEAREST: {                                              \
        val =                                                                 \
            (1.f - interpolant) * get_##type(voxelData, sliceOfs, voxelOfs) + \
            interpolant * get_##type(voxelData, sliceOfs, nextOfs);           \
        break;                                                                \
      }                                                                       \
      case VKL_FILTER_LINEAR: {                                               \
        /* fractional coordinates within the lower corner voxel used during   \
         * interpolation. */                                                  \
        const univary vec3f frac =                                            \
            clampedLocalCoordinates - to_float(voxelIndex_0);                 \
                                                                              \
        const uniform uint64 ofs000 = 0;                                      \
        const uniform uint64 ofs001 = voxelOfs_dx;                            \
        const univary float val000 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs000, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs000, nextOfs);  \
        const univary float val001 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs001, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs001, nextOfs);  \
        const univary float val00 = val000 + frac.x * (val001 - val000);      \
                                                                              \
        const uniform uint64 ofs010 = voxelOfs_dy;                            \
        const uniform uint64 ofs011 = voxelOfs_dy + voxelOfs_dx;              \
        const univary float val010 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs010, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs010, nextOfs);  \
        const univary float val011 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs011, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs011, nextOfs);  \
        const univary float val01 = val010 + frac.x * (val011 - val010);      \
                                                                              \
        const uniform uint64 ofs100 = voxelOfs_dz;                            \
        const uniform uint64 ofs101 = ofs100 + ofs001;                        \
        const univary float val100 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs100, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs100, nextOfs);  \
        const univary float val101 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs101, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs101, nextOfs);  \
        const univary float val10 = val100 + frac.x * (val101 - val100);      \
                                                                              \
        const uniform uint64 ofs110 = ofs100 + ofs010;                        \
        const uniform uint64 ofs111 = ofs100 + ofs011;                        \
        const univary float val110 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs110, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs110, nextOfs);  \
        const univary float val111 =                                          \
            (1.f - interpolant) *                                             \
                get_##type(voxelData, sliceOfs + ofs111, voxelOfs) +          \
            interpolant * get_##type(voxelData, sliceOfs + ofs111, nextOfs);  \
        const univary float val11 = val110 + frac.x * (val111 - val110);      \
                                                                              \
        const univary float val0 = val00 + frac.y * (val01 - val00);          \
        const univary float val1 = val10 + frac.y * (val11 - val10);          \
        val                      = val0 + frac.z * (val1 - val0);             \
        break;                                                                \
      }                                                                       \
      }                                                                       \
    }                                                                         \
                                                                              \
    return val;                                                               \
  }

declare_all_voxel_types_univary(template_sample_inner_MB_64_32);

#undef template_sample_inner_MB_64_32

// default sampling function (64-bit addressing)
#define template_sample_inner_64(type, univary)                                 \
  inline univary float SSV_sample_inner_##type##_##univary##_64(                \
      const SharedStructuredVolume *uniform self,                               \
      const univary vec3f &clampedLocalCoordinates,                             \
      const uniform VKLFilter filter,                                           \
      const uniform uint32 attributeIndex,                                      \
      const univary float &_time)                                               \
  {                                                                             \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];      \
                                                                                \
    /* lower and upper corners of the box straddling the voxels to be           \
     interpolated. */                                                           \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);         \
                                                                                \
    const uniform uint64 voxelOfs_dx = 1;                                       \
    const uniform uint64 voxelOfs_dy = self->dimensions.x;                      \
    const uniform uint64 voxelOfs_dz =                                          \
        self->dimensions.x * (uint64)self->dimensions.y;                        \
    const univary uint64 voxelOfs_0 = voxelIndex_0.x * voxelOfs_dx +            \
                                      voxelIndex_0.y * voxelOfs_dy +            \
                                      voxelIndex_0.z * voxelOfs_dz;             \
                                                                                \
    univary float val = 0.f;                                                    \
    switch (filter) {                                                           \
    case VKL_FILTER_NEAREST: {                                                  \
      val = get_##type(voxelData, voxelOfs_0);                                  \
      break;                                                                    \
    }                                                                           \
    case VKL_FILTER_LINEAR: {                                                   \
      /* fractional coordinates within the lower corner voxel used during       \
       interpolation. */                                                        \
      const univary vec3f frac =                                                \
          clampedLocalCoordinates - to_float(voxelIndex_0);                     \
                                                                                \
      const uniform uint64 ofs000 = 0;                                          \
      const uniform uint64 ofs001 = voxelOfs_dx;                                \
      const univary float val000  = get_##type(voxelData, ofs000 + voxelOfs_0); \
      const univary float val001  = get_##type(voxelData, ofs001 + voxelOfs_0); \
      const univary float val00   = val000 + frac.x * (val001 - val000);        \
                                                                                \
      const uniform uint64 ofs010 = voxelOfs_dy;                                \
      const uniform uint64 ofs011 = voxelOfs_dy + voxelOfs_dx;                  \
      const univary float val010  = get_##type(voxelData, ofs010 + voxelOfs_0); \
      const univary float val011  = get_##type(voxelData, ofs011 + voxelOfs_0); \
      const univary float val01   = val010 + frac.x * (val011 - val010);        \
                                                                                \
      const uniform uint64 ofs100 = voxelOfs_dz;                                \
      const uniform uint64 ofs101 = ofs100 + ofs001;                            \
      const univary float val100  = get_##type(voxelData, ofs100 + voxelOfs_0); \
      const univary float val101  = get_##type(voxelData, ofs101 + voxelOfs_0); \
      const univary float val10   = val100 + frac.x * (val101 - val100);        \
                                                                                \
      const uniform uint64 ofs110 = ofs100 + ofs010;                            \
      const uniform uint64 ofs111 = ofs100 + ofs011;                            \
      const univary float val110  = get_##type(voxelData, ofs110 + voxelOfs_0); \
      const univary float val111  = get_##type(voxelData, ofs111 + voxelOfs_0); \
      const univary float val11   = val110 + frac.x * (val111 - val110);        \
                                                                                \
      const univary float val0 = val00 + frac.y * (val01 - val00);              \
      const univary float val1 = val10 + frac.y * (val11 - val10);              \
      val                      = val0 + frac.z * (val1 - val0);                 \
      break;                                                                    \
    }                                                                           \
    }                                                                           \
    return val;                                                                 \
  }

declare_all_voxel_types_univary(template_sample_inner_64);

#undef template_sample_inner_64

#define template_sample_inner_TUV_64(type, univary)                         \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_64(        \
      const SharedStructuredVolume *uniform self,                           \
      const univary vec3f &clampedLocalCoordinates,                         \
      const uniform VKLFilter filter,                                       \
      const uniform uint32 attributeIndex,                                  \
      const univary float &time)                                            \
  {                                                                         \
    /* lower corner of the box straddling the voxels to be interpolated. */ \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);     \
                                                                            \
    const uniform uint64 voxelOfs_dx = 1;                                   \
    const uniform uint64 voxelOfs_dy = self->dimensions.x;                  \
    const uniform uint64 voxelOfs_dz =                                      \
        self->dimensions.x * (uint64)self->dimensions.y;                    \
                                                                            \
    const univary uint64 timeOfs = (uint64)voxelIndex_0.x * voxelOfs_dx +   \
                                   (uint64)voxelIndex_0.y * voxelOfs_dy +   \
                                   (uint64)voxelIndex_0.z * voxelOfs_dz;    \
                                                                            \
    univary float val = 0.f;                                                \
    switch (filter) {                                                       \
    case VKL_FILTER_NEAREST: {                                              \
      val = interpolateVoxelTUV_##type##_##univary(                         \
          self, attributeIndex, timeOfs, time);                             \
      break;                                                                \
    }                                                                       \
    case VKL_FILTER_LINEAR: {                                               \
      /* fractional coordinates within the lower corner voxel used during   \
       * interpolation. */                                                  \
      const univary vec3f frac =                                            \
          clampedLocalCoordinates - to_float(voxelIndex_0);                 \
                                                                            \
      const uniform uint64 ofs000 = 0;                                      \
      const uniform uint64 ofs001 = voxelOfs_dx;                            \
      const univary float val000  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs000 + timeOfs, time);                    \
      const univary float val001 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs001 + timeOfs, time);                    \
                                                                            \
      const univary float val00 = val000 + frac.x * (val001 - val000);      \
                                                                            \
      const uniform uint64 ofs010 = voxelOfs_dy;                            \
      const uniform uint64 ofs011 = voxelOfs_dy + voxelOfs_dx;              \
      const univary float val010  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs010 + timeOfs, time);                    \
      const univary float val011 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs011 + timeOfs, time);                    \
                                                                            \
      const univary float val01 = val010 + frac.x * (val011 - val010);      \
                                                                            \
      const uniform uint64 ofs100 = voxelOfs_dz;                            \
      const uniform uint64 ofs101 = ofs100 + ofs001;                        \
      const univary float val100  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs100 + timeOfs, time);                    \
      const univary float val101 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs101 + timeOfs, time);                    \
                                                                            \
      const univary float val10 = val100 + frac.x * (val101 - val100);      \
                                                                            \
      const uniform uint64 ofs110 = ofs100 + ofs010;                        \
      const uniform uint64 ofs111 = ofs100 + ofs011;                        \
      const univary float val110  = interpolateVoxelTUV_##type##_##univary( \
          self, attributeIndex, ofs110 + timeOfs, time);                    \
      const univary float val111 = interpolateVoxelTUV_##type##_##univary(  \
          self, attributeIndex, ofs111 + timeOfs, time);                    \
                                                                            \
      const univary float val11 = val110 + frac.x * (val111 - val110);      \
                                                                            \
      const univary float val0 = val00 + frac.y * (val01 - val00);          \
      const univary float val1 = val10 + frac.y * (val11 - val10);          \
      val                      = val0 + frac.z * (val1 - val0);             \
      break;                                                                \
    }                                                                       \
    }                                                                       \
                                                                            \
    return val;                                                             \
  }

declare_all_voxel_types_univary(template_sample_inner_TUV_64);

#undef template_sample_inner_TUV_64

#define template_sample_inner_MB_64(type, univary)                            \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_64(           \
      const SharedStructuredVolume *uniform self,                             \
      const univary vec3f &clampedLocalCoordinates,                           \
      const uniform VKLFilter filter,                                         \
      const uniform uint32 attributeIndex,                                    \
      const univary float &time)                                              \
  {                                                                           \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];    \
    const uniform uint32 numTimeSamples =                                     \
        self->temporallyStructuredNumTimesteps;                               \
                                                                              \
    /* lower and upper corners of the box straddling the voxels to be         \
     interpolated. */                                                         \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);       \
                                                                              \
    const uniform uint64 voxelOfs_dx = numTimeSamples;                        \
    const uniform uint64 voxelOfs_dy = numTimeSamples * self->dimensions.x;   \
    const uniform uint64 voxelOfs_dz =                                        \
        numTimeSamples * self->dimensions.x * (uint64)self->dimensions.y;     \
    const univary uint64 voxelOfs_0 = voxelIndex_0.x * voxelOfs_dx +          \
                                      voxelIndex_0.y * voxelOfs_dy +          \
                                      voxelIndex_0.z * voxelOfs_dz;           \
                                                                              \
    const univary uint32 localVoxelOfs = time * (numTimeSamples - 1);         \
    const univary float interpolant =                                         \
        time * (float)(numTimeSamples - 1) - (float)localVoxelOfs;            \
                                                                              \
    const univary uint64 voxelOfs = voxelOfs_0 + localVoxelOfs;               \
                                                                              \
    /* if interpolant is zero, we don't need to access the next time entry.   \
     * this also prevents illegal accesses for the time == 1.f case at the    \
     * volume boundary. */                                                    \
    const univary uint64 nextOfs =                                            \
        interpolant == 0.f ? voxelOfs : voxelOfs + 1;                         \
                                                                              \
    univary float val = 0.f;                                                  \
    switch (filter) {                                                         \
    case VKL_FILTER_NEAREST: {                                                \
      val = (1.f - interpolant) * get_##type(voxelData, voxelOfs) +           \
            interpolant * get_##type(voxelData, nextOfs);                     \
      break;                                                                  \
    }                                                                         \
    case VKL_FILTER_LINEAR: {                                                 \
      /* fractional coordinates within the lower corner voxel used during     \
       * interpolation. */                                                    \
      const univary vec3f frac =                                              \
          clampedLocalCoordinates - to_float(voxelIndex_0);                   \
                                                                              \
      const uniform uint64 ofs000 = 0;                                        \
      const uniform uint64 ofs001 = voxelOfs_dx;                              \
      const univary float val000 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs000 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs000 + nextOfs);              \
      const univary float val001 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs001 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs001 + nextOfs);              \
      const univary float val00 = val000 + frac.x * (val001 - val000);        \
                                                                              \
      const uniform uint64 ofs010 = voxelOfs_dy;                              \
      const uniform uint64 ofs011 = voxelOfs_dy + voxelOfs_dx;                \
      const univary float val010 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs010 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs010 + nextOfs);              \
      const univary float val011 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs011 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs011 + nextOfs);              \
      const univary float val01 = val010 + frac.x * (val011 - val010);        \
                                                                              \
      const uniform uint64 ofs100 = voxelOfs_dz;                              \
      const uniform uint64 ofs101 = ofs100 + ofs001;                          \
      const univary float val100 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs100 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs100 + nextOfs);              \
      const univary float val101 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs101 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs101 + nextOfs);              \
      const univary float val10 = val100 + frac.x * (val101 - val100);        \
                                                                              \
      const uniform uint64 ofs110 = ofs100 + ofs010;                          \
      const uniform uint64 ofs111 = ofs100 + ofs011;                          \
      const univary float val110 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs110 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs110 + nextOfs);              \
      const univary float val111 =                                            \
          (1.f - interpolant) * get_##type(voxelData, ofs111 + voxelOfs) +    \
          interpolant * get_##type(voxelData, ofs111 + nextOfs);              \
      const univary float val11 = val110 + frac.x * (val111 - val110);        \
                                                                              \
      const univary float val0 = val00 + frac.y * (val01 - val00);            \
      const univary float val1 = val10 + frac.y * (val11 - val10);            \
      val                      = val0 + frac.z * (val1 - val0);               \
      break;                                                                  \
    }                                                                         \
    }                                                                         \
    return val;                                                               \
  }

declare_all_voxel_types_univary(template_sample_inner_MB_64);

#undef template_sample_inner_MB_64

///////////////////////////////////////////////////////////////////////////////
// Gradient computation ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline varying vec3f SharedStructuredVolume_computeGradient_bbox_checks(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex,
    const varying float &time)
{
  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gridSpacing;

  // compute via forward or backward differences depending on volume boundary
  const vec3f gradientExtent = objectCoordinates + gradientStep;

  if (gradientExtent.x >= self->boundingBox.upper.x)
    gradientStep.x *= -1.f;

  if (gradientExtent.y >= self->boundingBox.upper.y)
    gradientStep.y *= -1.f;

  if (gradientExtent.z >= self->boundingBox.upper.z)
    gradientStep.z *= -1.f;

  vec3f gradient;

  float sample = SharedStructuredVolume_computeSample_varying(
      self, objectCoordinates, filter, attributeIndex, time);

  gradient.x = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   filter,
                   attributeIndex,
                   time) -
               sample;
  gradient.y = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   filter,
                   attributeIndex,
                   time) -
               sample;
  gradient.z = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   filter,
                   attributeIndex,
                   time) -
               sample;

  return gradient / gradientStep;
}

inline varying vec3f SharedStructuredVolume_computeGradient_NaN_checks(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex,
    const varying float &time)
{
  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gridSpacing;

  // compute via forward or backward differences depending on volume boundary
  // (as determined by NaN sample values outside the boundary)
  const vec3f gradientExtent = objectCoordinates + gradientStep;

  vec3f gradient;

  float sample = SharedStructuredVolume_computeSample_varying(
      self, objectCoordinates, filter, attributeIndex, time);

  gradient.x = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   filter,
                   attributeIndex,
                   time) -
               sample;
  gradient.y = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   filter,
                   attributeIndex,
                   time) -
               sample;
  gradient.z = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   VKL_FILTER_LINEAR,
                   attributeIndex,
                   time) -
               sample;

  if (isnan(gradient.x)) {
    gradientStep.x *= -1.f;

    gradient.x = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                     filter,
                     attributeIndex,
                     time) -
                 sample;
  }

  if (isnan(gradient.y)) {
    gradientStep.y *= -1.f;

    gradient.y = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                     filter,
                     attributeIndex,
                     time) -
                 sample;
  }

  if (isnan(gradient.z)) {
    gradientStep.z *= -1.f;

    gradient.z = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                     filter,
                     attributeIndex,
                     time) -
                 sample;
  }

  return gradient / gradientStep;
}

///////////////////////////////////////////////////////////////////////////////
// wrappers to enable setting old-style function pointers in the Volume
// superclass; restricts to first attribute
///////////////////////////////////////////////////////////////////////////////
varying float computeSample_varying_0_iterator_wrapper(
    const SamplerShared *uniform _sampler,
    const varying vec3f &objectCoordinates,
    const uniform uint32 attributeIndex,
    const varying float &time)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  assert(sampler);

  return SharedStructuredVolume_computeSample_varying(
      (const SharedStructuredVolume *uniform)sampler->volume,
      objectCoordinates,
      sampler->filter,
      attributeIndex,
      time);
}

uniform float computeSample_uniform_0_iterator_wrapper(
    const SamplerShared *uniform _sampler,
    const uniform vec3f &objectCoordinates,
    const uniform uint32 attributeIndex,
    const uniform float &time)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  assert(sampler);

  return SharedStructuredVolume_computeSample_uniform(
      (const SharedStructuredVolume *uniform)sampler->volume,
      objectCoordinates,
      sampler->filter,
      attributeIndex,
      time);
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions for assigning sampling functions //////////////////////////
///////////////////////////////////////////////////////////////////////////////

export uniform bool EXPORT_UNIQUE(assignTemporallyConstantSamplingFunctions,
    void *uniform _self, const uniform uint32 attributeIndex)
{
  SharedStructuredVolume *uniform self = (SharedStructuredVolume *uniform) _self;
  assert(!(hasStructuredTimeData(self) || hasUnstructuredTimeData(self)));

  ComputeVoxelRangeFunc cvr            = NULL;
  ComputeSampleInnerVaryingFunc innerV = NULL;
  ComputeSampleInnerUniformFunc innerU = NULL;

  // default sampling functions (64-bit addressing)
  switch (self->attributesData[attributeIndex].dataType) {
  case (VKL_UCHAR): {
    cvr    = SSV_computeVoxelRange_uint8;
    innerV = SSV_sample_inner_uint8_varying_64;
    innerU = SSV_sample_inner_uint8_uniform_64;
    break;
  }
  case (VKL_SHORT): {
    cvr    = SSV_computeVoxelRange_int16;
    innerV = SSV_sample_inner_int16_varying_64;
    innerU = SSV_sample_inner_int16_uniform_64;
    break;
  }
  case (VKL_USHORT): {
    cvr    = SSV_computeVoxelRange_uint16;
    innerV = SSV_sample_inner_uint16_varying_64;
    innerU = SSV_sample_inner_uint16_uniform_64;
    break;
  }
  case (VKL_HALF): {
    cvr    = SSV_computeVoxelRange_half;
    innerV = SSV_sample_inner_half_varying_64;
    innerU = SSV_sample_inner_half_uniform_64;
    break;
  }
  case (VKL_FLOAT): {
    cvr    = SSV_computeVoxelRange_float;
    innerV = SSV_sample_inner_float_varying_64;
    innerU = SSV_sample_inner_float_uniform_64;
    break;
  }
  case (VKL_DOUBLE): {
    cvr    = SSV_computeVoxelRange_double;
    innerV = SSV_sample_inner_double_varying_64;
    innerU = SSV_sample_inner_double_uniform_64;
    break;
  }
  default: {
    print("#vkl:shared_structured_volume: unknown voxelType\n");
    return false;
  }
  }

  // sampling functions for 32-bit addressing
  if (safe_32bit_indexing(self->attributesData[attributeIndex],
                          self->attributesData[attributeIndex].numItems)) {
    // in this case, we know ALL addressing can be 32-bit.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_uint8_varying_32;
      innerU = SSV_sample_inner_uint8_uniform_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_int16_varying_32;
      innerU = SSV_sample_inner_int16_uniform_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_uint16_varying_32;
      innerU = SSV_sample_inner_uint16_uniform_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_half_varying_32;
      innerU = SSV_sample_inner_half_uniform_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_float_varying_32;
      innerU = SSV_sample_inner_float_uniform_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_double_varying_32;
      innerU = SSV_sample_inner_double_uniform_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  else if (safe_32bit_indexing(
               self->attributesData[attributeIndex],
               self->dimensions.x * (uniform uint64)self->dimensions.y)) {
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice beginning.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_uint8_varying_64_32;
      innerU = SSV_sample_inner_uint8_uniform_64_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_int16_varying_64_32;
      innerU = SSV_sample_inner_int16_uniform_64_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_uint16_varying_64_32;
      innerU = SSV_sample_inner_uint16_uniform_64_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_half_varying_64_32;
      innerU = SSV_sample_inner_half_uniform_64_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_float_varying_64_32;
      innerU = SSV_sample_inner_float_uniform_64_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_double_varying_64_32;
      innerU = SSV_sample_inner_double_uniform_64_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  assert(cvr && innerV && innerU);
  self->computeVoxelRange[attributeIndex]           = cvr;
  self->computeSamplesInner_varying[attributeIndex] = innerV;
  self->computeSamplesInner_uniform[attributeIndex] = innerU;

  return true;
}

export uniform bool EXPORT_UNIQUE(assignTemporallyStructuredSamplingFunctions,
    void *uniform _self, const uniform uint32 attributeIndex)
{
  SharedStructuredVolume *uniform self = (SharedStructuredVolume *uniform) _self;
  assert(hasStructuredTimeData(self));

  ComputeVoxelRangeFunc cvr            = NULL;
  ComputeSampleInnerVaryingFunc innerV = NULL;
  ComputeSampleInnerUniformFunc innerU = NULL;

  const uniform uint32 numTimeSamples = self->temporallyStructuredNumTimesteps;

  // default sampling functions (64-bit addressing)
  switch (self->attributesData[attributeIndex].dataType) {
  case (VKL_UCHAR): {
    cvr    = SSV_computeVoxelRange_MB_uint8;
    innerV = SSV_sample_inner_MB_uint8_varying_64;
    innerU = SSV_sample_inner_MB_uint8_uniform_64;
    break;
  }
  case (VKL_SHORT): {
    cvr    = SSV_computeVoxelRange_MB_int16;
    innerV = SSV_sample_inner_MB_int16_varying_64;
    innerU = SSV_sample_inner_MB_int16_uniform_64;
    break;
  }
  case (VKL_USHORT): {
    cvr    = SSV_computeVoxelRange_MB_uint16;
    innerV = SSV_sample_inner_MB_uint16_varying_64;
    innerU = SSV_sample_inner_MB_uint16_uniform_64;
    break;
  }
  case (VKL_HALF): {
    cvr    = SSV_computeVoxelRange_MB_half;
    innerV = SSV_sample_inner_MB_half_varying_64;
    innerU = SSV_sample_inner_MB_half_uniform_64;
    break;
  }
  case (VKL_FLOAT): {
    cvr    = SSV_computeVoxelRange_MB_float;
    innerV = SSV_sample_inner_MB_float_varying_64;
    innerU = SSV_sample_inner_MB_float_uniform_64;
    break;
  }
  case (VKL_DOUBLE): {
    cvr    = SSV_computeVoxelRange_MB_double;
    innerV = SSV_sample_inner_MB_double_varying_64;
    innerU = SSV_sample_inner_MB_double_uniform_64;
    break;
  }
  default: {
    print("#vkl:shared_structured_volume: unknown voxelType\n");
    return false;
  }
  }

  // sampling functions for 32-bit addressing
  if (safe_32bit_indexing(self->attributesData[attributeIndex],
                          self->attributesData[attributeIndex].numItems)) {
    // in this case, we know ALL addressing can be 32-bit.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_MB_uint8_varying_32;
      innerU = SSV_sample_inner_MB_uint8_uniform_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_MB_int16_varying_32;
      innerU = SSV_sample_inner_MB_int16_uniform_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_MB_uint16_varying_32;
      innerU = SSV_sample_inner_MB_uint16_uniform_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_MB_half_varying_32;
      innerU = SSV_sample_inner_MB_half_uniform_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_MB_float_varying_32;
      innerU = SSV_sample_inner_MB_float_uniform_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_MB_double_varying_32;
      innerU = SSV_sample_inner_MB_double_uniform_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  else if (safe_32bit_indexing(self->attributesData[attributeIndex],
                               numTimeSamples * self->dimensions.x *
                                   (uniform uint64)self->dimensions.y)) {
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice beginning.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_MB_uint8_varying_64_32;
      innerU = SSV_sample_inner_MB_uint8_uniform_64_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_MB_int16_varying_64_32;
      innerU = SSV_sample_inner_MB_int16_uniform_64_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_MB_uint16_varying_64_32;
      innerU = SSV_sample_inner_MB_uint16_uniform_64_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_MB_half_varying_64_32;
      innerU = SSV_sample_inner_MB_half_uniform_64_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_MB_float_varying_64_32;
      innerU = SSV_sample_inner_MB_float_uniform_64_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_MB_double_varying_64_32;
      innerU = SSV_sample_inner_MB_double_uniform_64_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  assert(cvr && innerV && innerU);
  self->computeVoxelRange[attributeIndex]           = cvr;
  self->computeSamplesInner_varying[attributeIndex] = innerV;
  self->computeSamplesInner_uniform[attributeIndex] = innerU;

  return true;
}

export uniform bool EXPORT_UNIQUE(assignTemporallyUnstructuredSamplingFunctions,
    void *uniform _self, const uniform uint32 attributeIndex)
{
  SharedStructuredVolume *uniform self = (SharedStructuredVolume *uniform) _self;
  assert(hasUnstructuredTimeData(self));

  ComputeVoxelRangeFunc cvr            = NULL;
  ComputeSampleInnerVaryingFunc innerV = NULL;
  ComputeSampleInnerUniformFunc innerU = NULL;

  // default sampling functions (64-bit addressing on time indices)
  switch (self->attributesData[attributeIndex].dataType) {
  case (VKL_UCHAR): {
    cvr    = SSV_computeVoxelRange_TUV_uint8;
    innerV = SSV_sample_inner_TUV_uint8_varying_64;
    innerU = SSV_sample_inner_TUV_uint8_uniform_64;
    break;
  }
  case (VKL_SHORT): {
    cvr    = SSV_computeVoxelRange_TUV_int16;
    innerV = SSV_sample_inner_TUV_int16_varying_64;
    innerU = SSV_sample_inner_TUV_int16_uniform_64;
    break;
  }
  case (VKL_USHORT): {
    cvr    = SSV_computeVoxelRange_TUV_uint16;
    innerV = SSV_sample_inner_TUV_uint16_varying_64;
    innerU = SSV_sample_inner_TUV_uint16_uniform_64;
    break;
  }
  case (VKL_HALF): {
    cvr    = SSV_computeVoxelRange_TUV_half;
    innerV = SSV_sample_inner_TUV_half_varying_64;
    innerU = SSV_sample_inner_TUV_half_uniform_64;
    break;
  }
  case (VKL_FLOAT): {
    cvr    = SSV_computeVoxelRange_TUV_float;
    innerV = SSV_sample_inner_TUV_float_varying_64;
    innerU = SSV_sample_inner_TUV_float_uniform_64;
    break;
  }
  case (VKL_DOUBLE): {
    cvr    = SSV_computeVoxelRange_TUV_double;
    innerV = SSV_sample_inner_TUV_double_varying_64;
    innerU = SSV_sample_inner_TUV_double_uniform_64;
    break;
  }
  default: {
    print("#vkl:shared_structured_volume: unknown voxelType\n");
    return false;
  }
  }

  // sampling functions for 32-bit addressing (on time indices)
  if (safe_32bit_indexing(self->attributesData[attributeIndex],
                          self->attributesData[attributeIndex].numItems)) {
    // in this case, we know ALL addressing can be 32-bit.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_TUV_uint8_varying_32;
      innerU = SSV_sample_inner_TUV_uint8_uniform_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_TUV_int16_varying_32;
      innerU = SSV_sample_inner_TUV_int16_uniform_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_TUV_uint16_varying_32;
      innerU = SSV_sample_inner_TUV_uint16_uniform_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_TUV_half_varying_32;
      innerU = SSV_sample_inner_TUV_half_uniform_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_TUV_float_varying_32;
      innerU = SSV_sample_inner_TUV_float_uniform_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_TUV_double_varying_32;
      innerU = SSV_sample_inner_TUV_double_uniform_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  // sampling functions for 64/32-bit addressing (on time indices)
  else if (safe_32bit_indexing(
               self->attributesData[attributeIndex],
               self->dimensions.x * (uniform uint64)self->dimensions.y)) {
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice beginning.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

    switch (self->attributesData[attributeIndex].dataType) {
    case (VKL_UCHAR): {
      innerV = SSV_sample_inner_TUV_uint8_varying_64_32;
      innerU = SSV_sample_inner_TUV_uint8_uniform_64_32;
      break;
    }
    case (VKL_SHORT): {
      innerV = SSV_sample_inner_TUV_int16_varying_64_32;
      innerU = SSV_sample_inner_TUV_int16_uniform_64_32;
      break;
    }
    case (VKL_USHORT): {
      innerV = SSV_sample_inner_TUV_uint16_varying_64_32;
      innerU = SSV_sample_inner_TUV_uint16_uniform_64_32;
      break;
    }
    case (VKL_HALF): {
      innerV = SSV_sample_inner_TUV_half_varying_64_32;
      innerU = SSV_sample_inner_TUV_half_uniform_64_32;
      break;
    }
    case (VKL_FLOAT): {
      innerV = SSV_sample_inner_TUV_float_varying_64_32;
      innerU = SSV_sample_inner_TUV_float_uniform_64_32;
      break;
    }
    case (VKL_DOUBLE): {
      innerV = SSV_sample_inner_TUV_double_varying_64_32;
      innerU = SSV_sample_inner_TUV_double_uniform_64_32;
      break;
    }
    default: {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    }
  }

  assert(cvr && innerV && innerU);
  self->computeVoxelRange[attributeIndex]           = cvr;
  self->computeSamplesInner_varying[attributeIndex] = innerV;
  self->computeSamplesInner_uniform[attributeIndex] = innerU;

  return true;
}

export void EXPORT_UNIQUE(assignComputeGradientChecks, void *uniform _self) {
 SharedStructuredVolume *uniform self = (SharedStructuredVolume *uniform) _self;
 if (self->gridType == structured_regular)
 {
   self->computeGradient_varying = SharedStructuredVolume_computeGradient_bbox_checks;
 } else if (self->gridType == structured_spherical) {
   self->computeGradient_varying = SharedStructuredVolume_computeGradient_NaN_checks;
 }
}

export void EXPORT_UNIQUE(assignStructuredSamplerKernels,
                          void *uniform _self)
{
  StructuredSamplerShared *uniform ssampler =
      (StructuredSamplerShared * uniform) _self;
  SamplerShared *uniform sampler = &ssampler->super.super;
  sampler->computeSample_uniform = computeSample_uniform_0_iterator_wrapper;
  sampler->computeSample_varying = computeSample_varying_0_iterator_wrapper;
}

///////////////////////////////////////////////////////////////////////////////
// SharedStructuredVolume exported functions //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export void EXPORT_UNIQUE(SharedStructuredVolume_getBoundingBox,
                          void *uniform _self,
                          uniform box3f &boundingBox)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  boundingBox = self->boundingBox;
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const void *uniform _time,
                          void *uniform _samples)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    const varying float *uniform time = (const varying float *uniform)_time;
    varying float *uniform samples    = (varying float *uniform)_samples;

    *samples = SharedStructuredVolume_computeSample_varying(
        self, *objectCoordinates, sampler->filter, attributeIndex, *time);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_uniform_export,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const void *uniform _time,
                          void *uniform _sample)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  const float *uniform time = (const float *uniform)_time;
  float *uniform sample     = (float *uniform)_sample;

  *sample = SharedStructuredVolume_computeSample_uniform(
      self, *objectCoordinates, sampler->filter, attributeIndex, *time);
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_N_export,
                          const void *uniform _sampler,
                          const uniform uint32 N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const float *uniform time,
                          float *uniform samples)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    varying float t  = time ? time[i] : 0.f;

    samples[i] = SharedStructuredVolume_computeSample_varying(
        self, oc, sampler->filter, attributeIndex, t);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const void *uniform _time,
                          void *uniform _gradients)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    const varying float *uniform time = (const varying float *uniform)_time;
    varying vec3f *uniform gradients  = (varying vec3f * uniform) _gradients;
    *gradients                        = self->computeGradient_varying(self,
                                               *objectCoordinates,
                                               sampler->gradientFilter,
                                               attributeIndex,
                                               *time);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_N_export,
                          const void *uniform _sampler,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const float *uniform time,
                          vec3f *uniform gradients)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    varying float t  = time ? time[i] : 0.f;

    gradients[i] = self->computeGradient_varying(
        self, oc, sampler->gradientFilter, attributeIndex, t);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const void *uniform _time,
                          float *uniform samples)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    const varying float *uniform time = (const varying float *uniform)_time;

    vec3f clampedLocalCoordinates;
    bool inBounds;
    clampedLocalCoordinates_varying(
        self, *objectCoordinates, clampedLocalCoordinates, inBounds);

    if (inBounds) {
      for (uniform uint32 i = 0; i < M; i++) {
        float attributeSamples =
            self->computeSamplesInner_varying[attributeIndices[i]](
                self,
                clampedLocalCoordinates,
                sampler->filter,
                attributeIndices[i],
                *time);

        samples[i * VKL_TARGET_WIDTH + programIndex] = attributeSamples;
      }
    } else {
      for (uniform uint32 i = 0; i < M; i++) {
        samples[i * VKL_TARGET_WIDTH + programIndex] =
            sampler->volume->background[i];
      }
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_uniform_export,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const void *uniform _time,
                          float *uniform samples)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  const float *uniform time = (const float *uniform)_time;

  uniform vec3f clampedLocalCoordinates;
  uniform bool inBounds;
  clampedLocalCoordinates_uniform(
      self, *objectCoordinates, clampedLocalCoordinates, inBounds);

  if (inBounds) {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = self->computeSamplesInner_uniform[attributeIndices[i]](
          self,
          clampedLocalCoordinates,
          sampler->filter,
          attributeIndices[i],
          *time);
    }
  } else {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = sampler->volume->background[i];
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_N_export,
                          const void *uniform _sampler,
                          const uniform uint32 N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const float *uniform time,
                          float *uniform samples)
{
  const StructuredSamplerShared *uniform ssampler =
      (const StructuredSamplerShared *uniform)_sampler;
  const SamplerShared *uniform sampler = (const SamplerShared *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f objectCoordinatesV = objectCoordinates[i];
    varying float timeV              = time ? time[i] : 0.f;

    vec3f clampedLocalCoordinates;
    bool inBounds;
    clampedLocalCoordinates_varying(
        self, objectCoordinatesV, clampedLocalCoordinates, inBounds);

    if (inBounds) {
      for (uniform uint32 a = 0; a < M; a++) {
        varying float attributeSamples =
            self->computeSamplesInner_varying[attributeIndices[a]](
                self,
                clampedLocalCoordinates,
                sampler->filter,
                attributeIndices[a],
                timeV);

        samples[i * M + a] = attributeSamples;
      }
    } else {
      for (uniform uint32 a = 0; a < M; a++) {
        samples[i * M + a] = sampler->volume->background[a];
      }
    }
  }
}

