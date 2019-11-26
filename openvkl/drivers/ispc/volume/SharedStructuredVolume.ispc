// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

inline void transformObjectToLocalUniform_structured_regular(
    const SharedStructuredVolume *uniform self,
    const uniform vec3f &objectCoordinates,
    uniform vec3f &localCoordinates)
{
  localCoordinates =
      1.f / (self->gridSpacing) * (objectCoordinates - self->gridOrigin);
}

// Structured spherical ///////////////////////////////////////////////////////

inline void transformLocalToObject_structured_spherical(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &localCoordinates,
    varying vec3f &objectCoordinates)
{
  // (r, inclination, azimuth) -> (x, y, z), using the ISO convention for
  // coordinates and ordering

  const uniform float degToRad = PI / 180.f;

  const float r = self->gridOrigin.x + localCoordinates.x * self->gridSpacing.x;

  const float inclination =
      (self->gridOrigin.y + localCoordinates.y * self->gridSpacing.y) *
      degToRad;

  const float azimuth =
      (self->gridOrigin.z + localCoordinates.z * self->gridSpacing.z) *
      degToRad;

  objectCoordinates.x = r * sinf(inclination) * cosf(azimuth);
  objectCoordinates.y = r * sinf(inclination) * sinf(azimuth);
  objectCoordinates.z = r * cosf(inclination);
}

#define template_transformObjectToLocal_structured_spherical(univary)          \
  inline void transformObjectToLocal_##univary##_structured_spherical(         \
      const SharedStructuredVolume *uniform self,                              \
      const univary vec3f &objectCoordinates,                                  \
      univary vec3f &localCoordinates)                                         \
  {                                                                            \
    /* (x, y, z) -> (r, inclination, azimuth), using the ISO convention for    \
     coordinates and ordering */                                               \
                                                                               \
    const uniform radToDeg = 180.f / PI;                                       \
                                                                               \
    const univary float uncorrectedRadius =                                    \
        sqrtf(objectCoordinates.x * objectCoordinates.x +                      \
              objectCoordinates.y * objectCoordinates.y +                      \
              objectCoordinates.z * objectCoordinates.z);                      \
                                                                               \
    const univary float uncorrectedInclination =                               \
        acos(objectCoordinates.z / uncorrectedRadius) * radToDeg;              \
                                                                               \
    univary float uncorrectedAzimuth =                                         \
        atan2(objectCoordinates.y, objectCoordinates.x) * radToDeg;            \
                                                                               \
    /* the above returns [-180, 180], while our azimuth grid convention is [0, \
     360] */                                                                   \
    if (uncorrectedAzimuth < 0.f) {                                            \
      uncorrectedAzimuth += 360.f;                                             \
    }                                                                          \
                                                                               \
    localCoordinates.x =                                                       \
        1.f / self->gridSpacing.x * (uncorrectedRadius - self->gridOrigin.x);  \
    localCoordinates.y = 1.f / self->gridSpacing.y *                           \
                         (uncorrectedInclination - self->gridOrigin.y);        \
    localCoordinates.z =                                                       \
        1.f / self->gridSpacing.z * (uncorrectedAzimuth - self->gridOrigin.z); \
  }

template_transformObjectToLocal_structured_spherical(varying);
template_transformObjectToLocal_structured_spherical(uniform);
#undef template_transformObjectToLocal_structured_spherical

///////////////////////////////////////////////////////////////////////////////
// getVoxel functions for all addressing / voxel type combinations ////////////
///////////////////////////////////////////////////////////////////////////////

// used below in template_getVoxel
#define process_index_z(univary) process_index_z_##univary
#define process_index_z_varying foreach_unique(z in index.z)
#define process_index_z_uniform uniform int z = index.z;

#define process_hi28(univary) process_hi28_##univary
#define process_hi28_varying foreach_unique(hi in hi28)
#define process_hi28_uniform uniform uint32 hi = hi28;

