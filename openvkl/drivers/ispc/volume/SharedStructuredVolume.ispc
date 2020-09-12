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
// Sampling functions /////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline uniform float nanValue()
{
  const uniform int NaN_bits = 0x7fc00000;
  return floatbits(NaN_bits);
}

inline uniform bool hasMotionBlurData (
    const SharedStructuredVolume *uniform self)
{
  // print("self->attributesTimeConfig->numItems: %\n", self->attributesTimeConfig->numItems);
  return self->attributesTimeConfig->numItems > 0;
}

inline uniform bool attributeHasStructuredTimeData (
    const SharedStructuredVolume *uniform self,
    const uniform uint32 attributeID)
{
  return  hasMotionBlurData(self) &&
          self->attributesTimeConfig[attributeID].numItems == 1 &&
          get_uint32(self->attributesTimeConfig[attributeID], (uint32)0) > 1;       // TODO attrTimeConfig needs 64 bit for large volumes
  }

inline uniform bool attributeHasUnstructuredTimeData (
    const SharedStructuredVolume *uniform self,
    const uniform uint32 attributeID)
{
  // print("valid(self->attributesTimeData[attributeID]): %\n", valid(self->attributesTimeData[attributeID]));
  return hasMotionBlurData(self) &&
         valid(self->attributesTimeData[attributeID]);
}

#define template_clampedLocalCoordinates(univary)                      \
  inline void clampedLocalCoordinates_##univary(                       \
      const SharedStructuredVolume *uniform self,                      \
      const univary vec3f &objectCoordinates,                          \
      univary vec3f &clampedLocalCoordinates,                          \
      univary bool &inBounds)                                          \
  {                                                                    \
    inBounds = true;                                                   \
                                                                       \
    self->transformObjectToLocal_##univary(                            \
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

#define template_computeTUVInterpolation64(univary)                           \
  inline void computeTUVInterpolation_##univary(                              \
      const uniform uint64 fixed0Ofs,                                         \
      const uniform uint64 fixed1Ofs,                                         \
      const univary uint64 timeOfs,                                           \
      const uniform Data1D &timesData,                                        \
      const uniform Data1D &timesConfig,                                      \
      const uniform float sampleTime,                                         \
      univary uint64 &lower,                                                  \
      univary uint64 &upper,                                                  \
      univary float &interpolant)                                             \
  {                                                                           \
    const univary uint64 voxelOfs =                                           \
        (uint64)get_uint32(timesConfig, fixed0Ofs+timeOfs); /*TODO timesConfig switch 32/64  */          \
    const univary uint64 nextOfs =                                            \
        (uint64)get_uint32(timesConfig, fixed1Ofs+timeOfs);                   \
    const univary uint64 numTimeSamples = nextOfs - voxelOfs;                 \
    assert(nextOfs>voxelOfs);\
    if (numTimeSamples == 2) {                                                \
      lower = voxelOfs;                                                       \
      upper = lower+1;                                                        \
      interpolant = sampleTime;                                               \
      return;                                                                 \
    }                                                                         \
    lower = 0;                                                                \
    upper = numTimeSamples;                                                   \
    while (lower <= upper) {                                                  \
      const univary uint64 mid = (lower + upper) / 2;                         \
      const univary float timesMid = get_float(timesData, voxelOfs+mid);      \
      if (sampleTime >= timesMid &&                                           \
          sampleTime <= get_float(timesData, voxelOfs+mid+1)) {               \
        lower = voxelOfs+mid;                                                 \
        upper = lower+1;                                                      \
        break;                                                                \
      } else if (sampleTime < timesMid) {                                     \
        upper = mid;                                                          \
      } else {                                                                \
        lower = mid;                                                          \
      }                                                                       \
    }                                                                         \
                                                                              \
    const univary float timesDataLow = get_float(timesData, lower);           \
    const univary float timesDataHigh = get_float(timesData, upper);          \
    interpolant = (sampleTime-timesDataLow) * rcp(timesDataHigh-timesDataLow);\
  }

template_computeTUVInterpolation64(varying);
template_computeTUVInterpolation64(uniform);
#undef template_computeTUVInterpolation64

#define template_computeTUVInterpolation32(univary)                           \
  inline void computeTUVInterpolation_##univary(                              \
      const uniform uint64 fixed0Ofs,                                         \
      const uniform uint64 fixed1Ofs,                                         \
      const univary uint32 timeOfs,                                           \
      const uniform Data1D &timesData,                                        \
      const uniform Data1D &timesConfig,                                      \
      const uniform float sampleTime,                                         \
      univary uint32 &lower,                                                  \
      univary uint32 &upper,                                                  \
      univary float &interpolant)                                             \
  {                                                                           \
    const univary uint32 voxelOfs =                                           \
        get_uint32(timesConfig, fixed0Ofs, timeOfs);                          \
    const univary uint32 nextOfs =                                            \
        get_uint32(timesConfig, fixed1Ofs, timeOfs);                          \
    if (nextOfs<=voxelOfs) {\
 print("fixed0Ofs: %\n",fixed0Ofs);\
    print("fixed1Ofs: %\n",fixed1Ofs);\
    print("time0Ofs: %\n",timeOfs);\
     print("voxelOfs: %\n",voxelOfs);\
     print("nextOfs: %\n",nextOfs);\
    }\
    assert(nextOfs>voxelOfs);\
    const univary uint32 numTimeSamples = nextOfs - voxelOfs;                 \
    if (numTimeSamples == 2) {                                                \
      lower = voxelOfs;                                                       \
      upper = lower+1;                                                        \
      interpolant = sampleTime;                                               \
      return;                                                                 \
    }                                                                         \
    lower = 0;                                                                \
    upper = numTimeSamples;                                                   \
    while (lower <= upper) {                                                  \
       /*print("lower: %\n",lower);*/\
       /*print("upper: %\n",upper);*/\
      const univary uint32 mid = (lower + upper) / 2;                         \
      /*print("mid: %\n",mid);*/\
      const univary float timesMid = get_float(timesData, voxelOfs+mid);      \
      /*print("timesMid: %\n",timesMid);*/\
      if (sampleTime >= timesMid &&                                           \
          sampleTime <= get_float(timesData, voxelOfs+mid+1)) {               \
        lower = voxelOfs+mid;                                                 \
        upper = lower+1;                                                      \
        break;                                                                \
      } else if (sampleTime < timesMid) {                                     \
        upper = mid;                                                          \
      } else {                                                                \
        lower = mid;                                                          \
      }                                                                       \
    }                                                                         \
                                                                              \
    const univary float timesDataLow = get_float(timesData, lower);           \
    const univary float timesDataHigh = get_float(timesData, upper);          \
    interpolant =(sampleTime-timesDataLow) * rcp(timesDataHigh-timesDataLow); \
  }

template_computeTUVInterpolation32(varying);
template_computeTUVInterpolation32(uniform);
#undef template_computeTUVInterpolation32

#define template_sample(univary)                                       \
  inline univary float SharedStructuredVolume_computeSample_##univary( \
      const SharedStructuredVolume *uniform self,                      \
      const univary vec3f &objectCoordinates,                          \
      const uniform VKLFilter filter,                                  \
      const uniform uint32 attributeIndex)                             \
  {                                                                    \
    univary vec3f clampedLocalCoordinates;                             \
                                                                       \
    /* computing clampedLocalCoordinates directly here rather than     \
     * using the above clampedLocalCoordinates_univary() function      \
     * produces more efficient code; clampedLocalCoordinates_univary() \
     * is still used for multi-attribute sampling functions... */      \
    self->transformObjectToLocal_##univary(                            \
        self, objectCoordinates, clampedLocalCoordinates);             \
                                                                       \
    if (clampedLocalCoordinates.x < 0.f ||                             \
        clampedLocalCoordinates.x > self->dimensions.x - 1.f ||        \
        clampedLocalCoordinates.y < 0.f ||                             \
        clampedLocalCoordinates.y > self->dimensions.y - 1.f ||        \
        clampedLocalCoordinates.z < 0.f ||                             \
        clampedLocalCoordinates.z > self->dimensions.z - 1.f) {        \
      return nanValue();                                               \
    }                                                                  \
                                                                       \
    clampedLocalCoordinates = clamp(clampedLocalCoordinates,           \
                                    make_vec3f(0.0f),                  \
                                    self->localCoordinatesUpperBound); \
                                                                       \
    return self->computeSamplesInner_##univary[attributeIndex](        \
        self, clampedLocalCoordinates, filter, attributeIndex);        \
  }

