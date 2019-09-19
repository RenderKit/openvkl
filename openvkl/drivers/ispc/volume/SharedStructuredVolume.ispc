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

#include "GridAccelerator.ih"
#include "SharedStructuredVolume.ih"

// #define PRINT_DEBUG_ENABLE
#include "common/print_debug.ih"

///////////////////////////////////////////////////////////////////////////////
// Coordinate transformations for all supported structured volume types ///////
///////////////////////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////////////////////
// getVoxel functions for all addressing / voxel type combinations ////////////
///////////////////////////////////////////////////////////////////////////////

#define template_getVoxel(type)                                              \
  /* for pure 32-bit addressing. volume *MUST* be smaller than 2G */         \
  inline void SSV_getVoxel_##type##_32(                                      \
      const SharedStructuredVolume *uniform self,                            \
      const varying vec3i &index,                                            \
      varying float &value)                                                  \
  {                                                                          \
    const type *uniform voxelData = (const type *uniform)self->voxelData;    \
    const uint32 addr =                                                      \
        index.x +                                                            \
        self->dimensions.x * (index.y + self->dimensions.y * index.z);       \
                                                                             \
    value = voxelData[addr];                                                 \
  }                                                                          \
  /* for 64/32-bit addressing. volume itself can be larger than 2G, but each \
   * slice must be within the 2G limit. */                                   \
  inline void SSV_getVoxel_##type##_64_32(                                   \
      const SharedStructuredVolume *uniform self,                            \
      const varying vec3i &index,                                            \
      varying float &value)                                                  \
  {                                                                          \
    const uniform uint8 *uniform basePtr =                                   \
        (const uniform uint8 *uniform)self->voxelData;                       \
                                                                             \
    /* iterate over slices, then do 32-bit gather in slice */                \
    const uint32 ofs = index.x + self->dimensions.x * index.y;               \
    foreach_unique(z in index.z)                                             \
    {                                                                        \
      const uniform uint64 byteOffset = z * self->bytesPerSlice;             \
      const uniform type *uniform sliceData =                                \
          (const uniform type *uniform)(basePtr + byteOffset);               \
      value = sliceData[ofs];                                                \
    }                                                                        \
  }                                                                          \
  /* for full 64-bit addressing, for all dimensions or slice size */         \
  inline void SSV_getVoxel_##type##_64(                                      \
      const SharedStructuredVolume *uniform self,                            \
      const varying vec3i &index,                                            \
      varying float &value)                                                  \
  {                                                                          \
    const uint64 index64 =                                                   \
        (uint64)index.x +                                                    \
        self->dimensions.x *                                                 \
            ((int64)index.y + self->dimensions.y * ((uint64)index.z));       \
    const uint32 hi28 = index64 >> 28;                                       \
    const uint32 lo28 = index64 & ((1 << 28) - 1);                           \
                                                                             \
    foreach_unique(hi in hi28)                                               \
    {                                                                        \
      const uniform uint64 hi64 = hi;                                        \
      const type *uniform base =                                             \
          ((const type *)self->voxelData) + (hi64 << 28);                    \
      value = base[lo28];                                                    \
    }                                                                        \
  }

template_getVoxel(uint8);
template_getVoxel(int16);
template_getVoxel(uint16);
template_getVoxel(float);
template_getVoxel(double);
#undef template_getVoxel

///////////////////////////////////////////////////////////////////////////////
// Sampling methods for all addressing / voxel type combinations //////////////
///////////////////////////////////////////////////////////////////////////////

// read a _typed_ value from an address that's given by an *BYTE*-offset
// relative to a base array. note that even though we assume that the offset is
// already in bytes (ie, WITHOUT scaling by the width of the array data type),
// the base pointer must *STILL* be of the proper type for this macro to figure
// out the type of data it's reading from this address
#define template_accessArray(type)                                    \
  inline float accessArrayWithOffset(const type *uniform basePtr,     \
                                     const varying uint32 offset)     \
  {                                                                   \
    uniform uint8 *uniform base = (uniform uint8 * uniform) basePtr;  \
    return *((uniform type *)(base + offset));                        \
  }                                                                   \
  inline float accessArrayWithOffset(const type *uniform basePtr,     \
                                     const uniform uint64 baseOfs,    \
                                     const varying uint32 offset)     \
  {                                                                   \
    uniform uint8 *uniform base = (uniform uint8 * uniform)(basePtr); \
    return *((uniform type *)((base + baseOfs) + offset));            \
  }