#define template_getVoxel(type, univary)                                     \
  /* for pure 32-bit addressing. volume *MUST* be smaller than 2G */         \
  inline void SSV_getVoxel_##type##_##univary##_32(                          \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      univary float &value)                                                  \
  {                                                                          \
    const type *uniform voxelData = (const type *uniform)self->voxelData;    \
    const univary uint32 addr =                                              \
        index.x +                                                            \
        self->dimensions.x * (index.y + self->dimensions.y * index.z);       \
                                                                             \
    value = voxelData[addr];                                                 \
  }                                                                          \
  /* for 64/32-bit addressing. volume itself can be larger than 2G, but each \
   * slice must be within the 2G limit. */                                   \
  inline void SSV_getVoxel_##type##_##univary##_64_32(                       \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      univary float &value)                                                  \
  {                                                                          \
    const uniform uint8 *uniform basePtr =                                   \
        (const uniform uint8 *uniform)self->voxelData;                       \
                                                                             \
    /* iterate over slices, then do 32-bit gather in slice */                \
    const univary uint32 ofs = index.x + self->dimensions.x * index.y;       \
    process_index_z(univary)                                                 \
    {                                                                        \
      const uniform uint64 byteOffset = z * self->bytesPerSlice;             \
      const uniform type *uniform sliceData =                                \
          (const uniform type *uniform)(basePtr + byteOffset);               \
      value = sliceData[ofs];                                                \
    }                                                                        \
  }                                                                          \
  /* for full 64-bit addressing, for all dimensions or slice size */         \
  inline void SSV_getVoxel_##type##_##univary##_64(                          \
      const SharedStructuredVolume *uniform self,                            \
      const univary vec3i &index,                                            \
      univary float &value)                                                  \
  {                                                                          \
    const univary uint64 index64 =                                           \
        (uint64)index.x +                                                    \
        self->dimensions.x *                                                 \
            ((int64)index.y + self->dimensions.y * ((uint64)index.z));       \
    const univary uint32 hi28 = index64 >> 28;                               \
    const univary uint32 lo28 = index64 & ((1 << 28) - 1);                   \
                                                                             \
    process_hi28(univary)                                                    \
    {                                                                        \
      const uniform uint64 hi64 = hi;                                        \
      const type *uniform base =                                             \
          ((const type *)self->voxelData) + (hi64 << 28);                    \
      value = base[lo28];                                                    \
    }                                                                        \
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

// read a _typed_ value from an address that's given by an *BYTE*-offset
// relative to a base array. note that even though we assume that the offset is
// already in bytes (ie, WITHOUT scaling by the width of the array data type),
// the base pointer must *STILL* be of the proper type for this macro to figure
// out the type of data it's reading from this address
#define template_accessArray(type, univary)                                \
  inline univary float accessArrayWithOffset(const type *uniform basePtr,  \
                                             const univary uint32 offset)  \
  {                                                                        \
    uniform uint8 *uniform base = (uniform uint8 * uniform) basePtr;       \
    return *((uniform type *)(base + offset));                             \
  }                                                                        \
  inline univary float accessArrayWithOffset(const type *uniform basePtr,  \
                                             const uniform uint64 baseOfs, \
                                             const univary uint32 offset)  \
  {                                                                        \
    uniform uint8 *uniform base = (uniform uint8 * uniform)(basePtr);      \
    return *((uniform type *)((base + baseOfs) + offset));                 \
  }

template_accessArray(uint8, varying);
template_accessArray(int16, varying);
template_accessArray(uint16, varying);
template_accessArray(float, varying);
template_accessArray(double, varying);

template_accessArray(uint8, uniform);
template_accessArray(int16, uniform);
template_accessArray(uint16, uniform);
template_accessArray(float, uniform);
template_accessArray(double, uniform);
#undef template_accessArray

// overloads for both varying and uniform coordinate transformations, used in
// templated sampling functions
inline void transformObjectToLocalUnivary(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates,
    varying vec3f &localCoordinates)
{
  self->transformObjectToLocal(self, objectCoordinates, localCoordinates);
}

inline void transformObjectToLocalUnivary(
    const SharedStructuredVolume *uniform self,
    const uniform vec3f &objectCoordinates,
    uniform vec3f &localCoordinates)
{
  self->transformObjectToLocalUniform(
      self, objectCoordinates, localCoordinates);
}

// overloads for both varying and uniform voxel getters, used in templated
// sampling functions
inline void getVoxelUnivary(const SharedStructuredVolume *uniform self,
                            const varying vec3i &index,
                            varying float &value)
{
  self->getVoxel(self, index, value);
}

inline void getVoxelUnivary(const SharedStructuredVolume *uniform self,
                            const uniform vec3i &index,
                            uniform float &value)
{
  self->getVoxelUniform(self, index, value);
}

// perform trilinear interpolation for given sample. unlike old way of doing
// this (a single computesample on the StructuredVolume level that calls the
// virtual 'getSample()' of the volume layout) this function will directly do
// all the addressing for the getSample (inlined), and thus be about 50% faster
// (wall-time, meaning even much faster in pure sample speed)
#define template_sample_32(type, univary)                                      \
  inline univary float SSV_sample_##type##_##univary##_32(                     \
      const void *uniform _self, const univary vec3f &objectCoordinates)       \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    univary vec3f localCoordinates;                                            \
    transformObjectToLocalUnivary(self, objectCoordinates, localCoordinates);  \
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
    const type *uniform voxelData = (const type *uniform)self->voxelData;      \
    const uniform uint64 ofs000   = 0;                                         \
    const uniform uint64 ofs001   = self->bytesPerVoxel;                       \
    const univary float val000 =                                               \
        accessArrayWithOffset(voxelData, ofs000, voxelOfs);                    \
    const univary float val001 =                                               \
        accessArrayWithOffset(voxelData, ofs001, voxelOfs);                    \
    const univary float val00 = val000 + frac.x * (val001 - val000);           \
                                                                               \
    const uniform uint64 ofs010 = self->bytesPerLine;                          \
    const uniform uint64 ofs011 = self->bytesPerLine + self->bytesPerVoxel;    \
    const univary float val010 =                                               \
        accessArrayWithOffset(voxelData, ofs010, voxelOfs);                    \
    const univary float val011 =                                               \
        accessArrayWithOffset(voxelData, ofs011, voxelOfs);                    \
    const univary float val01 = val010 + frac.x * (val011 - val010);           \
                                                                               \
    const uniform uint64 ofs100 = self->bytesPerSlice;                         \
    const uniform uint64 ofs101 = ofs100 + ofs001;                             \
    const univary float val100 =                                               \
        accessArrayWithOffset(voxelData, ofs100, voxelOfs);                    \
    const univary float val101 =                                               \
        accessArrayWithOffset(voxelData, ofs101, voxelOfs);                    \
    const univary float val10 = val100 + frac.x * (val101 - val100);           \
                                                                               \
    const uniform uint64 ofs110 = ofs100 + ofs010;                             \
    const uniform uint64 ofs111 = ofs100 + ofs011;                             \
    const univary float val110 =                                               \
        accessArrayWithOffset(voxelData, ofs110, voxelOfs);                    \
    const univary float val111 =                                               \
        accessArrayWithOffset(voxelData, ofs111, voxelOfs);                    \
    const univary float val11 = val110 + frac.x * (val111 - val110);           \
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
#define process_sliceID_uniform uniform sliceID = voxelIndex_0.z;