template_sample(varying);
template_sample(uniform);
#undef template_sample

#define template_sample_TUV(univary)                                       \
  inline univary float SharedStructuredVolume_computeSample_TUV_##univary( \
      const SharedStructuredVolume *uniform self,                          \
      const univary vec3f &objectCoordinates,                              \
      const uniform VKLFilter filter,                                      \
      const uniform uint32 attributeIndex,                                 \
      const uniform float sampleTime)                                      \
  {                                                                        \
    univary vec3f clampedLocalCoordinates;                                 \
    /* computing clampedLocalCoordinates directly here rather than         \
     * using the above clampedLocalCoordinates_univary() function          \
     * produces more efficient code; clampedLocalCoordinates_univary()     \
     * is still used for multi-attribute sampling functions... */          \
    self->transformObjectToLocal_##univary(                                \
        self, objectCoordinates, clampedLocalCoordinates);                 \
                                                                           \
    if (clampedLocalCoordinates.x < 0.f ||                                 \
        clampedLocalCoordinates.x > self->dimensions.x - 1.f ||            \
        clampedLocalCoordinates.y < 0.f ||                                 \
        clampedLocalCoordinates.y > self->dimensions.y - 1.f ||            \
        clampedLocalCoordinates.z < 0.f ||                                 \
        clampedLocalCoordinates.z > self->dimensions.z - 1.f) {            \
      return nanValue();                                                   \
    }                                                                      \
                                                                           \
    clampedLocalCoordinates = clamp(clampedLocalCoordinates,               \
                                    make_vec3f(0.0f),                      \
                                    self->localCoordinatesUpperBound);     \
                                                                           \
    /*mputeSample_TUV\n");*/ \
    return self->computeSamplesInner_TUV_##univary[attributeIndex](        \
        self, clampedLocalCoordinates, filter, attributeIndex, sampleTime);\
  }

template_sample_TUV(varying);
template_sample_TUV(uniform);
#undef template_sample_TUV

#define template_sample_MB(univary)                                       \
  inline univary float SharedStructuredVolume_computeSample_MB_##univary( \
      const SharedStructuredVolume *uniform self,                          \
      const univary vec3f &objectCoordinates,                              \
      const uniform VKLFilter filter,                                      \
      const uniform uint32 attributeIndex,                                 \
      const uniform float sampleTime)                                      \
  {                                                                        \
    univary vec3f clampedLocalCoordinates;                                 \
    /* computing clampedLocalCoordinates directly here rather than         \
     * using the above clampedLocalCoordinates_univary() function          \
     * produces more efficient code; clampedLocalCoordinates_univary()     \
     * is still used for multi-attribute sampling functions... */          \
    self->transformObjectToLocal_##univary(                                \
        self, objectCoordinates, clampedLocalCoordinates);                 \
                                                                           \
    if (clampedLocalCoordinates.x < 0.f ||                                 \
        clampedLocalCoordinates.x > self->dimensions.x - 1.f ||            \
        clampedLocalCoordinates.y < 0.f ||                                 \
        clampedLocalCoordinates.y > self->dimensions.y - 1.f ||            \
        clampedLocalCoordinates.z < 0.f ||                                 \
        clampedLocalCoordinates.z > self->dimensions.z - 1.f) {            \
      return nanValue();                                                   \
    }                                                                      \
                                                                           \
    clampedLocalCoordinates = clamp(clampedLocalCoordinates,               \
                                    make_vec3f(0.0f),                      \
                                    self->localCoordinatesUpperBound);     \
                                                                           \
    /*print("in computeSample_TUV\n");*/ \
    return self->computeSamplesInner_MB_##univary[attributeIndex](        \
        self, clampedLocalCoordinates, filter, attributeIndex, sampleTime);\
  }

template_sample_MB(varying);
template_sample_MB(uniform);
#undef template_sample_MB

///////////////////////////////////////////////////////////////////////////////
// Sampling inner functions for all addressing / voxel type combinations //////
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
#define template_sample_inner_32(type, univary)                              \
  inline univary float SSV_sample_inner_##type##_##univary##_32(             \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3f &clampedLocalCoordinates,                          \
      const uniform VKLFilter filter,                                        \
      const uniform uint32 attributeIndex)                                   \
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
      val = get_##type(voxelData, 0, voxelOfs);                              \
      break;                                                                 \
    }                                                                        \
    case VKL_FILTER_TRILINEAR: {                                             \
      /* fractional coordinates within the lower corner voxel used during    \
       * interpolation. */                                                   \
      const univary vec3f frac =                                             \
          clampedLocalCoordinates - to_float(voxelIndex_0);                  \
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

template_sample_inner_32(uint8, varying);
template_sample_inner_32(int16, varying);
template_sample_inner_32(uint16, varying);
template_sample_inner_32(float, varying);
template_sample_inner_32(double, varying);

template_sample_inner_32(uint8, uniform);
template_sample_inner_32(int16, uniform);
template_sample_inner_32(uint16, uniform);
template_sample_inner_32(float, uniform);
template_sample_inner_32(double, uniform);
#undef template_sample_inner_32

#define template_sample_inner_TUV_32(type, univary)                                 \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_32(                \
      const SharedStructuredVolume *uniform self,                                   \
      const univary vec3f &clampedLocalCoordinates,                                 \
      const uniform VKLFilter filter,                                               \
      const uniform uint32 attributeIndex,                                          \
      const uniform float sampleTime)                                               \
  {                                                                                 \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];          \
    const uniform Data1D timesData = self->attributesTimeData[attributeIndex];      \
    const uniform Data1D timesConfig = self->attributesTimeConfig[attributeIndex];  \
                                                                                    \
    /* lower corner of the box straddling the voxels to be interpolated. */         \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);             \
                                                                                    \
    const univary uint32 timeOfs = voxelIndex_0.x * self->voxelOfs_dx +             \
                                   voxelIndex_0.y * self->voxelOfs_dy +             \
                                   voxelIndex_0.z * self->voxelOfs_dz;              \
                                                                                    \
    univary uint32 lower;                                                           \
    univary uint32 upper;                                                           \
    univary float interpolant;                                                      \
    computeTUVInterpolation_##univary(                                              \
        0, self->voxelOfs_dx,                                                       \
        timeOfs, timesData, timesConfig, sampleTime,                                \
        lower, upper, interpolant);                                                 \
                                                                                    \
    univary float val = 0.f;                                                        \
    switch (filter) {                                                               \
      case VKL_FILTER_NEAREST: {                                                    \
        val = (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
        break;                                                                      \
      }                                                                             \
      case VKL_FILTER_TRILINEAR: {                                                  \
        /* fractional coordinates within the lower corner voxel used during         \
         * interpolation. */                                                        \
        const univary vec3f frac =                                                  \
            clampedLocalCoordinates - to_float(voxelIndex_0);                       \
        const uniform uint64 ofs000 = 0;                                            \
        const uniform uint64 ofs001 = self->voxelOfs_dx;                            \
        computeTUVInterpolation_##univary(                                          \
            ofs000, ofs000+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val000  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs001, ofs001+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val001  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val00   = val000 + frac.x * (val001 - val000);          \
                                                                                    \
        const uniform uint64 ofs010 = self->voxelOfs_dy;                            \
        const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;        \
        computeTUVInterpolation_##univary(                                          \
            ofs010, ofs010+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val010  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs011, ofs011+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val011  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val01   = val010 + frac.x * (val011 - val010);          \
                                                                                    \
        const uniform uint64 ofs100 = self->voxelOfs_dz;                            \
        const uniform uint64 ofs101 = ofs100 + ofs001;                              \
        computeTUVInterpolation_##univary(                                          \
            ofs100, ofs100+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val100  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs101, ofs101+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val101  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val10   = val100 + frac.x * (val101 - val100);          \
                                                                                    \
        const uniform uint64 ofs110 = ofs100 + ofs010;                              \
        const uniform uint64 ofs111 = ofs100 + ofs011;                              \
        computeTUVInterpolation_##univary(                                          \
            ofs110, ofs110+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val110  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs111, ofs111+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val111  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val11   = val110 + frac.x * (val111 - val110);          \
                                                                                    \
        const univary float val0 = val00 + frac.y * (val01 - val00);                \
        const univary float val1 = val10 + frac.y * (val11 - val10);                \
        val = val0 + frac.z * (val1 - val0);                                        \
        break;                                                                      \
      }                                                                             \
    }                                                                               \
                                                                                    \
    return val;                                                                     \
  }

