// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"
#include "GridAccelerator.ih"
#include "SharedStructuredVolume.ih"

// #define PRINT_DEBUG_ENABLE
#include "common/print_debug.ih"

///////////////////////////////////////////////////////////////////////////////
// Coordinate transformations for all supported structured volume types ///////
///////////////////////////////////////////////////////////////////////////////

// Structured regular /////////////////////////////////////////////////////////

inline void transformLocalToObject_structured_regular(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &localCoordinates,
    varying vec3f &objectCoordinates)
{
  objectCoordinates = self->gridOrigin + localCoordinates * self->gridSpacing;
}

inline void transformObjectToLocal_structured_regular(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    varying vec3f &localCoordinates)
{
  localCoordinates =
      1.f / (self->gridSpacing) * (objectCoordinates - self->gridOrigin);
}

inline void transformLocalToObjectUniform_structured_regular(
    const SharedStructuredVolume *uniform self,
    const uniform vec3f &localCoordinates,
    uniform vec3f &objectCoordinates)
{
  objectCoordinates = self->gridOrigin + localCoordinates * self->gridSpacing;
}

inline void transformObjectToLocalUniform_structured_regular(
    const SharedStructuredVolume *uniform self,
    const uniform vec3f &objectCoordinates,
    uniform vec3f &localCoordinates)
{
  localCoordinates =
      1.f / (self->gridSpacing) * (objectCoordinates - self->gridOrigin);
}

// Structured spherical ///////////////////////////////////////////////////////

#define template_transformLocalToObject_structured_spherical(univary)       \
  inline void transformLocalToObject_##univary##_structured_spherical(      \
      const SharedStructuredVolume *uniform self,                           \
      const univary vec3f &localCoordinates,                                \
      univary vec3f &objectCoordinates)                                     \
  {                                                                         \
    /* (r, inclination, azimuth) -> (x, y, z), using the ISO convention for \
     coordinates and ordering. all angles in radians. */                    \
                                                                            \
    const univary float r =                                                 \
        self->gridOrigin.x + localCoordinates.x * self->gridSpacing.x;      \
                                                                            \
    const univary float inclination =                                       \
        self->gridOrigin.y + localCoordinates.y * self->gridSpacing.y;      \
                                                                            \
    const univary float azimuth =                                           \
        self->gridOrigin.z + localCoordinates.z * self->gridSpacing.z;      \
                                                                            \
    univary float sinInc, cosInc;                                           \
    sincos(inclination, &sinInc, &cosInc);                                  \
                                                                            \
    univary float sinAz, cosAz;                                             \
    sincos(azimuth, &sinAz, &cosAz);                                        \
                                                                            \
    objectCoordinates.x = r * sinInc * cosAz;                               \
    objectCoordinates.y = r * sinInc * sinAz;                               \
    objectCoordinates.z = r * cosInc;                                       \
  }

template_transformLocalToObject_structured_spherical(varying);
template_transformLocalToObject_structured_spherical(uniform);
#undef template_transformLocalToObject_structured_spherical

#define template_transformObjectToLocal_structured_spherical(univary)         \
  inline void transformObjectToLocal_##univary##_structured_spherical(        \
      const SharedStructuredVolume *uniform self,                             \
      const univary vec3f &objectCoordinates,                                 \
      univary vec3f &localCoordinates)                                        \
  {                                                                           \
    /* (x, y, z) -> (r, inclination, azimuth), using the ISO convention for   \
     coordinates and ordering. all angles in radians. */                      \
    const univary float r = sqrtf(objectCoordinates.x * objectCoordinates.x + \
                                  objectCoordinates.y * objectCoordinates.y + \
                                  objectCoordinates.z * objectCoordinates.z); \
                                                                              \
    const univary float inclination = acos(objectCoordinates.z / r);          \
                                                                              \
    univary float azimuth = atan2(objectCoordinates.y, objectCoordinates.x);  \
                                                                              \
    /* the above returns [-PI, PI], while our azimuth grid convention is [0,  \
     * 2*PI] */                                                               \
    if (azimuth < 0.f) {                                                      \
      azimuth += 2.f * PI;                                                    \
    }                                                                         \
                                                                              \
    localCoordinates.x =                                                      \
        (1.f / self->gridSpacing.x) * (r - self->gridOrigin.x);               \
    localCoordinates.y =                                                      \
        (1.f / self->gridSpacing.y) * (inclination - self->gridOrigin.y);     \
    localCoordinates.z =                                                      \
        (1.f / self->gridSpacing.z) * (azimuth - self->gridOrigin.z);         \
  }

template_transformObjectToLocal_structured_spherical(varying);
template_transformObjectToLocal_structured_spherical(uniform);
#undef template_transformObjectToLocal_structured_spherical

