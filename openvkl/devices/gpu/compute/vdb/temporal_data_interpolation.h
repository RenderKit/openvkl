// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.ih"

// ---------------------------------------------------------------------------
// A helper we use because data accessors take references for uniform buffers
// and pointers for varying buffers.
// ---------------------------------------------------------------------------

#define __vkl_data_param_uniform(buffer) (*buffer)

// ---------------------------------------------------------------------------
// Temporally structured data interpolation.
// This assumes that time steps are stored in consecutive memory,
// {v(t0), v(t1), ...}.
// ---------------------------------------------------------------------------

#define __vkl_template_interpolateTemporallyStructured(                      \
    univaryBuffer, univaryIdx, typeIdx, voxelType)                           \
  inline univaryIdx float interpolateTemporallyStructured_##voxelType(       \
      const Data1D *univaryBuffer buffer,                                    \
      const univaryBuffer int32 &numTimesteps,                               \
      const univaryIdx typeIdx &baseIdx,                                     \
      const univaryIdx float &time)                                          \
  {                                                                          \
    const univaryIdx float timestepFloat =                                   \
        time * ((univaryIdx float)(numTimesteps - 1));                       \
    const univaryIdx uint32 timestep = ((univaryIdx uint32)timestepFloat);   \
    const univaryIdx float w = timestepFloat - ((univaryIdx float)timestep); \
    /* If interpolant is zero, we don't need to access the next time entry.  \
     * This also prevents illegal accesses for the time == 1.f case at the   \
     * volume boundary. */                                                   \
    const univaryIdx uint32 nextTimestep =                                   \
        (w == 0.f) ? timestep : timestep + 1;                                \
                                                                             \
    if (buffer->compact) {                                                   \
      const univaryIdx float v0 = get_##voxelType##_compact(                 \
          __vkl_data_param_##univaryBuffer(buffer), baseIdx + timestep);     \
      const univaryIdx float v1 = get_##voxelType##_compact(                 \
          __vkl_data_param_##univaryBuffer(buffer), baseIdx + nextTimestep); \
      return (1.f - w) * v0 + w * v1;                                        \
    } else {                                                                 \
      const univaryIdx float v0 = get_##voxelType##_strided(                 \
          __vkl_data_param_##univaryBuffer(buffer), baseIdx + timestep);     \
      const univaryIdx float v1 = get_##voxelType##_strided(                 \
          __vkl_data_param_##univaryBuffer(buffer), baseIdx + nextTimestep); \
      return (1.f - w) * v0 + w * v1;                                        \
    }                                                                        \
  }

// A single buffer, sampled at a single index and time.
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, uint8)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, int16)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, uint16)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, float)

/* not currently supported on GPU:
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, half)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint32, double)
*/

// same as above, supporting 64-bit indexing used only for dense volumes.
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, uint8)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, int16)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, uint16)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, float)

/* not currently supported on GPU:
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, half)
__vkl_template_interpolateTemporallyStructured(uniform, uniform, uint64, double)
*/

#undef __vkl_template_interpolateTemporallyStructured

// ---------------------------------------------------------------------------
// Temporally unstructured data interpolation.
// This assumes that time steps are stored in consecutive memory,
// {v(t0), v(t1), ...}.
// ---------------------------------------------------------------------------

#define __vkl_template_interpolateTemporallyUnstructured(                      \
    univaryBuffer, univaryIdx, idxType, voxelType)                             \
                                                                               \
  inline univaryIdx float interpolateTemporallyUnstructured_##voxelType(       \
      const Data1D *univaryBuffer dataBuffer,                                  \
      const Data1D *univaryBuffer indices,                                     \
      const Data1D *univaryBuffer times,                                       \
      const univaryIdx idxType &voxelIdx,                                      \
      const univaryIdx float &time)                                            \
  {                                                                            \
    /* Note that we only support 32 bit indices at the moment, but that could  \
     * be extended in the future. */                                           \
    univaryIdx idxType dataIdx, dataNextIdx;                                   \
    if (indices->dataType == VKL_UINT) {                                       \
      dataIdx =                                                                \
          get_uint32(__vkl_data_param_##univaryBuffer(indices), voxelIdx);     \
      dataNextIdx =                                                            \
          get_uint32(__vkl_data_param_##univaryBuffer(indices), voxelIdx + 1); \
    } else {                                                                   \
      const univaryIdx uint64 dataIdx64 =                                      \
          get_uint64(__vkl_data_param_##univaryBuffer(indices), voxelIdx);     \
      const univaryIdx uint64 dataNextIdx64 =                                  \
          get_uint64(__vkl_data_param_##univaryBuffer(indices), voxelIdx + 1); \
      assert(dataIdx64 < ((uint64)1) << sizeof(idxType));                      \
      assert(dataNextIdx64 < ((uint64)1) << sizeof(idxType));                  \
      dataIdx     = ((univaryIdx idxType)dataIdx64);                           \
      dataNextIdx = ((univaryIdx idxType)dataNextIdx64);                       \
    }                                                                          \
    assert(dataNextIdx > dataIdx);                                             \
    const univaryIdx float t0 =                                                \
        get_float(__vkl_data_param_##univaryBuffer(times), dataIdx);           \
    if (time <= t0) {                                                          \
      return get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer),     \
                             dataIdx);                                         \
    }                                                                          \
    const univaryIdx float t1 =                                                \
        get_float(__vkl_data_param_##univaryBuffer(times), dataNextIdx - 1);   \
    if (time >= t1) {                                                          \
      return get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer),     \
                             dataNextIdx - 1);                                 \
    }                                                                          \
                                                                               \
    const univaryIdx uint64 numTimeSamples = dataNextIdx - dataIdx;            \
    if (numTimeSamples == 2) {                                                 \
      const univaryIdx float w = (time - t0) * rcp(t1 - t0);                   \
      return (1.f - w) *                                                       \
                 get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer), \
                                 dataIdx) +                                    \
             w * get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer), \
                                 dataNextIdx - 1);                             \
    }                                                                          \
                                                                               \
    univaryIdx uint64 lower = 0;                                               \
    univaryIdx uint64 upper = numTimeSamples;                                  \
    while (lower <= upper) {                                                   \
      const univaryIdx uint64 mid = (lower + upper) / 2;                       \
      const univaryIdx float timesMid =                                        \
          get_float(__vkl_data_param_##univaryBuffer(times), dataIdx + mid);   \
      if (time >= timesMid &&                                                  \
          time <= get_float(__vkl_data_param_##univaryBuffer(times),           \
                            dataIdx + mid + 1)) {                              \
        lower = dataIdx + mid;                                                 \
        upper = lower + 1;                                                     \
        break;                                                                 \
      } else if (time < timesMid) {                                            \
        upper = mid;                                                           \
      } else {                                                                 \
        lower = mid;                                                           \
      }                                                                        \
    }                                                                          \
    const univaryIdx float timesLow =                                          \
        get_float(__vkl_data_param_##univaryBuffer(times), lower);             \
    const univaryIdx float timesHigh =                                         \
        get_float(__vkl_data_param_##univaryBuffer(times), upper);             \
    const univaryIdx float w = (time - timesLow) * rcp(timesHigh - timesLow);  \
    return (1.f - w) *                                                         \
               get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer),   \
                               lower) +                                        \
           w * get_##voxelType(__vkl_data_param_##univaryBuffer(dataBuffer),   \
                               upper);                                         \
  }