template_sample_inner_TUV_32(uint8, varying);
template_sample_inner_TUV_32(int16, varying);
template_sample_inner_TUV_32(uint16, varying);
template_sample_inner_TUV_32(float, varying);
template_sample_inner_TUV_32(double, varying);

template_sample_inner_TUV_32(uint8, uniform);
template_sample_inner_TUV_32(int16, uniform);
template_sample_inner_TUV_32(uint16, uniform);
template_sample_inner_TUV_32(float, uniform);
template_sample_inner_TUV_32(double, uniform);
#undef template_sample_inner_TUV_32

#define template_sample_inner_MB_32(type, univary)                                \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_32(               \
      const SharedStructuredVolume *uniform self,                                 \
      const univary vec3f &clampedLocalCoordinates,                               \
      const uniform VKLFilter filter,                                             \
      const uniform uint32 attributeIndex,                                        \
      const uniform float sampleTime)                                             \
  {                                                                               \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];        \
    const uniform Data1D timesConfig = self->attributesTimeConfig[attributeIndex];\
    const uniform uint32 numTimeSamples = get_uint32(timesConfig, 0, 0);          \
                                                                                  \
    /* lower corner of the box straddling the voxels to be interpolated. */       \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);           \
    const uniform uint64 voxelOfs_dx = numTimeSamples * self->voxelOfs_dx;        \
    const uniform uint64 voxelOfs_dy = numTimeSamples * self->voxelOfs_dy;        \
    const uniform uint64 voxelOfs_dz = numTimeSamples * self->voxelOfs_dz;        \
    const univary uint32 voxelOfs0 = voxelIndex_0.x * voxelOfs_dx +               \
                                     voxelIndex_0.y * voxelOfs_dy +               \
                                     voxelIndex_0.z * voxelOfs_dz;                \
                                                                                  \
    const univary uint32 localVoxelOfs = sampleTime * (numTimeSamples-1);         \
    const univary float interpolant = sampleTime * (float)(numTimeSamples-1)      \
                                      - (float)localVoxelOfs;                     \
    const univary uint32 voxelOfs = voxelOfs0 + localVoxelOfs;                    \
    const univary uint32 nextOfs = voxelOfs + 1;                                  \
                                                                                  \
    univary float val = 0.f;                                                      \
    switch (filter) {                                                             \
      case VKL_FILTER_NEAREST: {                                                  \
        val = (1.f-interpolant) * get_##type(voxelData, 0, voxelOfs) +            \
              interpolant * get_##type(voxelData, 0, nextOfs);                    \
        break;                                                                    \
      }                                                                           \
      case VKL_FILTER_TRILINEAR: {                                                \
        /* fractional coordinates within the lower corner voxel used during       \
         * interpolation. */                                                      \
        const univary vec3f frac =                                                \
            clampedLocalCoordinates - to_float(voxelIndex_0);                     \
        const uniform uint64 ofs000 = 0;                                          \
        const uniform uint64 ofs001 = voxelOfs_dx;                                \
        const univary float val000  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs000, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs000, nextOfs);                 \
        const univary float val001  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs001, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs001, nextOfs);                 \
        const univary float val00   = val000 + frac.x * (val001 - val000);        \
                                                                                  \
        const uniform uint64 ofs010 = voxelOfs_dy;                                \
        const uniform uint64 ofs011 = voxelOfs_dy +                               \
                                      voxelOfs_dx;                                \
        const univary float val010  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs010, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs010, nextOfs);                 \
        const univary float val011  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs011, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs011, nextOfs);                 \
        const univary float val01   = val010 + frac.x * (val011 - val010);        \
                                                                                  \
        const uniform uint64 ofs100 = voxelOfs_dz;                                \
        const uniform uint64 ofs101 = ofs100 + ofs001;                            \
        const univary float val100  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs100, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs100, nextOfs);                 \
        const univary float val101  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs101, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs101, nextOfs);                 \
        const univary float val10   = val100 + frac.x * (val101 - val100);        \
                                                                                  \
        const uniform uint64 ofs110 = ofs100 + ofs010;                            \
        const uniform uint64 ofs111 = ofs100 + ofs011;                            \
        const univary float val110  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs110, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs110, nextOfs);                 \
        const univary float val111  =                                             \
            (1.f-interpolant) * get_##type(voxelData, ofs111, voxelOfs) +         \
            interpolant * get_##type(voxelData, ofs111, nextOfs);                 \
        const univary float val11   = val110 + frac.x * (val111 - val110);        \
                                                                                  \
        const univary float val0 = val00 + frac.y * (val01 - val00);              \
        const univary float val1 = val10 + frac.y * (val11 - val10);              \
        val = val0 + frac.z * (val1 - val0);                                      \
        break;                                                                    \
      }                                                                           \
    }                                                                             \
                                                                                  \
    return val;                                                                   \
  }

template_sample_inner_MB_32(uint8, varying);
template_sample_inner_MB_32(int16, varying);
template_sample_inner_MB_32(uint16, varying);
template_sample_inner_MB_32(float, varying);
template_sample_inner_MB_32(double, varying);

template_sample_inner_MB_32(uint8, uniform);
template_sample_inner_MB_32(int16, uniform);
template_sample_inner_MB_32(uint16, uniform);
template_sample_inner_MB_32(float, uniform);
template_sample_inner_MB_32(double, uniform);
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
      const uniform uint32 attributeIndex)                                   \
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
      case VKL_FILTER_TRILINEAR: {                                           \
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

template_sample_inner_64_32(uint8, varying);
template_sample_inner_64_32(int16, varying);
template_sample_inner_64_32(uint16, varying);
template_sample_inner_64_32(float, varying);
template_sample_inner_64_32(double, varying);

template_sample_inner_64_32(uint8, uniform);
template_sample_inner_64_32(int16, uniform);
template_sample_inner_64_32(uint16, uniform);
template_sample_inner_64_32(float, uniform);
template_sample_inner_64_32(double, uniform);
#undef template_sample_inner_64_32

#define template_sample_inner_TUV_64_32(type, univary)                              \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_64_32(             \
      const SharedStructuredVolume *uniform self,                                   \
      const univary vec3f &clampedLocalCoordinates,                                 \
      const uniform VKLFilter filter,                                               \
      const uniform uint32 attributeIndex,                                          \
      const uniform float sampleTime)                                               \
  {                                                                                 \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];          \
    const uniform Data1D timesData = self->attributesTimeData[attributeIndex];      \
    const uniform Data1D timesConfig = self->attributesTimeConfig[attributeIndex];  \
                                                                                    \
    /* lower corner of the box straddling the voxels to be interpolated. */         \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);             \
                                                                                    \
                                                                                    \
    univary float val = 0.f;                                                        \
    process_sliceID(univary)                                                        \
    {                                                                               \
      const uniform uint64 sliceOfs =                                               \
          (uint64)sliceID * self->dimensions.x * self->dimensions.y;                \
      const univary uint32 timeOfs = voxelIndex_0.x * self->voxelOfs_dx +           \
                                    voxelIndex_0.y * self->voxelOfs_dy;             \
      univary uint32 lower;                                                         \
      univary uint32 upper;                                                         \
      univary float interpolant;                                                    \
      computeTUVInterpolation_##univary(                                            \
          0, self->voxelOfs_dx,                                                     \
          timeOfs, timesData, timesConfig, sampleTime,                              \
          lower, upper, interpolant);                                               \
                                                                                    \
                                                                                    \
      switch (filter) {                                                             \
        case VKL_FILTER_NEAREST: {                                                  \
          val = (1.f-interpolant) * get_##type(voxelData, sliceOfs, lower)          \
                 + interpolant * get_##type(voxelData, sliceOfs, upper);            \
          break;                                                                    \
        }                                                                           \
        case VKL_FILTER_TRILINEAR: {                                                \
          /* fractional coordinates within the lower corner voxel used during       \
          * interpolation. */                                                       \
          const univary vec3f frac =                                                \
              clampedLocalCoordinates - to_float(voxelIndex_0);                     \
          const uniform uint64 ofs000 = 0;                                          \
          const uniform uint64 ofs001 = self->voxelOfs_dx;                          \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs000, sliceOfs+ofs000+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val000  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs001, sliceOfs+ofs001+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val001  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          const univary float val00   = val000 + frac.x * (val001 - val000);        \
                                                                                    \
          const uniform uint64 ofs010 = self->voxelOfs_dy;                          \
          const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;      \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs010, sliceOfs+ofs010+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val010  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs011, sliceOfs+ofs011+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val011  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          const univary float val01   = val010 + frac.x * (val011 - val010);        \
                                                                                    \
          const uniform uint64 ofs100 = self->voxelOfs_dz;                          \
          const uniform uint64 ofs101 = ofs100 + ofs001;                            \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs100, sliceOfs+ofs100+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val100  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs101, sliceOfs+ofs101+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val101  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          const univary float val10   = val100 + frac.x * (val101 - val100);        \
                                                                                    \
          const uniform uint64 ofs110 = ofs100 + ofs010;                            \
          const uniform uint64 ofs111 = ofs100 + ofs011;                            \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs110, sliceOfs+ofs110+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val110  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          computeTUVInterpolation_##univary(                                        \
              sliceOfs+ofs111, sliceOfs+ofs111+self->voxelOfs_dx,                   \
              timeOfs, timesData, timesConfig, sampleTime,                          \
              lower, upper, interpolant);                                           \
          const univary float val111  =                                             \
              (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
          const univary float val11   = val110 + frac.x * (val111 - val110);        \
                                                                                    \
          const univary float val0 = val00 + frac.y * (val01 - val00);              \
          const univary float val1 = val10 + frac.y * (val11 - val10);              \
          val = val0 + frac.z * (val1 - val0);                                      \
          break;                                                                    \
        }                                                                           \
      }                                                                             \
    }                                                                               \
                                                                                    \
    return val;                                                                     \
  }