inline void computeStructuredSphericalBoundingBox(
    const SharedStructuredVolume *uniform self, uniform box3f &boundingBox)
{
  uniform box1f rRange = make_box1f(
      self->gridOrigin.x,
      self->gridOrigin.x + (self->dimensions.x - 1.f) * self->gridSpacing.x);

  uniform box1f incRange = make_box1f(
      self->gridOrigin.y,
      self->gridOrigin.y + (self->dimensions.y - 1.f) * self->gridSpacing.y);

  uniform box1f azRange = make_box1f(
      self->gridOrigin.z,
      self->gridOrigin.z + (self->dimensions.z - 1.f) * self->gridSpacing.z);

  // reverse ranges in case of negative gridSpacing values
  if (isEmpty(rRange)) {
    rRange = make_box1f(rRange.upper, rRange.lower);
  }

  if (isEmpty(incRange)) {
    incRange = make_box1f(incRange.upper, incRange.lower);
  }

  if (isEmpty(azRange)) {
    azRange = make_box1f(azRange.upper, azRange.lower);
  }

  // critical values to test
#define NUM_R_TEST_VALUES 2
#define NUM_INCLINATION_TEST_VALUES 5
#define NUM_AZIMUTH_TEST_VALUES 7

  const uniform float rs[NUM_R_TEST_VALUES] = {rRange.lower, rRange.upper};

  // inclination grid is guaranteed in [0, PI]
  const uniform float inclinations[NUM_INCLINATION_TEST_VALUES] = {
      0.f, 0.5f * PI, PI, incRange.lower, incRange.upper};

  // azimuth grid is guaranteed in [0, 2*PI]
  const uniform float azimuths[NUM_AZIMUTH_TEST_VALUES] = {
      0.f, 0.5f * PI, PI, 1.5f * PI, 2.f * PI, azRange.lower, azRange.upper};

  boundingBox = make_box3f_empty();

  // iterate over critical values and extend bounding box
  for (uniform int i = 0; i < NUM_R_TEST_VALUES; i++) {
    for (uniform int j = 0; j < NUM_INCLINATION_TEST_VALUES; j++) {
      for (uniform int k = 0; k < NUM_AZIMUTH_TEST_VALUES; k++) {
        const uniform float r   = rs[i];
        const uniform float inc = inclinations[j];
        const uniform float az  = azimuths[k];

        // skip values outside the grid
        if (inc < incRange.lower || inc > incRange.upper ||
            az < azRange.lower || az > azRange.upper) {
          continue;
        }

        uniform float sinInc, cosInc;
        sincos(inc, &sinInc, &cosInc);

        uniform float sinAz, cosAz;
        sincos(az, &sinAz, &cosAz);

        uniform vec3f objectCoordinates;
        objectCoordinates.x = r * sinInc * cosAz;
        objectCoordinates.y = r * sinInc * sinAz;
        objectCoordinates.z = r * cosInc;

        boundingBox = box_extend(boundingBox, objectCoordinates);
      }
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// getVoxel functions for all addressing / voxel type combinations ////////////
///////////////////////////////////////////////////////////////////////////////

// used below in template_getVoxel
#define process_index_z(univary) process_index_z_##univary
#define process_index_z_varying foreach_unique(z in index.z)
#define process_index_z_uniform uniform int z = index.z;

#define template_getVoxel(type, univary)                                     \
  /* for pure 32-bit addressing. volume *MUST* be smaller than 2G */         \
  inline void SSV_getVoxel_##type##_##univary##_32(                          \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      const uniform uint32 attributeIndex,                                   \
      univary float &value)                                                  \
  {                                                                          \
    const univary uint32 index32 =                                           \
        index.x +                                                            \
        self->dimensions.x * (index.y + self->dimensions.y * index.z);       \
                                                                             \
    value = get_##type(self->attributesData[attributeIndex], index32);       \
  }                                                                          \
  /* for 64/32-bit addressing. volume itself can be larger than 2G, but each \
   * slice must be within the 2G limit. */                                   \
  inline void SSV_getVoxel_##type##_##univary##_64_32(                       \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      const uniform uint32 attributeIndex,                                   \
      univary float &value)                                                  \
  {                                                                          \
    /* iterate over slices, then do 32-bit gather in slice */                \
    const univary uint32 offsetIndex32 =                                     \
        index.x + self->dimensions.x * index.y;                              \
    process_index_z(univary)                                                 \
    {                                                                        \
      const uniform uint64 baseIndex64 =                                     \
          (uint64)z * self->dimensions.x * self->dimensions.y;               \
      value = get_##type(                                                    \
          self->attributesData[attributeIndex], baseIndex64, offsetIndex32); \
    }                                                                        \
  }                                                                          \
  /* for full 64-bit addressing, for all dimensions or slice size */         \
  inline void SSV_getVoxel_##type##_##univary##_64(                          \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      const uniform uint32 attributeIndex,                                   \
      univary float &value)                                                  \
  {                                                                          \
    const univary uint64 index64 =                                           \
        (uint64)index.x +                                                    \
        self->dimensions.x *                                                 \
            ((uint64)index.y + self->dimensions.y * ((uint64)index.z));      \
    value = get_##type(self->attributesData[attributeIndex], index64);       \
  }

