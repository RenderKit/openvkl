// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#pragma once

#include <ospcommon/math/AffineSpace.h>
#include "TestingVolume.h"
#include "openvkl/vdb.h"
#include "openvkl/vdb_util/VdbVolumeBuffers.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    template <float samplingFunction(const vec3f &)>
    struct ProceduralVdbVolume : public TestingVolume
    {
      using Buffers = vdb_util::VdbVolumeBuffers<VKL_FLOAT>;

      ProceduralVdbVolume()                            = default;
      ProceduralVdbVolume(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume &operator=(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume(ProceduralVdbVolume &&)                 = default;
      ProceduralVdbVolume &operator=(ProceduralVdbVolume &&) = default;
      ~ProceduralVdbVolume()                                 = default;

      ProceduralVdbVolume(const vec3i &dimensions,
                          const vec3f &gridOrigin,
                          const vec3f &gridSpacing,
                          VKLFilter filter = VKL_FILTER_TRILINEAR)
          : buffers(new Buffers),
            filter(filter),
            dimensions(dimensions),
            gridOrigin(gridOrigin),
            gridSpacing(gridSpacing)

      {
        buffers->setIndexToObject(gridSpacing.x,
                                  0,
                                  0,
                                  0,
                                  gridSpacing.y,
                                  0,
                                  0,
                                  0,
                                  gridSpacing.z,
                                  gridOrigin.x,
                                  gridOrigin.y,
                                  gridOrigin.z);

        // The number of leaf nodes, in each dimension.
        const vec3i numLeafNodesIn(getNumLeaves(dimensions.x),
                                   getNumLeaves(dimensions.y),
                                   getNumLeaves(dimensions.z));

        const size_t numLeafNodes = numLeafNodesIn.x *
                                    static_cast<size_t>(numLeafNodesIn.y) *
                                    numLeafNodesIn.z;

        const uint32_t leafLevel   = vklVdbNumLevels() - 1;
        const uint32_t leafRes     = vklVdbLevelRes(leafLevel);
        const size_t numLeafVoxels = vklVdbLevelNumVoxels(leafLevel);

        // Temp buffer for leaf data.
        std::vector<float> leaf(numLeafVoxels);

        buffers->reserve(numLeafNodes);

        valueRange = range1f();
        for (int x = 0; x < numLeafNodesIn.x; ++x)
          for (int y = 0; y < numLeafNodesIn.y; ++y)
            for (int z = 0; z < numLeafNodesIn.z; ++z) {
              range1f leafValueRange;
              const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);
              for (uint32_t vx = 0; vx < leafRes; ++vx)
                for (uint32_t vy = 0; vy < leafRes; ++vy)
                  for (uint32_t vz = 0; vz < leafRes; ++vz) {
                    // Note: column major data!
                    const uint64_t idx =
                        static_cast<uint64_t>(vx) * leafRes * leafRes +
                        static_cast<uint64_t>(vy) * leafRes +
                        static_cast<uint64_t>(vz);

                    const vec3f samplePosIndex = vec3f(nodeOrigin.x + vx,
                                                       nodeOrigin.y + vy,
                                                       nodeOrigin.z + vz);
                    const vec3f samplePosObject =
                        transformLocalToObjectCoordinates(samplePosIndex);

                    const float fieldValue = samplingFunction(samplePosObject);
                    leaf.at(idx)           = fieldValue;
                    leafValueRange.extend(fieldValue);
                  }

              // Skip empty nodes.
              if (leafValueRange.lower != 0.f || leafValueRange.upper != 0.f) {
                // We compress constant areas of space into tiles.
                // We also copy all leaf data so that we do not have to
                // explicitly store it.
                if (std::fabs(leafValueRange.upper - leafValueRange.lower) <
                    std::fabs(leafValueRange.upper) *
                        std::numeric_limits<float>::epsilon()) {
                  buffers->addTile(
                      leafLevel, nodeOrigin, &leafValueRange.upper);
                } else
                  buffers->addConstant(
                      leafLevel, nodeOrigin, leaf.data(), VKL_DATA_DEFAULT);
              }
              valueRange.extend(leafValueRange);
            }
      }

      range1f getComputedValueRange() const override
      {
        return valueRange;
      }

      vec3i getDimensions() const
      {
        return dimensions;
      }

      vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const
      {
        return gridSpacing * localCoordinates + gridOrigin;
      }

      float computeProceduralValue(const vec3f &objectCoordinates)
      {
        return samplingFunction(objectCoordinates);
      }

     protected:
      void generateVKLVolume() override
      {
        if (buffers) {
          release();
          volume = buffers->createVolume(filter);
        }
      }

     private:
      int getNumLeaves(int res) const
      {
        const int shift = vklVdbLevelLogRes(vklVdbNumLevels() - 1);
        int numLeaves   = res >> shift;
        return numLeaves + ((numLeaves << shift) < res);
      }

     private:
      std::unique_ptr<Buffers> buffers;
      range1f valueRange;
      VKLFilter filter;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
    };

    using WaveletVdbVolume = ProceduralVdbVolume<getWaveletValue<float>>;

  }  // namespace testing
}  // namespace openvkl