#define template_sample_64_32(type, univary)                                   \
  inline univary float SSV_sample_##type##_##univary##_64_32(                  \
      const void *uniform _self, const univary vec3f &objectCoordinates)       \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    univary vec3f localCoordinates;                                            \
    transformObjectToLocalUnivary(self, objectCoordinates, localCoordinates);  \
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
      const univary uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +     \
                                      voxelIndex_0.y * self->voxelOfs_dy;      \
      const type *uniform voxelData =                                          \
          (const type *uniform)((uniform uint8 * uniform) self->voxelData +    \
                                sliceID * self->bytesPerSlice);                \
      const uniform uint64 ofs000 = 0;                                         \
      const uniform uint64 ofs001 = self->bytesPerVoxel;                       \
      const univary float val000 =                                             \
          accessArrayWithOffset(voxelData, ofs000, voxelOfs);                  \
      const univary float val001 =                                             \
          accessArrayWithOffset(voxelData, ofs001, voxelOfs);                  \
      const univary float val00 = val000 + frac.x * (val001 - val000);         \
                                                                               \
      const uniform uint64 ofs010 = self->bytesPerLine;                        \
      const uniform uint64 ofs011 = self->bytesPerLine + self->bytesPerVoxel;  \
      const univary float val010 =                                             \
          accessArrayWithOffset(voxelData, ofs010, voxelOfs);                  \
      const univary float val011 =                                             \
          accessArrayWithOffset(voxelData, ofs011, voxelOfs);                  \
      const univary float val01 = val010 + frac.x * (val011 - val010);         \
                                                                               \
      const uniform uint64 ofs100 = self->bytesPerSlice;                       \
      const uniform uint64 ofs101 = ofs100 + ofs001;                           \
      const univary float val100 =                                             \
          accessArrayWithOffset(voxelData, ofs100, voxelOfs);                  \
      const univary float val101 =                                             \
          accessArrayWithOffset(voxelData, ofs101, voxelOfs);                  \
      const univary float val10 = val100 + frac.x * (val101 - val100);         \
                                                                               \
      const uniform uint64 ofs110 = ofs100 + ofs010;                           \
      const uniform uint64 ofs111 = ofs100 + ofs011;                           \
      const univary float val110 =                                             \
          accessArrayWithOffset(voxelData, ofs110, voxelOfs);                  \
      const univary float val111 =                                             \
          accessArrayWithOffset(voxelData, ofs111, voxelOfs);                  \
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
      const void *uniform _self, const univary vec3f &objectCoordinates)       \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    univary vec3f localCoordinates;                                            \
    transformObjectToLocalUnivary(self, objectCoordinates, localCoordinates);  \
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
        voxelValue_000);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),            \
        voxelValue_001);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),            \
        voxelValue_010);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),            \
        voxelValue_011);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),            \
        voxelValue_100);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),            \
        voxelValue_101);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),            \
        voxelValue_110);                                                       \
    getVoxelUnivary(                                                           \
        self,                                                                  \
        make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),            \
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