template_sample_inner_TUV_64_32(uint8, varying);
template_sample_inner_TUV_64_32(int16, varying);
template_sample_inner_TUV_64_32(uint16, varying);
template_sample_inner_TUV_64_32(float, varying);
template_sample_inner_TUV_64_32(double, varying);

template_sample_inner_TUV_64_32(uint8, uniform);
template_sample_inner_TUV_64_32(int16, uniform);
template_sample_inner_TUV_64_32(uint16, uniform);
template_sample_inner_TUV_64_32(float, uniform);
template_sample_inner_TUV_64_32(double, uniform);
#undef template_sample_inner_TUV_64_32

#define template_sample_inner_MB_64_32(type, univary)                               \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_64_32(              \
      const SharedStructuredVolume *uniform self,                                   \
      const univary vec3f &clampedLocalCoordinates,                                 \
      const uniform VKLFilter filter,                                               \
      const uniform uint32 attributeIndex,                                          \
      const uniform float sampleTime)                                               \
  {                                                                                 \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];          \
    const uniform Data1D timesConfig = self->attributesTimeConfig[attributeIndex];  \
    const uniform uint32 numTimeSamples = get_uint32(timesConfig, 0, 0);            \
                                                                                    \
    /* lower corner of the box straddling the voxels to be interpolated. */         \
      const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);           \
                                                                                    \
    univary float val = 0.f;                                                        \
    process_sliceID(univary)                                                        \
    {                                                                               \
      const uniform uint64 sliceOfs =                                               \
          (uint64)sliceID * numTimeSamples * numTimeSamples *                       \
          self->dimensions.x * self->dimensions.y;                                  \
      const uniform uint64 voxelOfs_dx = numTimeSamples * self->voxelOfs_dx;        \
      const uniform uint64 voxelOfs_dy = numTimeSamples * self->voxelOfs_dy;        \
      const uniform uint64 voxelOfs_dz = numTimeSamples * self->voxelOfs_dz;        \
      const univary uint32 voxelOfs0 = voxelIndex_0.x * voxelOfs_dx +               \
                                      voxelIndex_0.y * voxelOfs_dy;                 \
                                                                                    \
      const univary uint32 localVoxelOfs = sampleTime * (numTimeSamples-1);         \
      const univary float interpolant = sampleTime * (float)(numTimeSamples-1)      \
                                        - (float)localVoxelOfs;                     \
      const univary uint32 voxelOfs = sliceOfs + voxelOfs0 + localVoxelOfs;         \
      const univary uint32 nextOfs = voxelOfs + 1;                                  \
                                                                                    \
      switch (filter) {                                                             \
        case VKL_FILTER_NEAREST: {                                                  \
          val = (1.f-interpolant) * get_##type(voxelData, sliceOfs, voxelOfs) +     \
                interpolant * get_##type(voxelData, sliceOfs, nextOfs);             \
          break;                                                                    \
        }                                                                           \
        case VKL_FILTER_TRILINEAR: {                                                \
          /* fractional coordinates within the lower corner voxel used during       \
          * interpolation. */                                                       \
          const univary vec3f frac =                                                \
              clampedLocalCoordinates - to_float(voxelIndex_0);                     \
          const uniform uint64 ofs000 = 0;                                          \
          const uniform uint64 ofs001 = voxelOfs_dx;                                \
          const univary float val000  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs000, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs000, nextOfs);    \
          const univary float val001  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs001, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs001, nextOfs);    \
          const univary float val00   = val000 + frac.x * (val001 - val000);        \
                                                                                    \
          const uniform uint64 ofs010 = voxelOfs_dy;                                \
          const uniform uint64 ofs011 = voxelOfs_dy +                               \
                                        voxelOfs_dx;                                \
          const univary float val010  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs010, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs010, nextOfs);    \
          const univary float val011  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs011, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs011, nextOfs);    \
          const univary float val01   = val010 + frac.x * (val011 - val010);        \
                                                                                    \
          const uniform uint64 ofs100 = voxelOfs_dz;                                \
          const uniform uint64 ofs101 = ofs100 + ofs001;                            \
          const univary float val100  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs100, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs100, nextOfs);    \
          const univary float val101  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs101, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs101, nextOfs);    \
          const univary float val10   = val100 + frac.x * (val101 - val100);        \
                                                                                    \
          const uniform uint64 ofs110 = ofs100 + ofs010;                            \
          const uniform uint64 ofs111 = ofs100 + ofs011;                            \
          const univary float val110  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs110, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs110, nextOfs);    \
          const univary float val111  =                                             \
              (1.f-interpolant) * get_##type(voxelData, sliceOfs + ofs111, voxelOfs)\
              + interpolant * get_##type(voxelData, sliceOfs + ofs111, nextOfs);    \
          const univary float val11   = val110 + frac.x * (val111 - val110);        \
                                                                                    \
          const univary float val0 = val00 + frac.y * (val01 - val00);              \
          const univary float val1 = val10 + frac.y * (val11 - val10);              \
          val = val0 + frac.z * (val1 - val0);                                      \
          break;                                                                    \
        }                                                                           \
      }                                                                             \
    }                                                                               \
                                                                                    \
    return val;                                                                     \
  }

template_sample_inner_MB_64_32(uint8, varying);
template_sample_inner_MB_64_32(int16, varying);
template_sample_inner_MB_64_32(uint16, varying);
template_sample_inner_MB_64_32(float, varying);
template_sample_inner_MB_64_32(double, varying);

template_sample_inner_MB_64_32(uint8, uniform);
template_sample_inner_MB_64_32(int16, uniform);
template_sample_inner_MB_64_32(uint16, uniform);
template_sample_inner_MB_64_32(float, uniform);
template_sample_inner_MB_64_32(double, uniform);
#undef template_sample_inner_MB_64_32