template_getVoxel(uint8, varying);
template_getVoxel(int16, varying);
template_getVoxel(uint16, varying);
template_getVoxel(float, varying);
template_getVoxel(double, varying);

template_getVoxel(uint8, uniform);
template_getVoxel(int16, uniform);
template_getVoxel(uint16, uniform);
template_getVoxel(float, uniform);
template_getVoxel(double, uniform);
#undef template_getVoxel

///////////////////////////////////////////////////////////////////////////////
// Sampling methods for all addressing / voxel type combinations //////////////
///////////////////////////////////////////////////////////////////////////////

// overloads for both varying and uniform voxel getters, used in templated
// sampling functions
inline void getVoxelUnivary(const SharedStructuredVolume *uniform self,
                            const varying vec3i &index,
                            const uniform uint32 attributeIndex,
                            varying float &value)
{
  self->getVoxels_varying[attributeIndex](self, index, attributeIndex, value);
}

inline void getVoxelUnivary(const SharedStructuredVolume *uniform self,
                            const uniform vec3i &index,
                            const uniform uint32 attributeIndex,
                            uniform float &value)
{
  self->getVoxels_uniform[attributeIndex](self, index, attributeIndex, value);
}

// perform trilinear interpolation for given sample. unlike old way of doing
// this (a single computesample on the StructuredVolume level that calls the
// virtual 'getSample()' of the volume layout) this function will directly do
// all the addressing for the getSample (inlined), and thus be about 50% faster
// (wall-time, meaning even much faster in pure sample speed)
#define template_sample_32(type, univary)                                      \
  inline univary float SSV_sample_##type##_##univary##_32(                     \
      const void *uniform _self,                                               \
      const univary vec3f &objectCoordinates,                                  \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];     \
                                                                               \
    univary vec3f localCoordinates;                                            \
    self->transformObjectToLocal_##univary(                                    \
        self, objectCoordinates, localCoordinates);                            \
                                                                               \
    /* return NaN for local coordinates outside the bounds of the volume. */   \
    const uniform int NaN_bits   = 0x7fc00000;                                 \
    const uniform float nanValue = floatbits(NaN_bits);                        \
                                                                               \
    if (localCoordinates.x < 0.f ||                                            \
        localCoordinates.x > self->dimensions.x - 1.f ||                       \
        localCoordinates.y < 0.f ||                                            \
        localCoordinates.y > self->dimensions.y - 1.f ||                       \
        localCoordinates.z < 0.f ||                                            \
        localCoordinates.z > self->dimensions.z - 1.f) {                       \
      return nanValue;                                                         \
    }                                                                          \
                                                                               \
    const univary vec3f clampedLocalCoordinates = clamp(                       \
        localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound); \
                                                                               \
    /* lower corner of the box straddling the voxels to be interpolated. */    \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);        \
                                                                               \
    /* fractional coordinates within the lower corner voxel used during        \
     * interpolation. */                                                       \
    const univary vec3f frac =                                                 \
        clampedLocalCoordinates - to_float(voxelIndex_0);                      \
                                                                               \
    const univary uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +       \
                                    voxelIndex_0.y * self->voxelOfs_dy +       \
                                    voxelIndex_0.z * self->voxelOfs_dz;        \
                                                                               \
    const uniform uint64 ofs000 = 0;                                           \
    const uniform uint64 ofs001 = self->voxelOfs_dx;                           \
    const univary float val000  = get_##type(voxelData, ofs000, voxelOfs);     \
    const univary float val001  = get_##type(voxelData, ofs001, voxelOfs);     \
    const univary float val00   = val000 + frac.x * (val001 - val000);         \
                                                                               \
    const uniform uint64 ofs010 = self->voxelOfs_dy;                           \
    const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;       \
    const univary float val010  = get_##type(voxelData, ofs010, voxelOfs);     \
    const univary float val011  = get_##type(voxelData, ofs011, voxelOfs);     \
    const univary float val01   = val010 + frac.x * (val011 - val010);         \
                                                                               \
    const uniform uint64 ofs100 = self->voxelOfs_dz;                           \
    const uniform uint64 ofs101 = ofs100 + ofs001;                             \
    const univary float val100  = get_##type(voxelData, ofs100, voxelOfs);     \
    const univary float val101  = get_##type(voxelData, ofs101, voxelOfs);     \
    const univary float val10   = val100 + frac.x * (val101 - val100);         \
                                                                               \
    const uniform uint64 ofs110 = ofs100 + ofs010;                             \
    const uniform uint64 ofs111 = ofs100 + ofs011;                             \
    const univary float val110  = get_##type(voxelData, ofs110, voxelOfs);     \
    const univary float val111  = get_##type(voxelData, ofs111, voxelOfs);     \
    const univary float val11   = val110 + frac.x * (val111 - val110);         \
                                                                               \
    const univary float val0 = val00 + frac.y * (val01 - val00);               \
    const univary float val1 = val10 + frac.y * (val11 - val10);               \
    const univary float val  = val0 + frac.z * (val1 - val0);                  \
                                                                               \
    return val;                                                                \
  }

