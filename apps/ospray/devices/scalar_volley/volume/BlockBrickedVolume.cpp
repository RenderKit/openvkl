// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "BlockBrickedVolume.h"
#include "../common/Data.h"
// ospcommon
#include <ospray/ospcommon/multidim_index_sequence.h>

namespace ospray {
  namespace scalar_volley_device {

    void BlockBrickedVolume::commit()
    {
      Volume::commit();

      dimensions  = getParam<vec3i>("dimensions", dimensions);
      gridOrigin  = getParam<vec3f>("gridOrigin", gridOrigin);
      gridSpacing = getParam<vec3f>("gridSpacing", gridSpacing);

      // compute required values
      boundingBox =
          box3f{gridOrigin, gridOrigin + vec3f{dimensions - 1} * gridSpacing};

      samplingStep = reduce_min(gridSpacing) * rcp(samplingRate);
    }

    bool BlockBrickedVolume::intersect(Ray &ray) const
    {
      auto hits = intersectBox(ray, boundingBox);

      if (hits.first < hits.second && hits.first < ray.t) {
        ray.t0 = hits.first;
        ray.t  = hits.second;
        return true;
      } else {
        return false;
      }
    }

    float BlockBrickedVolume::computeSample(const vec3f &worldCoordinates) const
    {
      vec3f localCoordinates = transformWorldToLocal(worldCoordinates);

      const vec3f clampedLocalCoordinates =
          clamp(localCoordinates, vec3f{0.0f}, localCoordinatesUpperBound);

      // Lower and upper corners of the box straddling the voxels to be
      // interpolated. "vi" means "voxelIndex"
      const vec3i vi_0 = vec3i(clampedLocalCoordinates);
      const vec3i vi_1 = vi_0 + 1;

      // Fractional coordinates within the lower corner voxel used during
      // interpolation. "flc" means "fractionalLocalCoordinates"
      const vec3f flc = clampedLocalCoordinates - vec3f(vi_0);

      // Look up the voxel values to be interpolated. "vv" means "voxelValue"
      float vv_000 = getVoxel(vec3i{vi_0.x, vi_0.y, vi_0.z});
      float vv_001 = getVoxel(vec3i{vi_1.x, vi_0.y, vi_0.z});
      float vv_010 = getVoxel(vec3i{vi_0.x, vi_1.y, vi_0.z});
      float vv_011 = getVoxel(vec3i{vi_1.x, vi_1.y, vi_0.z});
      float vv_100 = getVoxel(vec3i{vi_0.x, vi_0.y, vi_1.z});
      float vv_101 = getVoxel(vec3i{vi_1.x, vi_0.y, vi_1.z});
      float vv_110 = getVoxel(vec3i{vi_0.x, vi_1.y, vi_1.z});
      float vv_111 = getVoxel(vec3i{vi_1.x, vi_1.y, vi_1.z});

      // Interpolate the voxel values.
      const float vv_00        = vv_000 + flc.x * (vv_001 - vv_000);
      const float vv_01        = vv_010 + flc.x * (vv_011 - vv_010);
      const float vv_10        = vv_100 + flc.x * (vv_101 - vv_100);
      const float vv_11        = vv_110 + flc.x * (vv_111 - vv_110);
      const float vv_0         = vv_00 + flc.y * (vv_01 - vv_00);
      const float vv_1         = vv_10 + flc.y * (vv_11 - vv_10);
      const float volumeSample = vv_0 + flc.z * (vv_1 - vv_0);

      return volumeSample;
    }

    void BlockBrickedVolume::advance(Ray &ray) const
    {
      ray.t0 += samplingStep;
    }

    int BlockBrickedVolume::setRegion(const void *source,
                                      const vec3i &start,
                                      const vec3i &count)
    {
      auto numCurrentVoxels = data.size();
      auto maxInputVoxels   = vec3ul(start + count).product();
      if (numCurrentVoxels < maxInputVoxels) {
        data.resize(maxInputVoxels);
        dimensions = start + count;
      }

      // TODO: assumes floats only!!!
      multidim_index_sequence<3> input_indices(count);
      multidim_index_sequence<3> volume_indices(dimensions);

      for (auto idx : input_indices) {
        auto src  = input_indices.flatten(idx);
        auto dst  = volume_indices.flatten(start + idx);
        data[dst] = ((float *)source)[src];
      }

      return 0;
    }

    float BlockBrickedVolume::getVoxel(const vec3i &index) const
    {
      const size_t i = index.z * (dimensions.y * dimensions.x) +
                       index.y * dimensions.x + index.x;
      return data[i];
    }

    vec3f BlockBrickedVolume::transformLocalToWorld(
        const vec3f &localCoordinates) const
    {
      return gridOrigin + localCoordinates * gridSpacing;
    }

    vec3f BlockBrickedVolume::transformWorldToLocal(
        const vec3f &worldCoordinates) const
    {
      return rcp(gridSpacing) * (worldCoordinates - gridOrigin);
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