// default sampling function (64-bit addressing)
#define template_sample_inner_64(univary)                                   \
  inline univary float SSV_sample_inner_##univary##_64(                     \
      const SharedStructuredVolume *uniform self,                           \
      const univary vec3f &clampedLocalCoordinates,                         \
      const uniform VKLFilter filter,                                       \
      const uniform uint32 attributeIndex)                                  \
  {                                                                         \
    /* lower and upper corners of the box straddling the voxels to be       \
     interpolated. */                                                       \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);     \
    const univary vec3i voxelIndex_1 = voxelIndex_0 + 1;                    \
                                                                            \
    univary float val = 0.f;                                                \
    switch (filter) {                                                       \
    case VKL_FILTER_NEAREST: {                                              \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),       \
          attributeIndex,                                                   \
          val);                                                             \
      break;                                                                \
    }                                                                       \
    case VKL_FILTER_TRILINEAR: {                                            \
      /* fractional coordinates within the lower corner voxel used during   \
       interpolation. */                                                    \
      const univary vec3f fractionalLocalCoordinates =                      \
          clampedLocalCoordinates - to_float(voxelIndex_0);                 \
                                                                            \
      /* look up the voxel values to be interpolated. */                    \
      univary float voxelValue_000;                                         \
      univary float voxelValue_001;                                         \
      univary float voxelValue_010;                                         \
      univary float voxelValue_011;                                         \
      univary float voxelValue_100;                                         \
      univary float voxelValue_101;                                         \
      univary float voxelValue_110;                                         \
      univary float voxelValue_111;                                         \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),       \
          attributeIndex,                                                   \
          voxelValue_000);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),       \
          attributeIndex,                                                   \
          voxelValue_001);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),       \
          attributeIndex,                                                   \
          voxelValue_010);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),       \
          attributeIndex,                                                   \
          voxelValue_011);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),       \
          attributeIndex,                                                   \
          voxelValue_100);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),       \
          attributeIndex,                                                   \
          voxelValue_101);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),       \
          attributeIndex,                                                   \
          voxelValue_110);                                                  \
      getVoxelUnivary(                                                      \
          self,                                                             \
          make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),       \
          attributeIndex,                                                   \
          voxelValue_111);                                                  \
                                                                            \
      /* interpolate the voxel values. */                                   \
      const univary float voxelValue_00 =                                   \
          voxelValue_000 +                                                  \
          fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000); \
      const univary float voxelValue_01 =                                   \
          voxelValue_010 +                                                  \
          fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010); \
      const univary float voxelValue_10 =                                   \
          voxelValue_100 +                                                  \
          fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100); \
      const univary float voxelValue_11 =                                   \
          voxelValue_110 +                                                  \
          fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110); \
      const univary float voxelValue_0 =                                    \
          voxelValue_00 +                                                   \
          fractionalLocalCoordinates.y * (voxelValue_01 - voxelValue_00);   \
      const univary float voxelValue_1 =                                    \
          voxelValue_10 +                                                   \
          fractionalLocalCoordinates.y * (voxelValue_11 - voxelValue_10);   \
                                                                            \
      val = voxelValue_0 +                                                  \
            fractionalLocalCoordinates.z * (voxelValue_1 - voxelValue_0);   \
      break;                                                                \
    }                                                                       \
    }                                                                       \
    return val;                                                             \
  }

template_sample_inner_64(varying);
template_sample_inner_64(uniform);
#undef template_sample_inner_64

#define template_sample_inner_TUV_64(type, univary)                                 \
  inline univary float SSV_sample_inner_TUV_##type##_##univary##_64(                \
      const SharedStructuredVolume *uniform self,                                   \
      const univary vec3f &clampedLocalCoordinates,                                 \
      const uniform VKLFilter filter,                                               \
      const uniform uint32 attributeIndex,                                          \
      const uniform float sampleTime)                                               \
  {                                                                                 \
    const uniform Data1D voxelData = self->attributesData[attributeIndex];          \
    const uniform Data1D timesData = self->attributesTimeData[attributeIndex];      \
    const uniform Data1D timesConfig = self->attributesTimeConfig[attributeIndex];  \
                                                                                    \
    /* lower corner of the box straddling the voxels to be interpolated. */         \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);             \
                                                                                    \
    const univary uint64 timeOfs = (uint64)voxelIndex_0.x * self->voxelOfs_dx +     \
                                   (uint64)voxelIndex_0.y * self->voxelOfs_dy +     \
                                   (uint64)voxelIndex_0.z * self->voxelOfs_dz;      \
                                                                                    \
    univary uint64 lower;                                                           \
    univary uint64 upper;                                                           \
    univary float interpolant;                                                      \
    computeTUVInterpolation_##univary(                                              \
        0, (uint64)self->voxelOfs_dx,                                               \
        timeOfs, timesData, timesConfig, sampleTime,                                \
        lower, upper, interpolant);                                                 \
                                                                                    \
    univary float val = 0.f;                                                        \
    switch (filter) {                                                               \
      case VKL_FILTER_NEAREST: {                                                    \
        val = (1.f-interpolant) * get_##type(voxelData, lower) +                    \
              interpolant * get_##type(voxelData, upper);                           \
        break;                                                                      \
      }                                                                             \
      case VKL_FILTER_TRILINEAR: {                                                  \
        /* fractional coordinates within the lower corner voxel used during         \
         * interpolation. */                                                        \
        const univary vec3f frac =                                                  \
            clampedLocalCoordinates - to_float(voxelIndex_0);                       \
        const uniform uint64 ofs000 = 0;                                            \
        const uniform uint64 ofs001 = self->voxelOfs_dx;                            \
        computeTUVInterpolation_##univary(                                          \
            ofs000, ofs000+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val000  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs001, ofs001+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val001  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val00   = val000 + frac.x * (val001 - val000);          \
                                                                                    \
        const uniform uint64 ofs010 = self->voxelOfs_dy;                            \
        const uniform uint64 ofs011 = self->voxelOfs_dy + self->voxelOfs_dx;        \
        computeTUVInterpolation_##univary(                                          \
            ofs010, ofs010+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val010  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs011, ofs011+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val011  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val01   = val010 + frac.x * (val011 - val010);          \
                                                                                    \
        const uniform uint64 ofs100 = self->voxelOfs_dz;                            \
        const uniform uint64 ofs101 = ofs100 + ofs001;                              \
        computeTUVInterpolation_##univary(                                          \
            ofs100, ofs100+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val100  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs101, ofs101+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val101  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val10   = val100 + frac.x * (val101 - val100);          \
                                                                                    \
        const uniform uint64 ofs110 = ofs100 + ofs010;                              \
        const uniform uint64 ofs111 = ofs100 + ofs011;                              \
        computeTUVInterpolation_##univary(                                          \
            ofs110, ofs110+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val110  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        computeTUVInterpolation_##univary(                                          \
            ofs111, ofs111+self->voxelOfs_dx,                                       \
            timeOfs, timesData, timesConfig, sampleTime,                            \
            lower, upper, interpolant);                                             \
        const univary float val111  =                                               \
            (1.f-interpolant) * get_##type(voxelData, lower) +                      \
            interpolant * get_##type(voxelData, upper);                             \
        const univary float val11   = val110 + frac.x * (val111 - val110);          \
                                                                                    \
        const univary float val0 = val00 + frac.y * (val01 - val00);                \
        const univary float val1 = val10 + frac.y * (val11 - val10);                \
        val = val0 + frac.z * (val1 - val0);                                        \
        break;                                                                      \
      }                                                                             \
    }                                                                               \
                                                                                    \
    return val;                                                                     \
  }

template_sample_inner_TUV_64(uint8, varying);
template_sample_inner_TUV_64(int16, varying);
template_sample_inner_TUV_64(uint16, varying);
template_sample_inner_TUV_64(float, varying);
template_sample_inner_TUV_64(double, varying);

template_sample_inner_TUV_64(uint8, uniform);
template_sample_inner_TUV_64(int16, uniform);
template_sample_inner_TUV_64(uint16, uniform);
template_sample_inner_TUV_64(float, uniform);
template_sample_inner_TUV_64(double, uniform);
#undef template_sample_inner_TUV_64

