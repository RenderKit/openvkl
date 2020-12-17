// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/AffineSpace.h>
#include "ProceduralVolume.h"
#include "TestingVolume.h"
#include "openvkl/vdb.h"
#include "openvkl/vdb_util/VdbVolumeBuffers.h"

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    template <float samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float) =
                  gradientNotImplemented>
    struct ProceduralVdbVolume : public TestingVolume, public ProceduralVolume
    {
      using Buffers = vdb_util::VdbVolumeBuffers<VKL_FLOAT>;

      ProceduralVdbVolume()                            = default;
      ProceduralVdbVolume(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume &operator=(const ProceduralVdbVolume &) = delete;
      ProceduralVdbVolume(ProceduralVdbVolume &&)                 = default;
      ProceduralVdbVolume &operator=(ProceduralVdbVolume &&) = default;
      ~ProceduralVdbVolume()                                 = default;

      ProceduralVdbVolume(
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLFilter filter                       = VKL_FILTER_TRILINEAR,
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0)
          : ProceduralVolume(false),
            buffers(new Buffers),
            dimensions(dimensions),
            gridOrigin(gridOrigin),
            gridSpacing(gridSpacing),
            filter(filter),
            dataCreationFlags(dataCreationFlags),
            byteStride(byteStride)

      {
        if (byteStride == 0) {
          byteStride = sizeOfVKLDataType(VKL_FLOAT);
        }

        if (byteStride < sizeOfVKLDataType(VKL_FLOAT)) {
          throw std::runtime_error("byteStride must be >= size of voxel type");
        }

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

        buffers->reserve(numLeafNodes);

        valueRange = range1f();
        for (int x = 0; x < numLeafNodesIn.x; ++x)
          for (int y = 0; y < numLeafNodesIn.y; ++y)
            for (int z = 0; z < numLeafNodesIn.z; ++z) {
              // Buffer for leaf data.
              leaves.emplace_back(
                  std::vector<unsigned char>(numLeafVoxels * byteStride));
              std::vector<unsigned char> &leaf = leaves.back();

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

                    const float fieldValue =
                        samplingFunction(samplePosObject, 0.f);

                    float *leafValueTyped =
                        (float *)(leaf.data() + idx * byteStride);
                    *leafValueTyped = fieldValue;

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
                  buffers->addConstant(leafLevel,
                                       nodeOrigin,
                                       leaf.data(),
                                       dataCreationFlags,
                                       byteStride);
              }
              valueRange.extend(leafValueRange);

              if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
                leaves.clear();
              }
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

     protected:
      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       float time) const override
      {
        return samplingFunction(objectCoordinates, time);
      }

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          float time) const override
      {
        return gradientFunction(objectCoordinates, time);
      }

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
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLFilter filter;
      VKLDataCreationFlags dataCreationFlags;
      size_t byteStride;

      // leaf data may need to be retained for shared data buffers
      std::vector<std::vector<unsigned char>> leaves;
    };

    using WaveletVdbVolume =
        ProceduralVdbVolume<getWaveletValue<float>, getWaveletGradient>;

    using XYZVdbVolume =
        ProceduralVdbVolume<getXYZValue<float>, getXYZGradient>;

    using ZVdbVolume = ProceduralVdbVolume<getZValue, getZGradient>;

    using SphereVdbVolume = ProceduralVdbVolume<getRotatingSphereValue<float>,
                                                getRotatingSphereGradient>;

  }  // namespace testing
}  // namespace openvkl