template_sample_32(uint8, varying);
template_sample_32(int16, varying);
template_sample_32(uint16, varying);
template_sample_32(float, varying);
template_sample_32(double, varying);

template_sample_32(uint8, uniform);
template_sample_32(int16, uniform);
template_sample_32(uint16, uniform);
template_sample_32(float, uniform);
template_sample_32(double, uniform);
#undef template_sample_32

// used below in template_sample_64_32
#define process_sliceID(univary) process_sliceID_##univary
#define process_sliceID_varying foreach_unique(sliceID in voxelIndex_0.z)
#define process_sliceID_uniform uniform int sliceID = voxelIndex_0.z;

#define template_sample_64_32(type, univary)                                   \
  inline univary float SSV_sample_##type##_##univary##_64_32(                  \
      const void *uniform _self,                                               \
      const univary vec3f &objectCoordinates,                                  \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];     \
                                                                               \
    univary vec3f localCoordinates;                                            \
    self->transformObjectToLocal_##univary(                                    \
        self, objectCoordinates, localCoordinates);                            \
                                                                               \
    /* return NaN for local coordinates outside the bounds of the volume. */   \
    const uniform int NaN_bits   = 0x7fc00000;                                 \
    const uniform float nanValue = floatbits(NaN_bits);                        \
                                                                               \
    if (localCoordinates.x < 0.f ||                                            \
        localCoordinates.x > self->dimensions.x - 1.f ||                       \
        localCoordinates.y < 0.f ||                                            \
        localCoordinates.y > self->dimensions.y - 1.f ||                       \
        localCoordinates.z < 0.f ||                                            \
        localCoordinates.z > self->dimensions.z - 1.f) {                       \
      return nanValue;                                                         \
    }                                                                          \
                                                                               \
    const univary vec3f clampedLocalCoordinates = clamp(                       \
        localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound); \
                                                                               \
    /* lower corner of the box straddling the voxels to be interpolated. */    \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);        \
                                                                               \
    /* fractional coordinates within the lower corner voxel used during        \
     * interpolation. */                                                       \
    const univary vec3f frac =                                                 \
        clampedLocalCoordinates - to_float(voxelIndex_0);                      \
                                                                               \
    univary float ret = 0.f;                                                   \
    process_sliceID(univary)                                                   \
    {                                                                          \
      const uniform uint64 sliceOfs =                                          \
          (uint64)sliceID * self->dimensions.x * self->dimensions.y;           \
      const univary uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +     \
                                      voxelIndex_0.y * self->voxelOfs_dy;      \
                                                                               \
      const uniform uint64 ofs000 = 0;                                         \
      const uniform uint64 ofs001 = self->voxelOfs_dx;                         \
      const univary float val000 =                                             \
          get_##type(voxelData, sliceOfs + ofs000, voxelOfs);                  \
      const univary float val001 =                                             \
          get_##type(voxelData, sliceOfs + ofs001, voxelOfs);                  \
      const univary float val00 = val000 + frac.x * (val001 - val000);         \
                                                                               \
      const uniform uint64 ofs010 = self->voxelOfs_dy;                         \
      const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;     \
      const univary float val010 =                                             \
          get_##type(voxelData, sliceOfs + ofs010, voxelOfs);                  \
      const univary float val011 =                                             \
          get_##type(voxelData, sliceOfs + ofs011, voxelOfs);                  \
      const univary float val01 = val010 + frac.x * (val011 - val010);         \
                                                                               \
      const uniform uint64 ofs100 = self->voxelOfs_dz;                         \
      const uniform uint64 ofs101 = ofs100 + ofs001;                           \
      const univary float val100 =                                             \
          get_##type(voxelData, sliceOfs + ofs100, voxelOfs);                  \
      const univary float val101 =                                             \
          get_##type(voxelData, sliceOfs + ofs101, voxelOfs);                  \
      const univary float val10 = val100 + frac.x * (val101 - val100);         \
                                                                               \
      const uniform uint64 ofs110 = ofs100 + ofs010;                           \
      const uniform uint64 ofs111 = ofs100 + ofs011;                           \
      const univary float val110 =                                             \
          get_##type(voxelData, sliceOfs + ofs110, voxelOfs);                  \
      const univary float val111 =                                             \
          get_##type(voxelData, sliceOfs + ofs111, voxelOfs);                  \
      const univary float val11 = val110 + frac.x * (val111 - val110);         \
                                                                               \
      const univary float val0 = val00 + frac.y * (val01 - val00);             \
      const univary float val1 = val10 + frac.y * (val11 - val10);             \
      const univary float val  = val0 + frac.z * (val1 - val0);                \
      ret                      = val;                                          \
    }                                                                          \
    return ret;                                                                \
  }