template_accessArray(uint8);
template_accessArray(int16);
template_accessArray(uint16);
template_accessArray(float);
template_accessArray(double);
#undef template_accessArray

// perform trilinear interpolation for given sample. unlike old way of doing
// this (a single computesample on the StructuredVolume level that calls the
// virtual 'getSample()' of the volume layout) this function will directly do
// all the addressing for the getSample (inlined), and thus be about 50% faster
// (wall-time, meaning even much faster in pure sample speed)
#define template_sample(type)                                                  \
  inline float SSV_sample_##type##_32(const void *uniform _self,               \
                                      const varying vec3f &objectCoordinates)  \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    vec3f localCoordinates;                                                    \
    self->transformObjectToLocal(self, objectCoordinates, localCoordinates);   \
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
    const vec3f clampedLocalCoordinates = clamp(                               \
        localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound); \
                                                                               \
    /* lower corner of the box straddling the voxels to be interpolated. */    \
    const vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);                \
                                                                               \
    /* fractional coordinates within the lower corner voxel used during        \
     * interpolation. */                                                       \
    const vec3f frac = clampedLocalCoordinates - to_float(voxelIndex_0);       \
                                                                               \
    const uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +               \
                            voxelIndex_0.y * self->voxelOfs_dy +               \
                            voxelIndex_0.z * self->voxelOfs_dz;                \
    const type *uniform voxelData = (const type *uniform)self->voxelData;      \
    const uniform uint64 ofs000   = 0;                                         \
    const uniform uint64 ofs001   = self->bytesPerVoxel;                       \
    const float val000 = accessArrayWithOffset(voxelData, ofs000, voxelOfs);   \
    const float val001 = accessArrayWithOffset(voxelData, ofs001, voxelOfs);   \
    const float val00  = val000 + frac.x * (val001 - val000);                  \
                                                                               \
    const uniform uint64 ofs010 = self->bytesPerLine;                          \
    const uniform uint64 ofs011 = self->bytesPerLine + self->bytesPerVoxel;    \
    const float val010 = accessArrayWithOffset(voxelData, ofs010, voxelOfs);   \
    const float val011 = accessArrayWithOffset(voxelData, ofs011, voxelOfs);   \
    const float val01  = val010 + frac.x * (val011 - val010);                  \
                                                                               \
    const uniform uint64 ofs100 = self->bytesPerSlice;                         \
    const uniform uint64 ofs101 = ofs100 + ofs001;                             \
    const float val100 = accessArrayWithOffset(voxelData, ofs100, voxelOfs);   \
    const float val101 = accessArrayWithOffset(voxelData, ofs101, voxelOfs);   \
    const float val10  = val100 + frac.x * (val101 - val100);                  \
                                                                               \
    const uniform uint64 ofs110 = ofs100 + ofs010;                             \
    const uniform uint64 ofs111 = ofs100 + ofs011;                             \
    const float val110 = accessArrayWithOffset(voxelData, ofs110, voxelOfs);   \
    const float val111 = accessArrayWithOffset(voxelData, ofs111, voxelOfs);   \
    const float val11  = val110 + frac.x * (val111 - val110);                  \
                                                                               \
    const float val0 = val00 + frac.y * (val01 - val00);                       \
    const float val1 = val10 + frac.y * (val11 - val10);                       \
    const float val  = val0 + frac.z * (val1 - val0);                          \
                                                                               \
    return val;                                                                \
  }                                                                            \
                                                                               \
  inline float SSV_sample_##type##_64_32(                                      \
      const void *uniform _self, const varying vec3f &objectCoordinates)       \
  {                                                                            \
    const SharedStructuredVolume *uniform self =                               \
        (const SharedStructuredVolume *uniform)_self;                          \
                                                                               \
    vec3f localCoordinates;                                                    \
    self->transformObjectToLocal(self, objectCoordinates, localCoordinates);   \
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
    const vec3f clampedLocalCoordinates = clamp(                               \
        localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound); \
                                                                               \
    /* lower corner of the box straddling the voxels to be interpolated. */    \
    const vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);                \
                                                                               \
    /* fractional coordinates within the lower corner voxel used during        \
     * interpolation. */                                                       \
    const vec3f frac = clampedLocalCoordinates - to_float(voxelIndex_0);       \
                                                                               \
    varying float ret = 0.f;                                                   \
    foreach_unique(sliceID in voxelIndex_0.z)                                  \
    {                                                                          \
      const uint32 voxelOfs = voxelIndex_0.x * self->voxelOfs_dx +             \
                              voxelIndex_0.y * self->voxelOfs_dy;              \
      const type *uniform voxelData =                                          \
          (const type *uniform)((uniform uint8 * uniform) self->voxelData +    \
                                sliceID * self->bytesPerSlice);                \
      const uniform uint64 ofs000 = 0;                                         \
      const uniform uint64 ofs001 = self->bytesPerVoxel;                       \
      const float val000 = accessArrayWithOffset(voxelData, ofs000, voxelOfs); \
      const float val001 = accessArrayWithOffset(voxelData, ofs001, voxelOfs); \
      const float val00  = val000 + frac.x * (val001 - val000);                \
                                                                               \
      const uniform uint64 ofs010 = self->bytesPerLine;                        \
      const uniform uint64 ofs011 = self->bytesPerLine + self->bytesPerVoxel;  \
      const float val010 = accessArrayWithOffset(voxelData, ofs010, voxelOfs); \
      const float val011 = accessArrayWithOffset(voxelData, ofs011, voxelOfs); \
      const float val01  = val010 + frac.x * (val011 - val010);                \
                                                                               \
      const uniform uint64 ofs100 = self->bytesPerSlice;                       \
      const uniform uint64 ofs101 = ofs100 + ofs001;                           \
      const float val100 = accessArrayWithOffset(voxelData, ofs100, voxelOfs); \
      const float val101 = accessArrayWithOffset(voxelData, ofs101, voxelOfs); \
      const float val10  = val100 + frac.x * (val101 - val100);                \
                                                                               \
      const uniform uint64 ofs110 = ofs100 + ofs010;                           \
      const uniform uint64 ofs111 = ofs100 + ofs011;                           \
      const float val110 = accessArrayWithOffset(voxelData, ofs110, voxelOfs); \
      const float val111 = accessArrayWithOffset(voxelData, ofs111, voxelOfs); \
      const float val11  = val110 + frac.x * (val111 - val110);                \
                                                                               \
      const float val0 = val00 + frac.y * (val01 - val00);                     \
      const float val1 = val10 + frac.y * (val11 - val10);                     \
      const float val  = val0 + frac.z * (val1 - val0);                        \
      ret              = val;                                                  \
    }                                                                          \
    return ret;                                                                \
  }