inline varying vec3f SharedStructuredVolume_computeGradient(
    const SharedStructuredVolume *uniform self,
    const varying vec3f &objectCoordinates)
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

  float sample = self->super.computeSample(self, objectCoordinates);

  gradient.x =
      self->super.computeSample(
          self, objectCoordinates + make_vec3f(gradientStep.x, 0.f, 0.f)) -
      sample;
  gradient.y =
      self->super.computeSample(
          self, objectCoordinates + make_vec3f(0.f, gradientStep.y, 0.f)) -
      sample;
  gradient.z =
      self->super.computeSample(
          self, objectCoordinates + make_vec3f(0.f, 0.f, gradientStep.z)) -
      sample;

  return gradient / gradientStep;
}

///////////////////////////////////////////////////////////////////////////////
// SharedStructuredVolume exported functions //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

export uniform box3f SharedStructuredVolume_getBoundingBox(void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  return self->boundingBox;
}

export void SharedStructuredVolume_sample_export(
    uniform const int *uniform imask,
    void *uniform _self,
    const void *uniform _objectCoordinates,
    void *uniform _samples)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    *samples = self->super.computeSample(self, *objectCoordinates);
  }
}

export void SharedStructuredVolume_sample_uniform_export(void *uniform _self,
                                                         const void *uniform
                                                             _objectCoordinates,
                                                         void *uniform _sample)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  const vec3f *uniform objectCoordinates =
      (const vec3f *uniform)_objectCoordinates;
  float *uniform sample = (float *uniform)_sample;

  *sample = self->computeSampleUniform(self, *objectCoordinates);
}

export void SharedStructuredVolume_gradient_export(
    uniform const int *uniform imask,
    void *uniform _self,
    const void *uniform _objectCoordinates,
    void *uniform _gradients)
{
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  if (imask[programIndex]) {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying vec3f *uniform gradients = (varying vec3f * uniform) _gradients;

    *gradients =
        SharedStructuredVolume_computeGradient(self, *objectCoordinates);
  }
}

export void *uniform SharedStructuredVolume_Destructor(void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  if (self->accelerator) {
    GridAccelerator_Destructor(self->accelerator);
  }

  delete self;
}

export void *uniform SharedStructuredVolume_Constructor()
{
  uniform SharedStructuredVolume *uniform self =
      uniform new uniform SharedStructuredVolume;

  self->accelerator = NULL;

  return self;
}

