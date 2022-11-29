// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <rkcommon/common.h>
#include <rkcommon/math/box.h>
using namespace rkcommon::math;

#include "../common/Data.ih"

#include "../../cpu/sampler/SamplerShared.h"
#include "../../cpu/volume/StructuredVolumeShared.h"

#define template_SSV_sample_inner_uniform_32(dataType)                      \
  inline float SSV_sample_inner_##dataType##_uniform_32(                    \
      const ispc::SharedStructuredVolume *self,                             \
      const vec3f &clampedLocalCoordinates,                                 \
      const VKLFilter filter,                                               \
      const uint32_t attributeIndex,                                        \
      const float &_time)                                                   \
  {                                                                         \
    const ispc::Data1D voxelData = self->attributesData[attributeIndex];    \
                                                                            \
    /* lower corner of the box straddling the voxels to be interpolated. */ \
    const vec3i voxelIndex_0 = vec3i(clampedLocalCoordinates);              \
                                                                            \
    const uint32_t voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +          \
                              voxelIndex_0.y * self->voxelOfs_dy +          \
                              voxelIndex_0.z * self->voxelOfs_dz;           \
                                                                            \
    float val = 0.f;                                                        \
    switch (filter) {                                                       \
    case VKL_FILTER_TRICUBIC:                                               \
      break;                                                                \
    case VKL_FILTER_NEAREST: {                                              \
      val = get_##dataType(voxelData, voxelOfs);                            \
      break;                                                                \
    }                                                                       \
    case VKL_FILTER_TRILINEAR: {                                            \
      /* fractional coordinates within the lower corner voxel used during   \
       * interpolation. */                                                  \
      const vec3f frac = clampedLocalCoordinates - vec3f(voxelIndex_0);     \
                                                                            \
      const uint64_t ofs000 = 0;                                            \
      const uint64_t ofs001 = self->voxelOfs_dx;                            \
                                                                            \
      const float val000 = get_##dataType(voxelData, ofs000, voxelOfs);     \
      const float val001 = get_##dataType(voxelData, ofs001, voxelOfs);     \
      const float val00  = val000 + frac.x * (val001 - val000);             \
                                                                            \
      const uint64_t ofs010 = self->voxelOfs_dy;                            \
      const uint64_t ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;        \
      const float val010    = get_##dataType(voxelData, ofs010, voxelOfs);  \
      const float val011    = get_##dataType(voxelData, ofs011, voxelOfs);  \
      const float val01     = val010 + frac.x * (val011 - val010);          \
                                                                            \
      const uint64_t ofs100 = self->voxelOfs_dz;                            \
      const uint64_t ofs101 = ofs100 + ofs001;                              \
      const float val100    = get_##dataType(voxelData, ofs100, voxelOfs);  \
      const float val101    = get_##dataType(voxelData, ofs101, voxelOfs);  \
      const float val10     = val100 + frac.x * (val101 - val100);          \
                                                                            \
      const uint64_t ofs110 = ofs100 + ofs010;                              \
      const uint64_t ofs111 = ofs100 + ofs011;                              \
      const float val110    = get_##dataType(voxelData, ofs110, voxelOfs);  \
      const float val111    = get_##dataType(voxelData, ofs111, voxelOfs);  \
      const float val11     = val110 + frac.x * (val111 - val110);          \
                                                                            \
      const float val0 = val00 + frac.y * (val01 - val00);                  \
      const float val1 = val10 + frac.y * (val11 - val10);                  \
                                                                            \
      val = val0 + frac.z * (val1 - val0);                                  \
      break;                                                                \
    }                                                                       \
    }                                                                       \
                                                                            \
    return val;                                                             \
  }

template_SSV_sample_inner_uniform_32(uint8);
template_SSV_sample_inner_uniform_32(int16);
template_SSV_sample_inner_uniform_32(uint16);
template_SSV_sample_inner_uniform_32(float);

inline vec3f make_vec3f_rkcommon(const ispc::vec3f &data)
{
  return *reinterpret_cast<vec3f *>((void *)&data);
}

inline void transformObjectToLocal_uniform_structured_regular(
    const ispc::SharedStructuredVolume *self,
    const vec3f &objectCoordinates,
    vec3f &localCoordinates)
{
  localCoordinates =
      1.f / make_vec3f_rkcommon(self->gridSpacing) *
      (objectCoordinates - make_vec3f_rkcommon(self->gridOrigin));
}

inline float SharedStructuredVolume_computeSample_uniform(
    const ispc::SharedStructuredVolume *self,
    const vec3f &objectCoordinates,
    const VKLFilter filter,
    const uint32_t attributeIndex,
    const float &time)
{
  vec3f clampedLocalCoordinates;

  /* computing clampedLocalCoordinates directly here rather than
   * using the above clampedLocalCoordinates_univary() function
   * produces more efficient code; clampedLocalCoordinates_univary()
   * is still used for multi-attribute sampling functions... */
  transformObjectToLocal_uniform_structured_regular(
      self, objectCoordinates, clampedLocalCoordinates);

  if (clampedLocalCoordinates.x < 0.f ||
      clampedLocalCoordinates.x > self->dimensions.x - 1.f ||
      clampedLocalCoordinates.y < 0.f ||
      clampedLocalCoordinates.y > self->dimensions.y - 1.f ||
      clampedLocalCoordinates.z < 0.f ||
      clampedLocalCoordinates.z > self->dimensions.z - 1.f) {
    return self->super.background[attributeIndex];
  }

  clampedLocalCoordinates =
      clamp(clampedLocalCoordinates,
            vec3f(0.0f),
            make_vec3f_rkcommon(self->localCoordinatesUpperBound));

  switch (self->attributesData[attributeIndex].dataType) {
  case VKL_UCHAR:
    return SSV_sample_inner_uint8_uniform_32(
        self, clampedLocalCoordinates, filter, attributeIndex, time);

  case VKL_SHORT:
    return SSV_sample_inner_int16_uniform_32(
        self, clampedLocalCoordinates, filter, attributeIndex, time);

  case VKL_USHORT:
    return SSV_sample_inner_uint16_uniform_32(
        self, clampedLocalCoordinates, filter, attributeIndex, time);

  case VKL_FLOAT:
    return SSV_sample_inner_float_uniform_32(
        self, clampedLocalCoordinates, filter, attributeIndex, time);

  default:
    return -1.0f;
  }
}