template_sample_64_32(uint8, varying);
template_sample_64_32(int16, varying);
template_sample_64_32(uint16, varying);
template_sample_64_32(float, varying);
template_sample_64_32(double, varying);

template_sample_64_32(uint8, uniform);
template_sample_64_32(int16, uniform);
template_sample_64_32(uint16, uniform);
template_sample_64_32(float, uniform);
template_sample_64_32(double, uniform);
#undef template_sample_64_32

// default sampling function (64-bit addressing)
#define template_sample_64(univary)                                            \
  inline univary float SSV_sample_##univary##_64(                              \
      const void *uniform _self,                                               \
      const univary vec3f &objectCoordinates,                                  \
      const uniform uint32 attributeIndex)                                     \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    univary vec3f localCoordinates;                                            \
    self->transformObjectToLocal_##univary(                                    \
        self, objectCoordinates, localCoordinates);                            \
                                                                               \
    /* return NaN for local coordinates outside the bounds of the volume. */   \
    const uniform int NaN_bits   = 0x7fc00000;                                 \
    const uniform float nanValue = floatbits(NaN_bits);                        \
                                                                               \
    if (localCoordinates.x < 0.f ||                                            \
        localCoordinates.x > self->dimensions.x - 1.f ||                       \
        localCoordinates.y < 0.f ||                                            \
        localCoordinates.y > self->dimensions.y - 1.f ||                       \
        localCoordinates.z < 0.f ||                                            \
        localCoordinates.z > self->dimensions.z - 1.f) {                       \
      return nanValue;                                                         \
    }                                                                          \
                                                                               \
    const univary vec3f clampedLocalCoordinates = clamp(                       \
        localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound); \
                                                                               \
    /* lower and upper corners of the box straddling the voxels to be          \
     interpolated. */                                                          \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);        \
    const univary vec3i voxelIndex_1 = voxelIndex_0 + 1;                       \
                                                                               \
    /* fractional coordinates within the lower corner voxel used during        \
     interpolation. */                                                         \
    const univary vec3f fractionalLocalCoordinates =                           \
        clampedLocalCoordinates - to_float(voxelIndex_0);                      \
                                                                               \
    /* look up the voxel values to be interpolated. */                         \
    univary float voxelValue_000;                                              \
    univary float voxelValue_001;                                              \
    univary float voxelValue_010;                                              \
    univary float voxelValue_011;                                              \
    univary float voxelValue_100;                                              \
    univary float voxelValue_101;                                              \
    univary float voxelValue_110;                                              \
    univary float voxelValue_111;                                              \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),            \
        attributeIndex,                                                        \
        voxelValue_000);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),            \
        attributeIndex,                                                        \
        voxelValue_001);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),            \
        attributeIndex,                                                        \
        voxelValue_010);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),            \
        attributeIndex,                                                        \
        voxelValue_011);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),            \
        attributeIndex,                                                        \
        voxelValue_100);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),            \
        attributeIndex,                                                        \
        voxelValue_101);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),            \
        attributeIndex,                                                        \
        voxelValue_110);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),            \
        attributeIndex,                                                        \
        voxelValue_111);                                                       \
                                                                               \
    /* interpolate the voxel values. */                                        \
    const univary float voxelValue_00 =                                        \
        voxelValue_000 +                                                       \
        fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);      \
    const univary float voxelValue_01 =                                        \
        voxelValue_010 +                                                       \
        fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);      \
    const univary float voxelValue_10 =                                        \
        voxelValue_100 +                                                       \
        fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);      \
    const univary float voxelValue_11 =                                        \
        voxelValue_110 +                                                       \
        fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);      \
    const univary float voxelValue_0 =                                         \
        voxelValue_00 +                                                        \
        fractionalLocalCoordinates.y * (voxelValue_01 - voxelValue_00);        \
    const univary float voxelValue_1 =                                         \
        voxelValue_10 +                                                        \
        fractionalLocalCoordinates.y * (voxelValue_11 - voxelValue_10);        \
                                                                               \
    return voxelValue_0 +                                                      \
           fractionalLocalCoordinates.z * (voxelValue_1 - voxelValue_0);       \
  }