#define template_sample_inner_MB_64(type, univary)                             \
  inline univary float SSV_sample_inner_MB_##type##_##univary##_64(            \
      const SharedStructuredVolume *uniform self,                              \
      const univary vec3f &clampedLocalCoordinates,                            \
      const uniform VKLFilter filter,                                          \
      const uniform uint32 attributeIndex,                                     \
      const uniform float sampleTime)                                          \
  {                                                                            \
    /* lower and upper corners of the box straddling the voxels to be          \
     interpolated. */                                                          \
    const univary vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);        \
    const univary vec3i voxelIndex_1 = voxelIndex_0 + 1;                       \
                                                                               \
    univary float val = 0.f;                                                   \
    switch (filter) {                                                          \
      case VKL_FILTER_NEAREST: {                                               \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),        \
            attributeIndex,                                                    \
            val);                                                              \
        break;                                                                 \
      }                                                                        \
      case VKL_FILTER_TRILINEAR: {                                             \
        /* fractional coordinates within the lower corner voxel used during    \
         interpolation. */                                                     \
        const univary vec3f fractionalLocalCoordinates =                       \
            clampedLocalCoordinates - to_float(voxelIndex_0);                  \
                                                                               \
        /* look up the voxel values to be interpolated. */                     \
        univary float voxelValue_000;                                          \
        univary float voxelValue_001;                                          \
        univary float voxelValue_010;                                          \
        univary float voxelValue_011;                                          \
        univary float voxelValue_100;                                          \
        univary float voxelValue_101;                                          \
        univary float voxelValue_110;                                          \
        univary float voxelValue_111;                                          \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),        \
            attributeIndex,                                                    \
            voxelValue_000);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),        \
            attributeIndex,                                                    \
            voxelValue_001);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),        \
            attributeIndex,                                                    \
            voxelValue_010);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),        \
            attributeIndex,                                                    \
            voxelValue_011);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),        \
            attributeIndex,                                                    \
            voxelValue_100);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),        \
            attributeIndex,                                                    \
            voxelValue_101);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),        \
            attributeIndex,                                                    \
            voxelValue_110);                                                   \
        getVoxelUnivary(                                                       \
            self,                                                              \
            make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),        \
            attributeIndex,                                                    \
            voxelValue_111);                                                   \
                                                                               \
        /* interpolate the voxel values. */                                    \
        const univary float voxelValue_00 =                                    \
            voxelValue_000 +                                                   \
            fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);  \
        const univary float voxelValue_01 =                                    \
            voxelValue_010 +                                                   \
            fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);  \
        const univary float voxelValue_10 =                                    \
            voxelValue_100 +                                                   \
            fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);  \
        const univary float voxelValue_11 =                                    \
            voxelValue_110 +                                                   \
            fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);  \
        const univary float voxelValue_0 =                                     \
            voxelValue_00 +                                                    \
            fractionalLocalCoordinates.y * (voxelValue_01 - voxelValue_00);    \
        const univary float voxelValue_1 =                                     \
            voxelValue_10 +                                                    \
            fractionalLocalCoordinates.y * (voxelValue_11 - voxelValue_10);    \
                                                                               \
        val = voxelValue_0 +                                                   \
               fractionalLocalCoordinates.z * (voxelValue_1 - voxelValue_0);   \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    return val;                                                                \
  }

template_sample_inner_MB_64(uint8, varying);
template_sample_inner_MB_64(int16, varying);
template_sample_inner_MB_64(uint16, varying);
template_sample_inner_MB_64(float, varying);
template_sample_inner_MB_64(double, varying);

template_sample_inner_MB_64(uint8, uniform);
template_sample_inner_MB_64(int16, uniform);
template_sample_inner_MB_64(uint16, uniform);
template_sample_inner_MB_64(float, uniform);
template_sample_inner_MB_64(double, uniform);
#undef template_sample_inner_MB_64

///////////////////////////////////////////////////////////////////////////////
// Gradient computation ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline varying vec3f SharedStructuredVolume_computeGradient_bbox_checks(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex)
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
      self, objectCoordinates, filter, attributeIndex);

  gradient.x = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   filter,
                   attributeIndex) -
               sample;
  gradient.y = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   filter,
                   attributeIndex) -
               sample;
  gradient.z = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   filter,
                   attributeIndex) -
               sample;

  return gradient / gradientStep;
}