// Dense VDB volumes additionally use [uint8, int16, uint16, double] voxel
// types, but only with uniform buffers.
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint32,
                                                 uint8)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint32,
                                                 int16)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint32,
                                                 uint16)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint32,
                                                 float)

/* not currently supported on GPU:
__vkl_template_interpolateTemporallyUnstructured(uniform, uniform, uint32, half)
__vkl_template_interpolateTemporallyUnstructured(uniform, uniform, uint32,
double)
*/

// same as above, supporting 64-bit indexing used only for dense volumes.
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint64,
                                                 uint8)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint64,
                                                 int16)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint64,
                                                 uint16)
__vkl_template_interpolateTemporallyUnstructured(uniform,
                                                 uniform,
                                                 uint64,
                                                 float)

/* not currently supported on GPU:
//__vkl_template_interpolateTemporallyUnstructured(uniform, uniform, uint64,
// half)
//__vkl_template_interpolateTemporallyUnstructured(uniform, uniform, uint64,
// double)
*/

#undef __vkl_template_interpolateTemporallyUnstructured

#define __vkl_template_computeValueRangeTemporallyUnstructured(voxelType, \
                                                               idxType)   \
                                                                          \
  inline box1f computeValueRangeTemporallyUnstructured_##voxelType(       \
      const Data1D *dataBuffer, const Data1D *indices, idxType voxelIdx)  \
  {                                                                       \
    box1f valueRange = make_box1f(pos_inf, neg_inf);                      \
    if (indices->dataType == VKL_UINT) {                                  \
      const idxType dataIdx = ((idxType)get_uint32(*indices, voxelIdx));  \
      const idxType dataIdxNext =                                         \
          ((idxType)get_uint32(*indices, voxelIdx + 1));                  \
      for (idxType i = dataIdx; i < dataIdxNext; ++i) {                   \
        extend(valueRange, get_##voxelType(*dataBuffer, i));              \
      }                                                                   \
    } else {                                                              \
      const idxType dataIdx = ((idxType)get_uint64(*indices, voxelIdx));  \
      const idxType dataIdxNext =                                         \
          ((idxType)get_uint64(*indices, voxelIdx + 1));                  \
      for (idxType i = dataIdx; i < dataIdxNext; ++i) {                   \
        extend(valueRange, get_##voxelType(*dataBuffer, i));              \
      }                                                                   \
    }                                                                     \
    return valueRange;                                                    \
  }

__vkl_template_computeValueRangeTemporallyUnstructured(uint8, uint32)
__vkl_template_computeValueRangeTemporallyUnstructured(int16, uint32)
__vkl_template_computeValueRangeTemporallyUnstructured(uint16, uint32)
__vkl_template_computeValueRangeTemporallyUnstructured(float, uint32)

/* not currently supported on GPU:
__vkl_template_computeValueRangeTemporallyUnstructured(half, uint32)
__vkl_template_computeValueRangeTemporallyUnstructured(double, uint32)
*/

__vkl_template_computeValueRangeTemporallyUnstructured(uint8, uint64)
__vkl_template_computeValueRangeTemporallyUnstructured(int16, uint64)
__vkl_template_computeValueRangeTemporallyUnstructured(uint16, uint64)
__vkl_template_computeValueRangeTemporallyUnstructured(float, uint64)

/* not currently supported on GPU:
__vkl_template_computeValueRangeTemporallyUnstructured(half, uint64)
__vkl_template_computeValueRangeTemporallyUnstructured(double, uint64)
*/

#undef __vkl_template_computeValueRangeTemporallyUnstructured