template_sample_64(varying);
template_sample_64(uniform);
#undef template_sample_64

///////////////////////////////////////////////////////////////////////////////
// Gradient computation ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline varying vec3f SharedStructuredVolume_computeGradient_bbox_checks(
    const void *uniform _self,
    const varying vec3f &objectCoordinates,
    const uniform uint32 attributeIndex)
{
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)_self;

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

  float sample = self->computeSamples_varying[attributeIndex](
      self, objectCoordinates, attributeIndex);

  gradient.x = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   attributeIndex) -
               sample;
  gradient.y = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   attributeIndex) -
               sample;
  gradient.z = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   attributeIndex) -
               sample;

  return gradient / gradientStep;
}

inline varying vec3f SharedStructuredVolume_computeGradient_NaN_checks(
    const void *uniform _self,
    const varying vec3f &objectCoordinates,
    const uniform uint32 attributeIndex)
{
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)_self;

  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gridSpacing;

  // compute via forward or backward differences depending on volume boundary
  // (as determined by NaN sample values outside the boundary)
  const vec3f gradientExtent = objectCoordinates + gradientStep;

  vec3f gradient;

  float sample = self->computeSamples_varying[attributeIndex](
      self, objectCoordinates, attributeIndex);

  gradient.x = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   attributeIndex) -
               sample;
  gradient.y = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   attributeIndex) -
               sample;
  gradient.z = self->computeSamples_varying[attributeIndex](
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   attributeIndex) -
               sample;

  if (isnan(gradient.x)) {
    gradientStep.x *= -1.f;

    gradient.x = self->computeSamples_varying[attributeIndex](
                     self,
                     objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                     attributeIndex) -
                 sample;
  }

  if (isnan(gradient.y)) {
    gradientStep.y *= -1.f;

    gradient.y = self->computeSamples_varying[attributeIndex](
                     self,
                     objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                     attributeIndex) -
                 sample;
  }

  if (isnan(gradient.z)) {
    gradientStep.z *= -1.f;

    gradient.z = self->computeSamples_varying[attributeIndex](
                     self,
                     objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                     attributeIndex) -
                 sample;
  }

  return gradient / gradientStep;
}

///////////////////////////////////////////////////////////////////////////////
// Helper functions for handling multiple attributes //////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline void destructAttributesStorage(SharedStructuredVolume *uniform self)
{
  self->numAttributes = 0;

  if (self->attributesData) {
    delete[] self->attributesData;
    self->attributesData = NULL;
  }

  if (self->computeSamples_varying) {
    delete[] self->computeSamples_varying;
    self->computeSamples_varying = NULL;
  }

  if (self->getVoxels_varying) {
    delete[] self->getVoxels_varying;
    self->getVoxels_varying = NULL;
  }

  if (self->computeSamples_uniform) {
    delete[] self->computeSamples_uniform;
    self->computeSamples_uniform = NULL;
  }

  if (self->getVoxels_uniform) {
    delete[] self->getVoxels_uniform;
    self->getVoxels_uniform = NULL;
  }
}

inline void constructAttributesStorage(SharedStructuredVolume *uniform self,
                                       const uniform uint32 numAttributes)
{
  self->numAttributes  = numAttributes;
  self->attributesData = uniform new uniform Data1D[numAttributes];

  self->computeSamples_varying =
      uniform new uniform ComputeSampleVaryingFunc[numAttributes];
  self->getVoxels_varying =
      uniform new uniform GetVoxelVaryingFunc[numAttributes];

  self->computeSamples_uniform =
      uniform new uniform ComputeSampleUniformFunc[numAttributes];
  self->getVoxels_uniform =
      uniform new uniform GetVoxelUniformFunc[numAttributes];
}

// wrappers to enable setting old-style function pointers in the Volume
// superclass; restricts to first attribute
varying float computeSample_varying_0_legacy_wrapper(
    const void *uniform _self, const varying vec3f &objectCoordinates)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;
  return self->computeSamples_varying[0](_self, objectCoordinates, 0);
}

uniform float computeSample_uniform_0_legacy_wrapper(
    const void *uniform _self, const uniform vec3f &objectCoordinates)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;
  return self->computeSamples_uniform[0](_self, objectCoordinates, 0);
}

