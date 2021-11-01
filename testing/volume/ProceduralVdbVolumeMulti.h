// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <numeric>
#include <vector>
#include "ProceduralVdbVolume.h"
#include "ProceduralVolumeMulti.h"
#include "TestingVolume.h"
#include "openvkl/vdb.h"
#include "rkcommon/common.h"

namespace openvkl {
  namespace testing {

    struct ProceduralVdbVolumeMulti : public TestingVolume,
                                      public ProceduralVolumeMulti
    {
      using Buffers = utility::vdb::VdbVolumeBuffers;

      ProceduralVdbVolumeMulti(
          VKLDevice device,
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const std::vector<std::shared_ptr<ProceduralVdbVolumeBase>>
              &attributeVolumes,
          VKLDataCreationFlags dataCreationFlags,
          bool useAOSLayout);

      // maps to first attribute only
      range1f getComputedValueRange() const override;

      range1f getComputedValueRange(unsigned int attributeIndex) const;

      vec3i getDimensions() const;
      vec3f getGridOrigin() const;
      vec3f getGridSpacing() const;

      unsigned int getNumAttributes() const override;

      vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const;

     protected:
      float computeProceduralValueImpl(const vec3f &objectCoordinates,
                                       unsigned int attributeIndex,
                                       float time) const override;

      vec3f computeProceduralGradientImpl(const vec3f &objectCoordinates,
                                          unsigned int attributeIndex,
                                          float time) const override;

      void generateVKLVolume(VKLDevice device) override final;

      std::unique_ptr<Buffers> buffers;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      std::vector<std::shared_ptr<ProceduralVdbVolumeBase>> attributeVolumes;
      VKLDataCreationFlags dataCreationFlags;
      bool useAOSLayout;

      // leaf data may need to be retained for shared data buffers
      std::vector<std::vector<unsigned char>> leaves;
      std::vector<std::vector<uint32_t>> indices;
      std::vector<std::vector<float>> times;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline ProceduralVdbVolumeMulti::ProceduralVdbVolumeMulti(
        VKLDevice device,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const std::vector<std::shared_ptr<ProceduralVdbVolumeBase>>
            &attributeVolumes,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
        : dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          attributeVolumes(attributeVolumes),
          dataCreationFlags(dataCreationFlags),
          useAOSLayout(useAOSLayout),
          ProceduralVolumeMulti(false)
    {
      if (attributeVolumes.size() == 0) {
        throw std::runtime_error("no provided attribute volumes");
      }

      const TemporalConfig temporalConfig =
          attributeVolumes[0]->getTemporalConfig();

      // verify provided attribute volumes are consistent with the provided
      // parameters
      for (const auto &v : attributeVolumes) {
        bool compatible = (v->getDimensions() == dimensions) &&
                          (v->getGridOrigin() == gridOrigin) &&
                          (v->getGridSpacing() == gridSpacing) &&
                          temporalConfig.isCompatible(v->getTemporalConfig());

        if (!compatible) {
          throw std::runtime_error(
              "a provided attribute volume is not compatible with the "
              "constructed ProceduralVdbVolumeMulti instance");
        }
      }

      // voxel type and size per attribute
      std::vector<VKLDataType> voxelTypes;
      std::vector<size_t> voxelSizes;

      for (const auto &av : attributeVolumes) {
        voxelTypes.push_back(av->getVoxelType());
        voxelSizes.push_back(sizeOfVKLDataType(av->getVoxelType()));
      }

      // voxel size for all attributes combined
      const size_t voxelSizeSum =
          std::accumulate(voxelSizes.begin(), voxelSizes.end(), 0);

      // voxel offset for each attribute
      std::vector<size_t> voxelSizeOffsets;
      for (size_t a = 0; a < attributeVolumes.size(); ++a) {
        voxelSizeOffsets.push_back(
            std::accumulate(voxelSizes.begin(), voxelSizes.begin() + a, 0));
      }

      buffers = rkcommon::make_unique<Buffers>(device, voxelTypes);

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

      for (int x = 0; x < numLeafNodesIn.x; ++x)
        for (int y = 0; y < numLeafNodesIn.y; ++y)
          for (int z = 0; z < numLeafNodesIn.z; ++z) {
            // Buffer for leaf data.
            leaves.emplace_back(
                std::vector<unsigned char>(numLeafVoxels * voxelSizeSum));
            std::vector<unsigned char> &leaf = leaves.back();

            std::vector<range1f> leafValueRanges(attributeVolumes.size());

            const vec3i nodeOrigin(leafRes * x, leafRes * y, leafRes * z);

            for (uint32_t vx = 0; vx < leafRes; ++vx)
              for (uint32_t vy = 0; vy < leafRes; ++vy)
                for (uint32_t vz = 0; vz < leafRes; ++vz) {
                  const vec3f samplePosIndex = vec3f(
                      nodeOrigin.x + vx, nodeOrigin.y + vy, nodeOrigin.z + vz);
                  const vec3f samplePosObject =
                      transformLocalToObjectCoordinates(samplePosIndex);

                  for (size_t a = 0; a < attributeVolumes.size(); ++a) {
                    // Note: column major data!
                    // Index in bytes
                    uint64_t idx;

                    if (useAOSLayout) {
                      idx = (static_cast<uint64_t>(vx) * leafRes * leafRes +
                             static_cast<uint64_t>(vy) * leafRes +
                             static_cast<uint64_t>(vz)) *
                                voxelSizeSum +
                            voxelSizeOffsets[a];
                    } else {
                      idx = voxelSizeOffsets[a] * numLeafVoxels +
                            (static_cast<uint64_t>(vx) * leafRes * leafRes +
                             static_cast<uint64_t>(vy) * leafRes +
                             static_cast<uint64_t>(vz)) *
                                voxelSizes[a];
                    }

                    const float fieldValue =
                        computeProceduralValue(samplePosObject, a);

                    if (voxelTypes[a] == VKL_HALF) {
                      half_float::half *leafValueTyped =
                          (half_float::half *)(leaf.data() + idx);
                      *leafValueTyped = fieldValue;
                    } else if (voxelTypes[a] == VKL_FLOAT) {
                      float *leafValueTyped = (float *)(leaf.data() + idx);
                      *leafValueTyped       = fieldValue;
                    } else {
                      throw std::runtime_error("unsupported voxel type");
                    }

                    leafValueRanges[a].extend(fieldValue);
                  }
                }

            bool nodeEmpty = true;

            for (size_t a = 0; a < attributeVolumes.size(); ++a) {
              if (leafValueRanges[a].lower != 0.f ||
                  leafValueRanges[a].upper != 0.f) {
                nodeEmpty = false;
                break;
              }
            }

            // Skip empty nodes (all attributes must be empty!).
            if (!nodeEmpty) {
              // We compress constant areas of space into tiles.
              // We also copy all leaf data so that we do not have to
              // explicitly store it.

              bool allConstant = true;

              for (size_t a = 0; a < attributeVolumes.size(); a++) {
                if (!(std::fabs(leafValueRanges[a].upper -
                                leafValueRanges[a].lower) <
                      std::fabs(leafValueRanges[a].upper) *
                          std::numeric_limits<float>::epsilon())) {
                  allConstant = false;
                  break;
                }
              }

              if (allConstant) {
                std::vector<unsigned char> tileValues(voxelSizeSum);
                std::vector<void *> ptrs(attributeVolumes.size());

                for (size_t a = 0; a < attributeVolumes.size(); a++) {
                  if (voxelTypes[a] == VKL_HALF) {
                    half_float::half *tileValueTyped =
                        (half_float::half *)(tileValues.data() +
                                             voxelSizeOffsets[a]);
                    *tileValueTyped =
                        half_float::half(leafValueRanges[a].upper);
                  } else if (voxelTypes[a] == VKL_FLOAT) {
                    float *tileValueTyped =
                        (float *)(tileValues.data() + voxelSizeOffsets[a]);
                    *tileValueTyped = leafValueRanges[a].upper;
                  } else {
                    throw std::runtime_error("unsupported voxel type");
                  }

                  ptrs[a] = tileValues.data() + voxelSizeOffsets[a];
                }

                buffers->addTile(leafLevel, nodeOrigin, ptrs);

              } else {
                std::vector<void *> ptrs;
                std::vector<size_t> byteStrides;

                if (useAOSLayout) {
                  for (size_t a = 0; a < attributeVolumes.size(); a++) {
                    ptrs.push_back(leaf.data() + voxelSizeOffsets[a]);
                    byteStrides.push_back(voxelSizeSum);
                  }
                } else {
                  for (size_t a = 0; a < attributeVolumes.size(); a++) {
                    ptrs.push_back(leaf.data() +
                                   numLeafVoxels * voxelSizeOffsets[a]);
                    byteStrides.push_back(voxelSizes[a]);
                  }
                }

                buffers->addConstant(leafLevel,
                                     nodeOrigin,
                                     ptrs,
                                     dataCreationFlags,
                                     byteStrides);
              }
            }

            if (dataCreationFlags != VKL_DATA_SHARED_BUFFER) {
              leaves.clear();
            }
          }
    }

    inline range1f ProceduralVdbVolumeMulti::getComputedValueRange() const
    {
      return attributeVolumes[0]->getComputedValueRange();
    }

    inline range1f ProceduralVdbVolumeMulti::getComputedValueRange(
        unsigned int attributeIndex) const
    {
      return attributeVolumes[attributeIndex]->getComputedValueRange();
    }

    inline float ProceduralVdbVolumeMulti::computeProceduralValueImpl(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralValue(
          objectCoordinates, time);
    }

    inline vec3f ProceduralVdbVolumeMulti::computeProceduralGradientImpl(
        const vec3f &objectCoordinates,
        unsigned int attributeIndex,
        float time) const
    {
      return attributeVolumes[attributeIndex]->computeProceduralGradient(
          objectCoordinates, time);
    }

    inline vec3i ProceduralVdbVolumeMulti::getDimensions() const
    {
      return dimensions;
    }

    inline vec3f ProceduralVdbVolumeMulti::getGridOrigin() const
    {
      return gridOrigin;
    }

    inline vec3f ProceduralVdbVolumeMulti::getGridSpacing() const
    {
      return gridSpacing;
    }

    inline unsigned int ProceduralVdbVolumeMulti::getNumAttributes() const
    {
      return attributeVolumes.size();
    }

    inline vec3f ProceduralVdbVolumeMulti::transformLocalToObjectCoordinates(
        const vec3f &localCoordinates) const
    {
      // at construction we're guaranteed all attribute volumes have the same
      // grid parameters, so we can simply do the transformation on the first
      // volume
      return attributeVolumes[0]->transformLocalToObjectCoordinates(
          localCoordinates);
    }

    inline void ProceduralVdbVolumeMulti::generateVKLVolume(VKLDevice device)
    {
      if (buffers) {
        release();

        if (device != buffers->getVKLDevice()) {
          throw std::runtime_error(
              "specified device not compatible with VdbVolumeBuffers device");
        }

        volume = buffers->createVolume();
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume generation helpers ///////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline ProceduralVdbVolumeMulti *generateMultiAttributeVdbVolumeHalf(
        VKLDevice device,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout,
        TemporalConfig temporalConfig = TemporalConfig())
    {
      // Not supported for multi attribute, as attributes share temporal config.
      temporalConfig.useTemporalCompression = false;

      std::vector<std::shared_ptr<ProceduralVdbVolumeBase>> volumes;

      volumes.push_back(
          std::make_shared<WaveletVdbVolumeHalf>(device,
                                                 dimensions,
                                                 gridOrigin,
                                                 gridSpacing,
                                                 temporalConfig));

      volumes.push_back(std::make_shared<XVdbVolumeHalf>(device,
                                                         dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         temporalConfig));

      volumes.push_back(std::make_shared<YVdbVolumeHalf>(device,
                                                         dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         temporalConfig));

      volumes.push_back(std::make_shared<ZVdbVolumeHalf>(device,
                                                         dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         temporalConfig));

      return new ProceduralVdbVolumeMulti(device,
                                          dimensions,
                                          gridOrigin,
                                          gridSpacing,
                                          volumes,
                                          dataCreationFlags,
                                          useAOSLayout);
    }

    inline ProceduralVdbVolumeMulti *generateMultiAttributeVdbVolumeFloat(
        VKLDevice device,
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout,
        TemporalConfig temporalConfig = TemporalConfig())
    {
      // Not supported for multi attribute, as attributes share temporal config.
      temporalConfig.useTemporalCompression = false;

      std::vector<std::shared_ptr<ProceduralVdbVolumeBase>> volumes;

      volumes.push_back(
          std::make_shared<WaveletVdbVolumeFloat>(device,
                                                  dimensions,
                                                  gridOrigin,
                                                  gridSpacing,
                                                  temporalConfig));

      volumes.push_back(std::make_shared<XVdbVolumeFloat>(device,
                                                          dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          temporalConfig));

      volumes.push_back(std::make_shared<YVdbVolumeFloat>(device,
                                                          dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          temporalConfig));

      volumes.push_back(std::make_shared<ZVdbVolumeFloat>(device,
                                                          dimensions,
                                                          gridOrigin,
                                                          gridSpacing,
                                                          temporalConfig));

      return new ProceduralVdbVolumeMulti(device,
                                          dimensions,
                                          gridOrigin,
                                          gridSpacing,
                                          volumes,
                                          dataCreationFlags,
                                          useAOSLayout);
    }

  }  // namespace testing
}  // namespace openvkl
