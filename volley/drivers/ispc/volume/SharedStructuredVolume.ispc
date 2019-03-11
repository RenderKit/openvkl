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

#include "math/vec.ih"
#include "volley/volley_volume.isph"

struct SharedStructuredVolume
{
  float *uniform volumeData;
  uniform vec3i dimensions;
  uniform vec3f gridOrigin;
  uniform vec3f gridSpacing;

  uniform vec3f localCoordinatesUpperBound;
};

export void *uniform SharedStructuredVolume_Constructor()
{
  uniform SharedStructuredVolume *uniform self =
      uniform new uniform SharedStructuredVolume;
  return self;
}

export void *uniform SharedStructuredVolume_Destructor(void *uniform _self)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  delete self;
}

export void SharedStructuredVolume_set(void *uniform _self,
                                       float *uniform volumeData,
                                       const uniform vec3i &dimensions,
                                       const uniform vec3f &gridOrigin,
                                       const uniform vec3f &gridSpacing)
{
  uniform SharedStructuredVolume *uniform self =
      (uniform SharedStructuredVolume * uniform) _self;

  self->volumeData  = volumeData;
  self->dimensions  = dimensions;
  self->gridOrigin  = gridOrigin;
  self->gridSpacing = gridSpacing;

  self->localCoordinatesUpperBound =
      nextafter(self->dimensions - 1, make_vec3i(0));
}

inline void SharedStructuredVolume_getVoxel(void *uniform _self,
                                            const varying vec3i &index,
                                            varying float &value)
{ /* Cast to the actual Volume subtype. */
  SharedStructuredVolume *uniform self =
      (SharedStructuredVolume * uniform) _self;

  /* Cast to the actual voxel type. */
  const float *uniform volumeData = (const float *uniform)self->volumeData;
  const uint32 addr =
      index.x + self->dimensions.x * (index.y + self->dimensions.y * index.z);

  /* The voxel value at the given index. */
  value = volumeData[addr];
}

inline void SharedStructuredVolume_transformObjectToLocal(
    SharedStructuredVolume *uniform volume,
    const varying vec3f &objectCoordinates,
    varying vec3f &localCoordinates)
{
  localCoordinates =
      rcp(volume->gridSpacing) * (objectCoordinates - volume->gridOrigin);
}

inline varying float SharedStructuredVolume_sample(
    void *uniform _volume,
    const varying vec3f &objectCoordinates,
    varying float &volumeSample)
{
  // Cast to the actual Volume subtype.
  SharedStructuredVolume *uniform volume =
      (SharedStructuredVolume * uniform) _volume;

  // Transform the sample location into the local coordinate system.
  vec3f localCoordinates;
  SharedStructuredVolume_transformObjectToLocal(
      volume, objectCoordinates, localCoordinates);

  // Coordinates outside the volume are clamped to the volume bounds.
  const vec3f clampedLocalCoordinates = clamp(
      localCoordinates, make_vec3f(0.0f), volume->localCoordinatesUpperBound);

  // Lower and upper corners of the box straddling the voxels to be
  // interpolated.
  const vec3i voxelIndex_0 = to_int(clampedLocalCoordinates);
  const vec3i voxelIndex_1 = voxelIndex_0 + 1;

  // Fractional coordinates within the lower corner voxel used during
  // interpolation.
  const vec3f fractionalLocalCoordinates =
      clampedLocalCoordinates - to_float(voxelIndex_0);

  // Look up the voxel values to be interpolated.
  float voxelValue_000;
  float voxelValue_001;
  float voxelValue_010;
  float voxelValue_011;
  float voxelValue_100;
  float voxelValue_101;
  float voxelValue_110;
  float voxelValue_111;
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_0.z),
      voxelValue_000);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_0.z),
      voxelValue_001);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_0.z),
      voxelValue_010);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_0.z),
      voxelValue_011);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_0.x, voxelIndex_0.y, voxelIndex_1.z),
      voxelValue_100);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_1.x, voxelIndex_0.y, voxelIndex_1.z),
      voxelValue_101);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_0.x, voxelIndex_1.y, voxelIndex_1.z),
      voxelValue_110);
  SharedStructuredVolume_getVoxel(
      volume,
      make_vec3i(voxelIndex_1.x, voxelIndex_1.y, voxelIndex_1.z),
      voxelValue_111);

  // Interpolate the voxel values.
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

  volumeSample = voxelValue_0 +
                 fractionalLocalCoordinates.z * (voxelValue_1 - voxelValue_0);
}

export void SharedStructuredVolume_sample_export(
    uniform const int *uniform imask,
    void *uniform _volume,
    void *uniform _objectCoordinates,
    void *uniform _samples)
{
  cif(imask[programIndex])
  {
    const varying vec3f *uniform objectCoordinates =
        (const varying vec3f *uniform)_objectCoordinates;
    varying float *uniform samples = (varying float *uniform)_samples;

    SharedStructuredVolume_sample(_volume, *objectCoordinates, *samples);
  }
}