export uniform bool SharedStructuredVolume_set(
    void *uniform _self,
    const void *uniform voxelData,
    const uniform int voxelType,
    const uniform vec3i &dimensions,
    const uniform SharedStructuredVolumeGridType gridType,
    const uniform vec3f &gridOrigin,
    const uniform vec3f &gridSpacing)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  self->voxelData   = voxelData;
  self->voxelType   = (VKLDataType)voxelType;
  self->dimensions  = dimensions;
  self->gridType    = gridType;
  self->gridOrigin  = gridOrigin;
  self->gridSpacing = gridSpacing;

  if (self->gridType == structured_regular) {
    self->boundingBox = make_box3f(
        gridOrigin, gridOrigin + make_vec3f(dimensions - 1.f) * gridSpacing);

    self->transformLocalToObject = transformLocalToObject_structured_regular;
    self->transformObjectToLocal = transformObjectToLocal_structured_regular;
    self->transformObjectToLocalUniform =
        transformObjectToLocalUniform_structured_regular;

  } else if (self->gridType == structured_spherical) {
    // a conservative bounding box
    // TODO: make this less conservative...
    const uniform float extent =
        self->gridOrigin.x + self->gridSpacing.x * (self->dimensions.x - 1);

    self->boundingBox = make_box3f(make_vec3f(-extent, -extent, -extent),
                                   make_vec3f(extent, extent, extent));

    self->transformLocalToObject = transformLocalToObject_structured_spherical;
    self->transformObjectToLocal =
        transformObjectToLocal_varying_structured_spherical;
    self->transformObjectToLocalUniform =
        transformObjectToLocal_uniform_structured_spherical;

  } else {
    print("#vkl:shared_structured_volume: unknown gridType\n");
    return false;
  }

  self->localCoordinatesUpperBound =
      nextafter(self->dimensions - 1, make_vec3i(0));

  uniform uint64 bytesPerVoxel;

  if (voxelType == VKL_UCHAR) {
    PRINT_DEBUG("#vkl:shared_structured_volume: using VKL_UCHAR voxelType\n");
    bytesPerVoxel = sizeof(uniform uint8);
  } else if (voxelType == VKL_SHORT) {
    PRINT_DEBUG("#vkl:shared_structured_volume: using VKL_SHORT voxelType\n");
    bytesPerVoxel = sizeof(uniform int16);
  } else if (voxelType == VKL_USHORT) {
    PRINT_DEBUG("#vkl:shared_structured_volume: using VKL_USHORT voxelType\n");
    bytesPerVoxel = sizeof(uniform uint16);
  } else if (voxelType == VKL_FLOAT) {
    PRINT_DEBUG("#vkl:shared_structured_volume: using VKL_FLOAT voxelType\n");
    bytesPerVoxel = sizeof(uniform float);
  } else if (voxelType == VKL_DOUBLE) {
    PRINT_DEBUG("#vkl:shared_structured_volume: using VKL_DOUBLE voxelType\n");
    bytesPerVoxel = sizeof(uniform double);
  } else {
    print("#vkl:shared_structured_volume: unknown voxelType\n");
    return false;
  }

  const uniform uint64 bytesPerLine   = bytesPerVoxel * dimensions.x;
  const uniform uint64 bytesPerSlice  = bytesPerLine * dimensions.y;
  const uniform uint64 bytesPerVolume = bytesPerSlice * dimensions.z;

  self->bytesPerVoxel = bytesPerVoxel;
  self->bytesPerLine  = bytesPerLine;
  self->bytesPerSlice = bytesPerSlice;
  self->voxelOfs_dx   = bytesPerVoxel;
  self->voxelOfs_dy   = bytesPerLine;
  self->voxelOfs_dz   = bytesPerSlice;

  // default sampling function (64-bit addressing)
  self->super.computeSample  = SSV_sample_varying_64;
  self->computeSampleUniform = SSV_sample_uniform_64;

  if (bytesPerVolume <= (1ULL << 30)) {
    // in this case, we know ALL addressing can be 32-bit.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

    if (voxelType == VKL_UCHAR) {
      self->getVoxel             = SSV_getVoxel_uint8_varying_32;
      self->super.computeSample  = SSV_sample_uint8_varying_32;
      self->getVoxelUniform      = SSV_getVoxel_uint8_uniform_32;
      self->computeSampleUniform = SSV_sample_uint8_uniform_32;
    } else if (voxelType == VKL_SHORT) {
      self->getVoxel             = SSV_getVoxel_int16_varying_32;
      self->super.computeSample  = SSV_sample_int16_varying_32;
      self->getVoxelUniform      = SSV_getVoxel_int16_uniform_32;
      self->computeSampleUniform = SSV_sample_int16_uniform_32;
    } else if (voxelType == VKL_USHORT) {
      self->getVoxel             = SSV_getVoxel_uint16_varying_32;
      self->super.computeSample  = SSV_sample_uint16_varying_32;
      self->getVoxelUniform      = SSV_getVoxel_uint16_uniform_32;
      self->computeSampleUniform = SSV_sample_uint16_uniform_32;
    } else if (voxelType == VKL_FLOAT) {
      self->getVoxel             = SSV_getVoxel_float_varying_32;
      self->super.computeSample  = SSV_sample_float_varying_32;
      self->getVoxelUniform      = SSV_getVoxel_float_uniform_32;
      self->computeSampleUniform = SSV_sample_float_uniform_32;
    } else if (voxelType == VKL_DOUBLE) {
      self->getVoxel             = SSV_getVoxel_double_varying_32;
      self->super.computeSample  = SSV_sample_double_varying_32;
      self->getVoxelUniform      = SSV_getVoxel_double_uniform_32;
      self->computeSampleUniform = SSV_sample_double_uniform_32;
    }

  } else if (bytesPerSlice <= (1ULL << 30)) {
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice begins
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

    if (voxelType == VKL_UCHAR) {
      self->getVoxel             = SSV_getVoxel_uint8_varying_64_32;
      self->super.computeSample  = SSV_sample_uint8_varying_64_32;
      self->getVoxelUniform      = SSV_getVoxel_uint8_uniform_64_32;
      self->computeSampleUniform = SSV_sample_uint8_uniform_64_32;
    } else if (voxelType == VKL_SHORT) {
      self->getVoxel             = SSV_getVoxel_int16_varying_64_32;
      self->super.computeSample  = SSV_sample_int16_varying_64_32;
      self->getVoxelUniform      = SSV_getVoxel_int16_uniform_64_32;
      self->computeSampleUniform = SSV_sample_int16_uniform_64_32;
    } else if (voxelType == VKL_USHORT) {
      self->getVoxel             = SSV_getVoxel_uint16_varying_64_32;
      self->super.computeSample  = SSV_sample_uint16_varying_64_32;
      self->getVoxelUniform      = SSV_getVoxel_uint16_uniform_64_32;
      self->computeSampleUniform = SSV_sample_uint16_uniform_64_32;
    } else if (voxelType == VKL_FLOAT) {
      self->getVoxel             = SSV_getVoxel_float_varying_64_32;
      self->super.computeSample  = SSV_sample_float_varying_64_32;
      self->getVoxelUniform      = SSV_getVoxel_float_uniform_64_32;
      self->computeSampleUniform = SSV_sample_float_uniform_64_32;
    } else if (voxelType == VKL_DOUBLE) {
      self->getVoxel             = SSV_getVoxel_double_varying_64_32;
      self->super.computeSample  = SSV_sample_double_varying_64_32;
      self->getVoxelUniform      = SSV_getVoxel_double_uniform_64_32;
      self->computeSampleUniform = SSV_sample_double_uniform_64_32;
    }
  } else {
    // in this case, even a single slice is too big to do 32-bit
    // addressing, and we have to do 64-bit throughout
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64-bit mode\n");

    if (voxelType == VKL_UCHAR) {
      self->getVoxel        = SSV_getVoxel_uint8_varying_64;
      self->getVoxelUniform = SSV_getVoxel_uint8_uniform_64;
    } else if (voxelType == VKL_SHORT) {
      self->getVoxel        = SSV_getVoxel_int16_varying_64;
      self->getVoxelUniform = SSV_getVoxel_int16_uniform_64;
    } else if (voxelType == VKL_USHORT) {
      self->getVoxel        = SSV_getVoxel_uint16_varying_64;
      self->getVoxelUniform = SSV_getVoxel_uint16_uniform_64;
    } else if (voxelType == VKL_FLOAT) {
      self->getVoxel        = SSV_getVoxel_float_varying_64;
      self->getVoxelUniform = SSV_getVoxel_float_uniform_64;
    } else if (voxelType == VKL_DOUBLE) {
      self->getVoxel        = SSV_getVoxel_double_varying_64;
      self->getVoxelUniform = SSV_getVoxel_double_uniform_64;
    }
  }

  return true;
}

export void *uniform
SharedStructuredVolume_createAccelerator(void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  if (self->accelerator) {
    GridAccelerator_Destructor(self->accelerator);
  }

  self->accelerator = GridAccelerator_Constructor(self);

  return self->accelerator;
}