inline varying vec3f SharedStructuredVolume_computeGradient_NaN_checks(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    const uniform VKLFilter filter,
    const uniform uint32 attributeIndex)
{
  // gradient step in each dimension (object coordinates)
  vec3f gradientStep = self->gridSpacing;

  // compute via forward or backward differences depending on volume boundary
  // (as determined by NaN sample values outside the boundary)
  const vec3f gradientExtent = objectCoordinates + gradientStep;

  vec3f gradient;

  float sample = SharedStructuredVolume_computeSample_varying(
      self, objectCoordinates, filter, attributeIndex);

  gradient.x = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                   filter,
                   attributeIndex) -
               sample;
  gradient.y = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                   filter,
                   attributeIndex) -
               sample;
  gradient.z = SharedStructuredVolume_computeSample_varying(
                   self,
                   objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                   VKL_FILTER_TRILINEAR,
                   attributeIndex) -
               sample;

  if (isnan(gradient.x)) {
    gradientStep.x *= -1.f;

    gradient.x = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f),
                     filter,
                     attributeIndex) -
                 sample;
  }

  if (isnan(gradient.y)) {
    gradientStep.y *= -1.f;

    gradient.y = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f),
                     filter,
                     attributeIndex) -
                 sample;
  }

  if (isnan(gradient.z)) {
    gradientStep.z *= -1.f;

    gradient.z = SharedStructuredVolume_computeSample_varying(
                     self,
                     objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z),
                     filter,
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

  if (self->attributesTimeData) {
    delete[] self->attributesTimeData;
    self->attributesTimeData = NULL;
  }

  if (self->attributesTimeConfig) {
    delete[] self->attributesTimeConfig;
    self->attributesTimeConfig = NULL;
  }

  if (self->computeSamplesInner_varying) {
    delete[] self->computeSamplesInner_varying;
    self->computeSamplesInner_varying = NULL;
  }

  if (self->getVoxels_varying) {
    delete[] self->getVoxels_varying;
    self->getVoxels_varying = NULL;
  }

  if (self->computeSamplesInner_uniform) {
    delete[] self->computeSamplesInner_uniform;
    self->computeSamplesInner_uniform = NULL;
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
  self->attributesTimeData = uniform new uniform Data1D[numAttributes];
  self->attributesTimeConfig = uniform new uniform Data1D[numAttributes];

  self->computeSamplesInner_varying =
      uniform new uniform ComputeSampleInnerVaryingFunc[numAttributes];
  self->computeSamplesInner_TUV_varying =
      uniform new uniform ComputeSampleInnerTimeVaryingFunc[numAttributes];
  self->computeSamplesInner_MB_varying =
      uniform new uniform ComputeSampleInnerTimeVaryingFunc[numAttributes];
  self->getVoxels_varying =
      uniform new uniform GetVoxelVaryingFunc[numAttributes];

  self->computeSamplesInner_uniform =
      uniform new uniform ComputeSampleInnerUniformFunc[numAttributes];
  self->computeSamplesInner_TUV_uniform =
      uniform new uniform ComputeSampleInnerTimeUniformFunc[numAttributes];
  self->computeSamplesInner_MB_uniform =
      uniform new uniform ComputeSampleInnerTimeUniformFunc[numAttributes];
  self->getVoxels_uniform =
      uniform new uniform GetVoxelUniformFunc[numAttributes];
}

// wrappers to enable setting old-style function pointers in the Volume
// superclass; restricts to first attribute
varying float computeSample_varying_0_iterator_wrapper(
    const Sampler *uniform sampler, const varying vec3f &objectCoordinates)
{
  return SharedStructuredVolume_computeSample_varying(
      (const SharedStructuredVolume *uniform)sampler->volume,
      objectCoordinates,
      sampler->filter,
      0);
}

uniform float computeSample_uniform_0_iterator_wrapper(
    const Sampler *uniform sampler, const uniform vec3f &objectCoordinates)
{
  return SharedStructuredVolume_computeSample_uniform(
      (const SharedStructuredVolume *uniform)sampler->volume,
      objectCoordinates,
      sampler->filter,
      0);
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
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const uniform float sampleTime,
                          void *uniform _samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;


  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    if (attributeHasUnstructuredTimeData(self, attributeIndex)) {
      *samples = SharedStructuredVolume_computeSample_TUV_varying(
        self, *objectCoordinates, sampler->filter, attributeIndex, sampleTime);
    } else if (attributeHasStructuredTimeData(self, attributeIndex)) {
      *samples = SharedStructuredVolume_computeSample_MB_varying(
        self, *objectCoordinates, sampler->filter, attributeIndex, sampleTime);
    } else {
      *samples = SharedStructuredVolume_computeSample_varying(
        self, *objectCoordinates, sampler->filter, attributeIndex);
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_TUV_export,
                          uniform const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform VKLFilter filter,
                          const uniform uint32 attributeIndex,
                          const uniform float sampleTime,
                          void *uniform _samples)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = SharedStructuredVolume_computeSample_TUV_varying(
        self, *objectCoordinates, filter, attributeIndex, sampleTime);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_MB_export,
                          uniform const int *uniform imask,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform VKLFilter filter,
                          const uniform uint32 attributeIndex,
                          const uniform float sampleTime,
                          void *uniform _samples)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = SharedStructuredVolume_computeSample_MB_varying(
        self, *objectCoordinates, filter, attributeIndex, sampleTime);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_uniform_export,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const uniform float sampleTime,
                          void *uniform _sample)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  float *uniform sample = (float *uniform)_sample;

  if (attributeHasUnstructuredTimeData(self, attributeIndex)) {
    *sample = SharedStructuredVolume_computeSample_TUV_uniform(
        self, *objectCoordinates, sampler->filter, attributeIndex, sampleTime);
  } else if (attributeHasStructuredTimeData(self, attributeIndex)) {
    *sample = SharedStructuredVolume_computeSample_MB_uniform(
        self, *objectCoordinates, sampler->filter, attributeIndex, sampleTime);
  } else {
    *sample = SharedStructuredVolume_computeSample_uniform(
        self, *objectCoordinates, sampler->filter, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sample_N_export,
                          const void *uniform _sampler,
                          const uniform uint32 N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];

    if (attributeHasUnstructuredTimeData(self, attributeIndex)) {
      samples[i]       = SharedStructuredVolume_computeSample_TUV_varying(
        self, oc, sampler->filter, attributeIndex, sampleTime);
    } else if (attributeHasStructuredTimeData(self, attributeIndex)) {
      samples[i]       = SharedStructuredVolume_computeSample_MB_varying(
        self, oc, sampler->filter, attributeIndex, sampleTime);
    } else {
      samples[i]       = SharedStructuredVolume_computeSample_varying(
        self, oc, sampler->filter, attributeIndex);
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 attributeIndex,
                          void *uniform _gradients)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;
    *gradients                       = self->computeGradient_varying(
        self, *objectCoordinates, sampler->gradientFilter, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_gradient_N_export,
                          const void *uniform _sampler,
                          const uniform unsigned int N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 attributeIndex,
                          vec3f *uniform gradients)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f oc = objectCoordinates[i];
    gradients[i]     = self->computeGradient_varying(
        self, oc, sampler->gradientFilter, attributeIndex);
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_export,
                          uniform const int *uniform imask,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;

    vec3f clampedLocalCoordinates;
    bool inBounds;
    clampedLocalCoordinates_varying(
        self, *objectCoordinates, clampedLocalCoordinates, inBounds);

    if (inBounds) {
      for (uniform uint32 i = 0; i < M; i++) {
        if (attributeHasUnstructuredTimeData(self, attributeIndices[i])) {
          float attributeSamples =
              self->computeSamplesInner_TUV_varying[attributeIndices[i]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[i], sampleTime);

          samples[i * VKL_TARGET_WIDTH + programIndex] = attributeSamples;
        } else if (attributeHasStructuredTimeData(self, attributeIndices[i])) {
          float attributeSamples =
              self->computeSamplesInner_MB_varying[attributeIndices[i]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[i], sampleTime);

          samples[i * VKL_TARGET_WIDTH + programIndex] = attributeSamples;
        } else {
          float attributeSamples =
              self->computeSamplesInner_varying[attributeIndices[i]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[i]);

          samples[i * VKL_TARGET_WIDTH + programIndex] = attributeSamples;
        }
      }
    } else {
      for (uniform uint32 i = 0; i < M; i++) {
        samples[i * VKL_TARGET_WIDTH + programIndex] = nanValue();
      }
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_uniform_export,
                          const void *uniform _sampler,
                          const void *uniform _objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;

  uniform vec3f clampedLocalCoordinates;
  uniform bool inBounds;
  clampedLocalCoordinates_uniform(
      self, *objectCoordinates, clampedLocalCoordinates, inBounds);

  if (inBounds) {
    for (uniform uint32 i = 0; i < M; i++) {

      if (attributeHasUnstructuredTimeData(self, attributeIndices[i])) {
        samples[i] = self->computeSamplesInner_TUV_uniform[attributeIndices[i]](
          self, clampedLocalCoordinates, sampler->filter, attributeIndices[i], sampleTime);
      } else if (attributeHasStructuredTimeData(self, attributeIndices[i])) {
        samples[i] = self->computeSamplesInner_MB_uniform[attributeIndices[i]](
          self, clampedLocalCoordinates, sampler->filter, attributeIndices[i], sampleTime);
      } else {
        samples[i] = self->computeSamplesInner_uniform[attributeIndices[i]](
          self, clampedLocalCoordinates, sampler->filter, attributeIndices[i]);
      }
    }
  } else {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = nanValue();
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_TUV_uniform_export,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform VKLFilter filter,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume * uniform) _self;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;

  uniform vec3f clampedLocalCoordinates;
  uniform bool inBounds;
  clampedLocalCoordinates_uniform(
      self, *objectCoordinates, clampedLocalCoordinates, inBounds);

  if (inBounds) {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = self->computeSamplesInner_TUV_uniform[attributeIndices[i]](
          self, clampedLocalCoordinates, filter, attributeIndices[i], sampleTime);
    }
  } else {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = nanValue();
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_MB_uniform_export,
                          void *uniform _self,
                          const void *uniform _objectCoordinates,
                          const uniform VKLFilter filter,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume * uniform) _self;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;

  uniform vec3f clampedLocalCoordinates;
  uniform bool inBounds;
  clampedLocalCoordinates_uniform(
      self, *objectCoordinates, clampedLocalCoordinates, inBounds);

  if (inBounds) {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = self->computeSamplesInner_MB_uniform[attributeIndices[i]](
          self, clampedLocalCoordinates, filter, attributeIndices[i], sampleTime);
    }
  } else {
    for (uniform uint32 i = 0; i < M; i++) {
      samples[i] = nanValue();
    }
  }
}

export void EXPORT_UNIQUE(SharedStructuredVolume_sampleM_N_export,
                          const void *uniform _sampler,
                          const uniform uint32 N,
                          const vec3f *uniform objectCoordinates,
                          const uniform uint32 M,
                          const uint32 *uniform attributeIndices,
                          const uniform float sampleTime,
                          float *uniform samples)
{
  const Sampler *uniform sampler = (const Sampler *uniform)_sampler;
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)sampler->volume;

  foreach (i = 0 ... N) {
    varying vec3f objectCoordinatesV = objectCoordinates[i];

    vec3f clampedLocalCoordinates;
    bool inBounds;
    clampedLocalCoordinates_varying(
        self, objectCoordinatesV, clampedLocalCoordinates, inBounds);

    if (inBounds) {
      for (uniform uint32 a = 0; a < M; a++) {

        if (attributeHasUnstructuredTimeData(self, attributeIndices[a])) {
          varying float attributeSamples =
              self->computeSamplesInner_TUV_varying[attributeIndices[a]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[a], sampleTime);

          samples[i * M + a] = attributeSamples;
        } else if (attributeHasStructuredTimeData(self, attributeIndices[a])) {
          varying float attributeSamples =
              self->computeSamplesInner_MB_varying[attributeIndices[a]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[a], sampleTime);

          samples[i * M + a] = attributeSamples;
        } else {
          varying float attributeSamples =
              self->computeSamplesInner_varying[attributeIndices[a]](
                  self, clampedLocalCoordinates, sampler->filter, attributeIndices[a]);

          samples[i * M + a] = attributeSamples;
        }
      }
    } else {
      for (uniform uint32 a = 0; a < M; a++) {
        samples[i * M + a] = nanValue();
      }
    }
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
  memset(self, 0, sizeof(uniform SharedStructuredVolume));

  return self;
}