template_sample(uint8);
template_sample(int16);
template_sample(uint16);
template_sample(float);
template_sample(double);
#undef template_sample

// default sampling function (64-bit addressing)
inline float SSV_sample_64(const void *uniform _self,
                           const varying vec3f &objectCoordinates)
{
  const SharedStructuredVolume *uniform self =
      (const SharedStructuredVolume *uniform)_self;

  vec3f localCoordinates;
  self->transformObjectToLocal(self, objectCoordinates, localCoordinates);

  // return NaN for local coordinates outside the bounds of the volume.
  const uniform int NaN_bits   = 0x7fc00000;
  const uniform float nanValue = floatbits(NaN_bits);

  if (localCoordinates.x < 0.f ||
      localCoordinates.x > self->dimensions.x - 1.f ||
      localCoordinates.y < 0.f ||
      localCoordinates.y > self->dimensions.y - 1.f ||
      localCoordinates.z < 0.f ||
      localCoordinates.z > self->dimensions.z - 1.f) {
    return nanValue;
  }

  const vec3f clampedLocalCoordinates = clamp(
      localCoordinates, make_vec3f(0.0f), self->localCoordinatesUpperBound);

  // lower and upper corners of the box straddling the voxels to be
  // interpolated.
  const vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  // fractional coordinates within the lower corner voxel used during
  // interpolation.
  const vec3f fractionalLocalCoordinates =
      clampedLocalCoordinates - to_float(voxelIndex_0);

  // look up the voxel values to be interpolated.
  float voxelValue_000;
  float voxelValue_001;
  float voxelValue_010;
  float voxelValue_011;
  float voxelValue_100;
  float voxelValue_101;
  float voxelValue_110;
  float voxelValue_111;
  self->getVoxel(self,
                 make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),
                 voxelValue_000);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),
                 voxelValue_001);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),
                 voxelValue_010);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),
                 voxelValue_011);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),
                 voxelValue_100);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),
                 voxelValue_101);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),
                 voxelValue_110);
  self->getVoxel(self,
                 make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),
                 voxelValue_111);

  // interpolate the voxel values.
  const float voxelValue_00 =
      voxelValue_000 +
      fractionalLocalCoordinates.x * (voxelValue_001 - voxelValue_000);
  const float voxelValue_01 =
      voxelValue_010 +
      fractionalLocalCoordinates.x * (voxelValue_011 - voxelValue_010);
  const float voxelValue_10 =
      voxelValue_100 +
      fractionalLocalCoordinates.x * (voxelValue_101 - voxelValue_100);
  const float voxelValue_11 =
      voxelValue_110 +
      fractionalLocalCoordinates.x * (voxelValue_111 - voxelValue_110);
  const float voxelValue_0 =
      voxelValue_00 +
      fractionalLocalCoordinates.y * (voxelValue_01 - voxelValue_00);
  const float voxelValue_1 =
      voxelValue_10 +
      fractionalLocalCoordinates.y * (voxelValue_11 - voxelValue_10);

  return voxelValue_0 +
         fractionalLocalCoordinates.z * (voxelValue_1 - voxelValue_0);
}

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
  self->super.computeSample = SSV_sample_64;

  if (bytesPerVolume <= (1ULL << 30)) {
    // in this case, we know ALL addressing can be 32-bit.
    PRINT_DEBUG("#vkl:shared_structured_volume: using 32-bit mode\n");

    if (voxelType == VKL_UCHAR) {
      self->getVoxel            = SSV_getVoxel_uint8_32;
      self->super.computeSample = SSV_sample_uint8_32;
    } else if (voxelType == VKL_SHORT) {
      self->getVoxel            = SSV_getVoxel_int16_32;
      self->super.computeSample = SSV_sample_int16_32;
    } else if (voxelType == VKL_USHORT) {
      self->getVoxel            = SSV_getVoxel_uint16_32;
      self->super.computeSample = SSV_sample_uint16_32;
    } else if (voxelType == VKL_FLOAT) {
      self->getVoxel            = SSV_getVoxel_float_32;
      self->super.computeSample = SSV_sample_float_32;
    } else if (voxelType == VKL_DOUBLE) {
      self->getVoxel            = SSV_getVoxel_double_32;
      self->super.computeSample = SSV_sample_double_32;
    }

  } else if (bytesPerSlice <= (1ULL << 30)) {
    // in this case, we know we can do 32-bit addressing within a
    // slice, but need 64-bit arithmetic to get slice begins
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64/32-bit mode\n");

    if (voxelType == VKL_UCHAR) {
      self->getVoxel            = SSV_getVoxel_uint8_64_32;
      self->super.computeSample = SSV_sample_uint8_64_32;
    } else if (voxelType == VKL_SHORT) {
      self->getVoxel            = SSV_getVoxel_int16_64_32;
      self->super.computeSample = SSV_sample_int16_64_32;
    } else if (voxelType == VKL_USHORT) {
      self->getVoxel            = SSV_getVoxel_uint16_64_32;
      self->super.computeSample = SSV_sample_uint16_64_32;
    } else if (voxelType == VKL_FLOAT) {
      self->getVoxel            = SSV_getVoxel_float_64_32;
      self->super.computeSample = SSV_sample_float_64_32;
    } else if (voxelType == VKL_DOUBLE) {
      self->getVoxel            = SSV_getVoxel_double_64_32;
      self->super.computeSample = SSV_sample_double_64_32;
    }
  } else {
    // in this case, even a single slice is too big to do 32-bit
    // addressing, and we have to do 64-bit throughout
    PRINT_DEBUG("#vkl:shared_structured_volume: using 64-bit mode\n");

    if (voxelType == VKL_UCHAR)
      self->getVoxel = SSV_getVoxel_uint8_64;
    else if (voxelType == VKL_SHORT)
      self->getVoxel = SSV_getVoxel_int16_64;
    else if (voxelType == VKL_USHORT)
      self->getVoxel = SSV_getVoxel_uint16_64;
    else if (voxelType == VKL_FLOAT)
      self->getVoxel = SSV_getVoxel_float_64;
    else if (voxelType == VKL_DOUBLE)
      self->getVoxel = SSV_getVoxel_double_64;
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
