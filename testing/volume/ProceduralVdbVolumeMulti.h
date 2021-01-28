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
      using Buffers = vdb_util::VdbVolumeBuffers;

      ProceduralVdbVolumeMulti(
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          VKLFilter filter,
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

      void generateVKLVolume() override final;

      std::unique_ptr<Buffers> buffers;
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      VKLFilter filter;
      std::vector<std::shared_ptr<ProceduralVdbVolumeBase>> attributeVolumes;
      VKLDataCreationFlags dataCreationFlags;
      bool useAOSLayout;

      // leaf data may need to be retained for shared data buffers
      std::vector<std::vector<unsigned char>> leaves;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline ProceduralVdbVolumeMulti::ProceduralVdbVolumeMulti(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLFilter filter,
        const std::vector<std::shared_ptr<ProceduralVdbVolumeBase>>
            &attributeVolumes,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
        : dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing),
          filter(filter),
          attributeVolumes(attributeVolumes),
          dataCreationFlags(dataCreationFlags),
          useAOSLayout(useAOSLayout),
          ProceduralVolumeMulti(false)
    {
      if (attributeVolumes.size() == 0) {
        throw std::runtime_error("no provided attribute volumes");
      }

      // verify provided attribute volumes are consistent with the provided
      // parameters
      for (const auto &v : attributeVolumes) {
        bool compatible = (v->getDimensions() == dimensions) &&
                          (v->getGridOrigin() == gridOrigin) &&
                          (v->getGridSpacing() == gridSpacing);

        if (!compatible) {
          throw std::runtime_error(
              "a provided attribute volume is not compatible with the "
              "constructed ProceduralVdbVolumeMulti instance");
        }
      }

      buffers = rkcommon::make_unique<Buffers>(
          std::vector<VKLDataType>(attributeVolumes.size(), VKL_FLOAT));

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
            leaves.emplace_back(std::vector<unsigned char>(
                numLeafVoxels * attributeVolumes.size() * sizeof(float)));
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
                    uint64_t idx;

                    if (useAOSLayout) {
                      idx = (static_cast<uint64_t>(vx) * leafRes * leafRes +
                             static_cast<uint64_t>(vy) * leafRes +
                             static_cast<uint64_t>(vz)) *
                                attributeVolumes.size() +
                            a;
                    } else {
                      idx = a * numLeafVoxels +
                            static_cast<uint64_t>(vx) * leafRes * leafRes +
                            static_cast<uint64_t>(vy) * leafRes +
                            static_cast<uint64_t>(vz);
                    }

                    const float fieldValue =
                        computeProceduralValue(samplePosObject, a);

                    float *leafValueTyped =
                        (float *)(leaf.data() + idx * sizeof(float));
                    *leafValueTyped = fieldValue;

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
                std::vector<float> tileValues(attributeVolumes.size());
                std::vector<void *> ptrs(attributeVolumes.size());

                for (size_t a = 0; a < attributeVolumes.size(); a++) {
                  tileValues[a] = leafValueRanges[a].upper;
                  ptrs[a]       = tileValues.data() + a;
                }

                buffers->addTile(leafLevel, nodeOrigin, ptrs);

              } else {
                std::vector<void *> ptrs;
                std::vector<size_t> byteStrides;

                if (useAOSLayout) {
                  for (size_t a = 0; a < attributeVolumes.size(); a++) {
                    ptrs.push_back(leaf.data() + sizeof(float) * a);
                    byteStrides.push_back(attributeVolumes.size() *
                                          sizeof(float));
                  }
                } else {
                  for (size_t a = 0; a < attributeVolumes.size(); a++) {
                    ptrs.push_back(leaf.data() +
                                   numLeafVoxels * sizeof(float) * a);
                    byteStrides.push_back(sizeof(float));
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

    inline void ProceduralVdbVolumeMulti::generateVKLVolume()
    {
      if (buffers) {
        release();
        volume = buffers->createVolume(filter);
      }
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume generation helpers ///////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    inline ProceduralVdbVolumeMulti *generateMultiAttributeVdbVolume(
        const vec3i &dimensions,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        VKLFilter filter,
        VKLDataCreationFlags dataCreationFlags,
        bool useAOSLayout)
    {
      std::vector<std::shared_ptr<ProceduralVdbVolumeBase>> volumes;

      volumes.push_back(std::make_shared<WaveletVdbVolume>(
          dimensions, gridOrigin, gridSpacing));

      volumes.push_back(
          std::make_shared<XVdbVolume>(dimensions, gridOrigin, gridSpacing));

      volumes.push_back(
          std::make_shared<YVdbVolume>(dimensions, gridOrigin, gridSpacing));

      volumes.push_back(
          std::make_shared<ZVdbVolume>(dimensions, gridOrigin, gridSpacing));

      return new ProceduralVdbVolumeMulti(dimensions,
                                          gridOrigin,
                                          gridSpacing,
                                          filter,
                                          volumes,
                                          dataCreationFlags,
                                          useAOSLayout);
    }

  }  // namespace testing
}  // namespace openvkl