///////////////////////////////////////////////////////////////////////////////
// SharedStructuredVolume exported functions //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export uniform box3f EXPORT_UNIQUE(SharedStructuredVolume_getBoundingBox,
                                   void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  return self->boundingBox;
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_export,
                          uniform const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _samples)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = self->computeSamples_varying[attributeIndex](
        self, *objectCoordinates, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_uniform_export,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _sample)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  float *uniform sample = (float *uniform)_sample;

  *sample = self->computeSamples_uniform[attributeIndex](
      self, *objectCoordinates, attributeIndex);
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_N_export,
                          void *uniform _self,
                          const uniform uint32 N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          float *uniform samples)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    samples[i] =
        self->computeSamples_varying[attributeIndex](self, oc, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_export,
                          uniform const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _gradients)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;
    *gradients =
        self->computeGradient_varying(self, *objectCoordinates, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_N_export,
                          void *uniform _self,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          vec3f *uniform gradients)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    gradients[i]     = self->computeGradient_varying(self, oc, attributeIndex);
  }
}

export void *uniform EXPORT_UNIQUE(SharedStructuredVolume_Destructor,
                                   void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  destructAttributesStorage(self);

  if (self->accelerator) {
    GridAccelerator_Destructor(self->accelerator);
  }

  delete self;
}

export void *uniform EXPORT_UNIQUE(SharedStructuredVolume_Constructor)
{
  uniform SharedStructuredVolume *uniform self =
      uniform new uniform SharedStructuredVolume;

  self->attributesData         = NULL;
  self->accelerator            = NULL;
  self->computeSamples_varying = NULL;
  self->getVoxels_varying      = NULL;
  self->computeSamples_uniform = NULL;
  self->getVoxels_uniform      = NULL;

  return self;
}