export uniform bool EXPORT_UNIQUE(SharedStructuredVolume_set,
                                  void *uniform _self,
                                  const uniform uint32 numAttributes,
                                  const Data1D *uniform *uniform attributesData,
                                  const Data1D *uniform *uniform attributesTimeData,
                                  const Data1D *uniform *uniform attributesTimeConfig,
                                  const uniform vec3i &dimensions,
                                  const uniform SharedStructuredVolumeGridType
                                      gridType,
                                  const uniform vec3f &gridOrigin,
                                  const uniform vec3f &gridSpacing,
                                  const uniform VKLFilter filter)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  destructAttributesStorage(self);
  constructAttributesStorage(self, numAttributes);

  for (uniform uint32 i = 0; i < numAttributes; i++) {
    self->attributesData[i] = *(attributesData[i]);
    if (attributesTimeData && attributesTimeData[i]) {
      self->attributesTimeData[i] = *(attributesTimeData[i]);
    } else {
      self->attributesTimeData[i].addr = NULL;
      self->attributesTimeData[i].numItems = 0;
    }
    if (attributesTimeConfig) {
      self->attributesTimeConfig[i] = *(attributesTimeConfig[i]);
    } else {
      self->attributesTimeConfig[i].addr = NULL;
      self->attributesTimeConfig[i].numItems = 0;
    }
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

  self->filter = filter;

  // default sampling function (64-bit addressing)
  for (uniform uint32 i = 0; i < numAttributes; i++) {
    self->computeSamplesInner_varying[i] = SSV_sample_inner_varying_64;
    self->computeSamplesInner_uniform[i] = SSV_sample_inner_uniform_64;
    if (self->attributesData[i].dataType == VKL_UCHAR) {
      self->computeSamplesInner_TUV_varying[i] =
          SSV_sample_inner_TUV_uint8_varying_64;
      self->computeSamplesInner_MB_varying[i] =
          SSV_sample_inner_MB_uint8_varying_64;
      self->computeSamplesInner_TUV_uniform[i] =
          SSV_sample_inner_TUV_uint8_uniform_64;
      self->computeSamplesInner_MB_uniform[i] =
          SSV_sample_inner_MB_uint8_uniform_64;
    } else if (self->attributesData[i].dataType == VKL_SHORT) {
      self->computeSamplesInner_TUV_varying[i] =
          SSV_sample_inner_TUV_int16_varying_64;
      self->computeSamplesInner_MB_varying[i] =
          SSV_sample_inner_MB_int16_varying_64;
      self->computeSamplesInner_TUV_uniform[i] =
          SSV_sample_inner_TUV_int16_uniform_64;
      self->computeSamplesInner_MB_uniform[i] =
          SSV_sample_inner_MB_int16_uniform_64;
    } else if (self->attributesData[i].dataType == VKL_USHORT) {
      self->computeSamplesInner_TUV_varying[i] =
          SSV_sample_inner_TUV_uint16_varying_64;
      self->computeSamplesInner_MB_varying[i] =
          SSV_sample_inner_MB_uint16_varying_64;
      self->computeSamplesInner_TUV_uniform[i] =
          SSV_sample_inner_TUV_uint16_uniform_64;
      self->computeSamplesInner_MB_uniform[i] =
          SSV_sample_inner_MB_uint16_uniform_64;
    } else if (self->attributesData[i].dataType == VKL_FLOAT) {
      self->computeSamplesInner_TUV_varying[i] =
          SSV_sample_inner_TUV_float_varying_64;
      self->computeSamplesInner_MB_varying[i] =
          SSV_sample_inner_MB_float_varying_64;
      self->computeSamplesInner_TUV_uniform[i] =
          SSV_sample_inner_TUV_float_uniform_64;
      self->computeSamplesInner_MB_uniform[i] =
          SSV_sample_inner_MB_float_uniform_64;
    } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
      self->computeSamplesInner_TUV_varying[i] =
          SSV_sample_inner_TUV_double_varying_64;
      self->computeSamplesInner_MB_varying[i] =
          SSV_sample_inner_MB_double_varying_64;
      self->computeSamplesInner_TUV_uniform[i] =
          SSV_sample_inner_TUV_double_uniform_64;
      self->computeSamplesInner_MB_uniform[i] =
          SSV_sample_inner_MB_double_uniform_64;
    } else {
      print("#vkl:shared_structured_volume: unknown voxelType\n");
      return false;
    }
    // self->computeSamplesInner_TUV_varying[i] = SSV_sample_inner_TUV_varying_64;
    // self->computeSamplesInner_TUV_uniform[i] = SSV_sample_inner_TUV_uniform_64;
    // self->computeSamplesInner_MB_varying[i] = SSV_sample_inner_MB_varying_64;
    // self->computeSamplesInner_MB_uniform[i] = SSV_sample_inner_MB_uniform_64;

    if (safe_32bit_indexing(
            self->attributesData[i],
            dimensions.x * dimensions.y * (uint64)dimensions.z)) {
      // in this case, we know ALL addressing can be 32-bit.
      PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

      if (self->attributesData[i].dataType == VKL_UCHAR) {
        self->getVoxels_varying[i] = SSV_getVoxel_uint8_varying_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_uint8_varying_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_uint8_varying_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_uint8_varying_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint8_uniform_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_uint8_uniform_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_uint8_uniform_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_uint8_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_SHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_int16_varying_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_int16_varying_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_int16_varying_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_int16_varying_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_int16_uniform_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_int16_uniform_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_int16_uniform_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_int16_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_USHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_uint16_varying_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_uint16_varying_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_uint16_varying_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_uint16_varying_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint16_uniform_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_uint16_uniform_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_uint16_uniform_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_uint16_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_FLOAT) {
        self->getVoxels_varying[i] = SSV_getVoxel_float_varying_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_float_varying_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_float_varying_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_float_varying_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_float_uniform_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_float_uniform_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_float_uniform_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_float_uniform_32;
      } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
        self->getVoxels_varying[i] = SSV_getVoxel_double_varying_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_double_varying_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_double_varying_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_double_varying_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_double_uniform_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_double_uniform_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_double_uniform_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_double_uniform_32;
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
        self->getVoxels_varying[i] = SSV_getVoxel_uint8_varying_64_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_uint8_varying_64_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_uint8_varying_64_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_uint8_varying_64_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint8_uniform_64_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_uint8_uniform_64_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_uint8_uniform_64_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_uint8_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_SHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_int16_varying_64_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_int16_varying_64_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_int16_varying_64_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_int16_varying_64_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_int16_uniform_64_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_int16_uniform_64_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_int16_uniform_64_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_int16_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_USHORT) {
        self->getVoxels_varying[i] = SSV_getVoxel_uint16_varying_64_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_uint16_varying_64_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_uint16_varying_64_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_uint16_varying_64_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_uint16_uniform_64_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_uint16_uniform_64_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_uint16_uniform_64_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_uint16_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_FLOAT) {
        self->getVoxels_varying[i] = SSV_getVoxel_float_varying_64_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_float_varying_64_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_float_varying_64_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_float_varying_64_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_float_uniform_64_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_float_uniform_64_32;
        self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_float_uniform_64_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_float_uniform_64_32;
      } else if (self->attributesData[i].dataType == VKL_DOUBLE) {
        self->getVoxels_varying[i] = SSV_getVoxel_double_varying_64_32;
        self->computeSamplesInner_varying[i] =
            SSV_sample_inner_double_varying_64_32;
        self->computeSamplesInner_TUV_varying[i] =
            SSV_sample_inner_TUV_double_varying_64_32;
        self->computeSamplesInner_MB_varying[i] =
            SSV_sample_inner_MB_double_varying_64_32;
        self->getVoxels_uniform[i] = SSV_getVoxel_double_uniform_64_32;
        self->computeSamplesInner_uniform[i] =
            SSV_sample_inner_double_uniform_64_32;
         self->computeSamplesInner_TUV_uniform[i] =
            SSV_sample_inner_TUV_double_uniform_64_32;
        self->computeSamplesInner_MB_uniform[i] =
            SSV_sample_inner_MB_double_uniform_64_32;
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

export Sampler *uniform EXPORT_UNIQUE(StructuredSampler_create,
                                      void *uniform _volume)
{
  Sampler *uniform sampler       = CALL_ISPC(Sampler_create, _volume);
  sampler->computeSample_uniform = computeSample_uniform_0_iterator_wrapper;
  sampler->computeSample_varying = computeSample_varying_0_iterator_wrapper;
  return sampler;
}

export void EXPORT_UNIQUE(StructuredSampler_destroy, void *uniform _sampler)
{
  CALL_ISPC(Sampler_destroy, _sampler);
}