export uniform bool EXPORT_UNIQUE(SharedStructuredVolume_set,
                                  void *uniform _self,
                                  const uniform uint32 numAttributes,
                                  const Data1D *uniform *uniform attributesData,
                                  const uniform vec3i &dimensions,
                                  const uniform SharedStructuredVolumeGridType
                                      gridType,
                                  const uniform vec3f &gridOrigin,
                                  const uniform vec3f &gridSpacing)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  destructAttributesStorage(self);
  constructAttributesStorage(self, numAttributes);

  for (uniform uint32 i = 0; i < numAttributes; i++) {
    self->attributesData[i] = *(attributesData[i]);
  }

  self->dimensions  = dimensions;
  self->gridType    = gridType;
  self->gridOrigin  = gridOrigin;
  self->gridSpacing = gridSpacing;

  if (self->gridType == structured_regular) {
    self->boundingBox = make_box3f(
        gridOrigin, gridOrigin + make_vec3f(dimensions - 1.f) * gridSpacing);

    self->transformLocalToObject_varying =
        transformLocalToObject_structured_regular;
    self->transformObjectToLocal_varying =
        transformObjectToLocal_structured_regular;
    self->transformLocalToObject_uniform =
        transformLocalToObjectUniform_structured_regular;
    self->transformObjectToLocal_uniform =
        transformObjectToLocalUniform_structured_regular;

    self->computeGradient_varying =
        SharedStructuredVolume_computeGradient_bbox_checks;

  } else if (self->gridType == structured_spherical) {
    computeStructuredSphericalBoundingBox(self, self->boundingBox);

    self->transformLocalToObject_varying =
        transformLocalToObject_varying_structured_spherical;
    self->transformObjectToLocal_varying =
        transformObjectToLocal_varying_structured_spherical;
    self->transformLocalToObject_uniform =
        transformLocalToObject_uniform_structured_spherical;
    self->transformObjectToLocal_uniform =
        transformObjectToLocal_uniform_structured_spherical;

    self->computeGradient_varying =
        SharedStructuredVolume_computeGradient_NaN_checks;
  } else {
    print("#vkl:shared_structured_volume: unknown gridType\n");
    return false;
  }

  self->localCoordinatesUpperBound =
      nextafter(self->dimensions - 1, make_vec3i(0));

  self->voxelOfs_dx = 1;
  self->voxelOfs_dy = dimensions.x;
  self->voxelOfs_dz = dimensions.x * dimensions.y;

  // default sampling function (64-bit addressing)
  for (uniform uint32 i = 0; i < numAttributes; i++) {
    self->computeSamples_varying[i] = SSV_sample_varying_64;
    self->computeSamples_uniform[i] = SSV_sample_uniform_64;

    if (safe_32bit_indexing(
            self->attributesData[i],
            dimensions.x * dimensions.y * (uint64)dimensions.z)) {
      // in this case, we know ALL addressing can be 32-bit.
      PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

      if (self->attributesData[i].dataType == VKL_UCHAR) {
        self->getVoxels_varying[i]      = SSV_getVoxel_uint8_varying_32;
        self->computeSamples_varying[i] = SSV_sample_uint8_varying_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_uint8_uniform_32;
        self->computeSamples_uniform[i] = SSV_sample_uint8_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_SHORT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_int16_varying_32;
        self->computeSamples_varying[i] = SSV_sample_int16_varying_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_int16_uniform_32;
        self->computeSamples_uniform[i] = SSV_sample_int16_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_USHORT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_uint16_varying_32;
        self->computeSamples_varying[i] = SSV_sample_uint16_varying_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_uint16_uniform_32;
        self->computeSamples_uniform[i] = SSV_sample_uint16_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_FLOAT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_float_varying_32;
        self->computeSamples_varying[i] = SSV_sample_float_varying_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_float_uniform_32;
        self->computeSamples_uniform[i] = SSV_sample_float_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
        self->getVoxels_varying[i]      = SSV_getVoxel_double_varying_32;
        self->computeSamples_varying[i] = SSV_sample_double_varying_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_double_uniform_32;
        self->computeSamples_uniform[i] = SSV_sample_double_uniform_32;
      } else {
        print("#vkl:shared_structured_volume: unknown voxelType\n");
        return false;
      }

    } else if (safe_32bit_indexing(self->attributesData[i],
                                   dimensions.x * (uint64)dimensions.y)) {
      // in this case, we know we can do 32-bit addressing within a
      // slice, but need 64-bit arithmetic to get slice begins
      PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

      if (self->attributesData[i].dataType == VKL_UCHAR) {
        self->getVoxels_varying[i]      = SSV_getVoxel_uint8_varying_64_32;
        self->computeSamples_varying[i] = SSV_sample_uint8_varying_64_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_uint8_uniform_64_32;
        self->computeSamples_uniform[i] = SSV_sample_uint8_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_SHORT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_int16_varying_64_32;
        self->computeSamples_varying[i] = SSV_sample_int16_varying_64_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_int16_uniform_64_32;
        self->computeSamples_uniform[i] = SSV_sample_int16_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_USHORT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_uint16_varying_64_32;
        self->computeSamples_varying[i] = SSV_sample_uint16_varying_64_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_uint16_uniform_64_32;
        self->computeSamples_uniform[i] = SSV_sample_uint16_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_FLOAT) {
        self->getVoxels_varying[i]      = SSV_getVoxel_float_varying_64_32;
        self->computeSamples_varying[i] = SSV_sample_float_varying_64_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_float_uniform_64_32;
        self->computeSamples_uniform[i] = SSV_sample_float_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
        self->getVoxels_varying[i]      = SSV_getVoxel_double_varying_64_32;
        self->computeSamples_varying[i] = SSV_sample_double_varying_64_32;
        self->getVoxels_uniform[i]      = SSV_getVoxel_double_uniform_64_32;
        self->computeSamples_uniform[i] = SSV_sample_double_uniform_64_32;
      } else {
        print("#vkl:shared_structured_volume: unknown voxelType\n");
        return false;
      }
    } else {
      // in this case, even a single slice is too big to do 32-bit
      // addressing, and we have to do 64-bit throughout
      PRINT_DEBUG("#vkl:shared_structured_volume: using 64-bit mode\n");

      if (self->attributesData[i].dataType == VKL_UCHAR) {
        self->getVoxels_varying[i] = SSV_getVoxel_uint8_varying_64;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint8_uniform_64;
      } else if (self->attributesData[i].dataType == VKL_SHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_int16_varying_64;
        self->getVoxels_uniform[i] = SSV_getVoxel_int16_uniform_64;
      } else if (self->attributesData[i].dataType == VKL_USHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_uint16_varying_64;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint16_uniform_64;
      } else if (self->attributesData[i].dataType == VKL_FLOAT) {
        self->getVoxels_varying[i] = SSV_getVoxel_float_varying_64;
        self->getVoxels_uniform[i] = SSV_getVoxel_float_uniform_64;
      } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
        self->getVoxels_varying[i] = SSV_getVoxel_double_varying_64;
        self->getVoxels_uniform[i] = SSV_getVoxel_double_uniform_64;
      } else {
        print("#vkl:shared_structured_volume: unknown voxelType\n");
        return false;
      }
    }
  }

  // TODO: temporary hack for Volume superclass functions used for iterators,
  // etc.; restricts to first attribute
  self->super.computeSample_varying = computeSample_varying_0_legacy_wrapper;
  self->super.computeSample_uniform = computeSample_uniform_0_legacy_wrapper;

  return true;
}

export void *uniform EXPORT_UNIQUE(SharedStructuredVolume_createAccelerator,
                                   void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  if (self->accelerator) {
    GridAccelerator_Destructor(self->accelerator);
  }

  self->accelerator = GridAccelerator_Constructor(self);

  return self->accelerator;
}
