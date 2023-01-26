// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"
#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace utility {
    namespace vdb {

    using vec3i         = rkcommon::math::vec3i;
    using vec3f         = rkcommon::math::vec3f;
    using AffineSpace3f = rkcommon::math::AffineSpace3f;
    using LinearSpace3f = rkcommon::math::LinearSpace3f;

    /*
     * These are all the buffers we need. We will create VKLData objects
     * from these buffers, and then set those as parameters on the
     * VKLVolume object.
     */
    struct VdbVolumeBuffers
    {
      /*
       * Construction / destruction.
       * If repackNodes is true, node data will be re-arranged for a more
       * optimal memory layout; this option is only compatible with temporally
       * constant volumes.
       */
      VdbVolumeBuffers(VKLDevice device,
                       const std::vector<VKLDataType> &attributeDataTypes,
                       bool repackNodes = false);
      ~VdbVolumeBuffers();

      VdbVolumeBuffers(const VdbVolumeBuffers &) = delete;
      VdbVolumeBuffers(VdbVolumeBuffers &&)      = delete;
      VdbVolumeBuffers &operator=(const VdbVolumeBuffers &) = delete;
      VdbVolumeBuffers &operator=(VdbVolumeBuffers &&) = delete;

      /*
       * Access to the index to object transformation matrix.
       */
      void setIndexToObject(float l00,
                            float l01,
                            float l02,
                            float l10,
                            float l11,
                            float l12,
                            float l20,
                            float l21,
                            float l22,
                            float p0,
                            float p1,
                            float p2);

      void setActiveVoxelsBoundingBox(const box3i &bbox);

      size_t numNodes() const;

      /*
       * Clear all buffers.
       */
      void clear();

      /*
       * Preallocate memory for dense leaf and tile nodes.
       * This helps reduce load times because only one allocation needs to be
       * made.
       */
      void reserve(size_t numDenseNodes, size_t numTileNodes);

      /*
       * Add a new tile node.
       * Returns the new node's index.
       */
      size_t addTile(uint32_t level,
                     const vec3i &origin,
                     const std::vector<void *> &ptrs,
                     uint32_t temporallyStructuredNumTimesteps     = 0,
                     uint32_t temporallyUnstructuredNumIndices     = 0,
                     const uint32_t *temporallyUnstructuredIndices = nullptr,
                     const float *temporallyUnstructuredTimes      = nullptr);

      /*
       * Add a new constant node.
       * Returns the new node's index.
       */
      size_t addConstant(
          uint32_t level,
          const vec3i &origin,
          const std::vector<void *> &ptrs,
          VKLDataCreationFlags flags,
          const std::vector<size_t> &byteStrides        = {},
          uint32_t temporallyStructuredNumTimesteps     = 0,
          uint32_t temporallyUnstructuredNumIndices     = 0,
          const uint32_t *temporallyUnstructuredIndices = nullptr,
          const float *temporallyUnstructuredTimes      = nullptr);

      /*
       * Change the given node to a constant node.
       * This is useful for deferred loading. nodeIndex and denseNodeIndex may be
       * different when repackNodes is enabled.
       */
      void makeConstant(size_t nodeIndex,
                        size_t denseNodeIndex,
                        const std::vector<void *> &ptrs,
                        VKLDataCreationFlags flags,
                        const std::vector<size_t> &byteStrides        = {},
                        uint32_t temporallyStructuredNumTimesteps     = 0,
                        uint32_t temporallyUnstructuredNumIndices     = 0,
                        const uint32_t *temporallyUnstructuredIndices = nullptr,
                        const float *temporallyUnstructuredTimes = nullptr);

      /*
       * Create a VKLVolume from these buffers.
       * If commit is true, the volume will be committed. Otherwise, the
       * application will need to commit the volume before use.
       */
      VKLVolume createVolume(bool commit = true) const;

      /*
       * Indicates if data provided to this object (via `addConstant()` or
       * `makeConstant()`) is being shared (without a copy made) with the
       * created VKLVolume. Note that if repackNodes is enabled, data may not be
       * directly shared with Open VKL even if requested. If data is not being
       * shared, any source data may be deleted; otherwise that source data must
       * be retained as long as the VKLVolume is active.
       */
      bool usingSharedData() const;

      VKLDevice getVKLDevice() const;

     private:
      /*
       * The Open VKL device where we are creating the volume.
       */
      VKLDevice device;

      /*
       * The data type for each scalar attribute.
       */
      std::vector<VKLDataType> attributeDataTypes;

      /*
       * The element size for each scalar attribute.
       */
      std::vector<size_t> attributeElementSizes;

      /*
       * Whether to re-arrange node data for a more optimal memory layout; this
       * option is only compatible with temporally constant volumes.
       */
      bool repackNodes;

      /*
       * The grid transform (index space to object space).
       */
      AffineSpace3f indexToObject = AffineSpace3f(one);

      box3i activeVoxelsBoundingBox = empty;

      /*
       * Level must be a number in [1, VKL_VDB_NUM_LEVELS-1].
       * The level also influences the node resolution. Constant
       * nodes on a level cover a domain of vklVdbLevelRes(level)^3
       * voxels.
       */
      std::vector<uint32_t> level;

      /*
       * The node origin.
       */
      std::vector<vec3i> origin;

      /*
       * The node format. This can be VKL_FORMAT_TILE or
       * VKL_FORMAT_DENSE_ZYX at this point.
       */
      std::vector<VKLFormat> format;

      /*
       * Temporal config. All of these buffers are optional, but we allocate
       * them for simplicity.
       */
      std::vector<VKLTemporalFormat> temporalFormat;
      std::vector<uint32_t> temporallyStructuredNumTimesteps;
      std::vector<VKLData> temporallyUnstructuredIndices;
      std::vector<VKLData> temporallyUnstructuredTimes;

      /*
       * Track current number of dense nodes and tile nodes separately; this is
       * used when repackNodes is true.
       */
      size_t numDenseNodes = 0;
      size_t numTileNodes = 0;

      /*
       * The actual node data, used when repackNodes is false. Tiles have
       * exactly one value, constant nodes have vklVdbLevelRes(level)^3 =
       * vklVdbLevelNumVoxels(level) values.
       */
      std::vector<VKLData> data;

      /*
       * The actual node data, used when repackNodes is true. Node data is
       * stored in a single contiguous array (per attribute, separately for
       * dense nodes and tiles), which can improve performance. Tiles have exactly
       * one value, constant nodes have vklVdbLevelRes(level)^3 =
       * vklVdbLevelNumVoxels(level) values.
       */
      std::vector<std::vector<char>> repackedDenseNodes;
      std::vector<std::vector<char>> repackedTiles;

      /*
       * Whether data provided (via `addConstant()` or `makeConstant()` is being
       * shared directly with the created VKLVolume.
       */
      bool isUsingSharedData = false;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline VdbVolumeBuffers::VdbVolumeBuffers(
        VKLDevice device,
        const std::vector<VKLDataType> &attributeDataTypes,
        bool repackNodes)
        : device(device),
          attributeDataTypes(attributeDataTypes),
          repackNodes(repackNodes)
    {
      for (const auto &dt : attributeDataTypes) {
        if (dt == VKL_HALF) {
          attributeElementSizes.push_back(sizeof(uint16_t));
        } else if (dt == VKL_FLOAT) {
          attributeElementSizes.push_back(sizeof(float));
        } else {
          throw std::runtime_error(
              "vdb volumes only support VKL_HALF and VKL_FLOAT attributes");
        }
      }

      if (repackNodes) {
        repackedDenseNodes.resize(attributeDataTypes.size());
        repackedTiles.resize(attributeDataTypes.size());
      }
    }

    inline VdbVolumeBuffers::~VdbVolumeBuffers()
    {
      clear();
    }

    inline void VdbVolumeBuffers::setIndexToObject(float l00,
                                                   float l01,
                                                   float l02,
                                                   float l10,
                                                   float l11,
                                                   float l12,
                                                   float l20,
                                                   float l21,
                                                   float l22,
                                                   float p0,
                                                   float p1,
                                                   float p2)
    {
      indexToObject.l = LinearSpace3f(
          vec3f(l00, l01, l02), vec3f(l10, l11, l12), vec3f(l20, l21, l22));
      indexToObject.p = vec3f(p0, p1, p2);
    }

    inline void  VdbVolumeBuffers::setActiveVoxelsBoundingBox(const box3i &bbox)
    {
      activeVoxelsBoundingBox = bbox;
    }

    inline size_t VdbVolumeBuffers::numNodes() const
    {
      assert(numDenseNodes + numTileNodes == level.size());
      return level.size();
    }

    inline void VdbVolumeBuffers::clear()
    {
      for (VKLData d : data)
        vklRelease(d);
      level.clear();
      origin.clear();
      format.clear();
      temporalFormat.clear();
      temporallyStructuredNumTimesteps.clear();
      temporallyUnstructuredIndices.clear();
      temporallyUnstructuredTimes.clear();
      data.clear();

      numDenseNodes = 0;
      numTileNodes = 0;

      if (repackNodes) {
        repackedDenseNodes.clear();
        repackedTiles.clear();
        repackedDenseNodes.resize(attributeDataTypes.size());
        repackedTiles.resize(attributeDataTypes.size());
      }
    }

    inline void VdbVolumeBuffers::reserve(size_t numDenseNodes,
                                          size_t numTileNodes)
    {
      assert(level.empty());
      assert(origin.empty());
      assert(format.empty());
      assert(temporalFormat.empty());
      assert(temporallyStructuredNumTimesteps.empty());
      assert(temporallyUnstructuredIndices.empty());
      assert(temporallyUnstructuredTimes.empty());
      assert(data.empty());

      for (const auto &r : repackedDenseNodes) {
        assert(r.empty());
      }

      for (const auto &r : repackedTiles) {
        assert(r.empty());
      }

      const size_t numNodes = numDenseNodes + numTileNodes;

      level.reserve(numNodes);
      origin.reserve(numNodes);
      format.reserve(numNodes);
      temporalFormat.reserve(numNodes);
      temporallyStructuredNumTimesteps.reserve(numNodes);
      temporallyUnstructuredIndices.reserve(numNodes);
      temporallyUnstructuredTimes.reserve(numNodes);

      if (repackNodes) {
        assert(attributeDataTypes.size() == attributeElementSizes.size());

        if (numDenseNodes > 0) {
          assert(repackedDenseNodes.size() == attributeDataTypes.size());

          for (size_t a = 0; a < attributeDataTypes.size(); a++) {
            // dense nodes only exist at the deepest level
            repackedDenseNodes[a].resize(
                numDenseNodes * attributeElementSizes[a] *
                vklVdbLevelNumVoxels(VKL_VDB_NUM_LEVELS - 1));
          }
        }

        if (numTileNodes > 0) {
          assert(repackedTiles.size() == attributeDataTypes.size());

          for (size_t a = 0; a < attributeDataTypes.size(); a++) {
            repackedTiles[a].resize(numTileNodes * attributeElementSizes[a]);
          }
        }

      } else {
        data.reserve(numNodes);
      }
    }

    inline size_t VdbVolumeBuffers::addTile(
        uint32_t level,
        const vec3i &origin,
        const std::vector<void *> &ptrs,
        uint32_t temporallyStructuredNumTimesteps,
        uint32_t temporallyUnstructuredNumIndices,
        const uint32_t *temporallyUnstructuredIndices,
        const float *temporallyUnstructuredTimes)
    {
      if (ptrs.size() != attributeDataTypes.size()) {
        throw std::runtime_error(
            "addTile() called with incorrect number of pointers");
      }

      const size_t index = repackNodes ? numTileNodes : numNodes();
      this->level.push_back(level);
      this->origin.push_back(origin);
      format.push_back(VKL_FORMAT_TILE);

      uint32_t dataSize = 1;
      if (temporallyStructuredNumTimesteps > 1) {
        this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_STRUCTURED);
        this->temporallyStructuredNumTimesteps.push_back(
            temporallyStructuredNumTimesteps);
        this->temporallyUnstructuredIndices.push_back(VKLData());
        this->temporallyUnstructuredTimes.push_back(VKLData());
        dataSize = temporallyStructuredNumTimesteps;
      } else if (temporallyUnstructuredIndices && temporallyUnstructuredTimes) {
        this->temporallyStructuredNumTimesteps.push_back(0);
        this->temporallyUnstructuredIndices.push_back(VKLData());
        this->temporallyUnstructuredTimes.push_back(VKLData());
        throw std::runtime_error("NOT IMPLEMENTED");
        assert(false);
      } else {
        this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_CONSTANT);
        this->temporallyStructuredNumTimesteps.push_back(0);
        this->temporallyUnstructuredIndices.push_back(VKLData());
        this->temporallyUnstructuredTimes.push_back(VKLData());
      }

      if (repackNodes) {
        if (this->temporalFormat.back() != VKL_TEMPORAL_FORMAT_CONSTANT) {
          throw std::runtime_error(
              "repackNodes only supported with temporally constant data");
        }

        for (size_t a = 0; a < attributeDataTypes.size(); a++) {
          const size_t elementSize = attributeElementSizes[a];
          const size_t nodeSize = elementSize;

          const size_t requiredSize = (index + 1) * nodeSize;

          if (repackedTiles[a].size() < requiredSize) {
            static bool warnOnce = false;

            if (!warnOnce) {
              std::cerr
                  << "VdbVolumeBuffers: resizing tile node memory; this is "
                     "slow, use reserve() to pre-allocate memory!"
                  << std::endl;
              warnOnce = true;
            }

            repackedTiles[a].resize(requiredSize);
          }

          std::memcpy(
              repackedTiles[a].data() + index * nodeSize, ptrs[a], nodeSize);
        }

      } else {
        // for default (not repacked) data

        // only use array-of-arrays when we have multiple attributes
        if (ptrs.size() == 1) {
          data.push_back(vklNewData(device,
                                    dataSize,
                                    attributeDataTypes[0],
                                    ptrs[0],
                                    VKL_DATA_DEFAULT));
        } else {
          std::vector<VKLData> attributesData;

          for (size_t i = 0; i < ptrs.size(); i++) {
            attributesData.push_back(vklNewData(device,
                                                dataSize,
                                                attributeDataTypes[i],
                                                ptrs[i],
                                                VKL_DATA_DEFAULT));
          }

          data.push_back(vklNewData(device,
                                    attributesData.size(),
                                    VKL_DATA,
                                    attributesData.data(),
                                    VKL_DATA_DEFAULT));

          for (size_t i = 0; i < attributesData.size(); i++) {
            vklRelease(attributesData[i]);
          }
        }
      }

      numTileNodes++;

      return index;
    }

    inline size_t VdbVolumeBuffers::addConstant(
        uint32_t level,
        const vec3i &origin,
        const std::vector<void *> &ptrs,
        VKLDataCreationFlags flags,
        const std::vector<size_t> &byteStrides,
        uint32_t temporallyStructuredNumTimesteps,
        uint32_t temporallyUnstructuredNumIndices,
        const uint32_t *temporallyUnstructuredIndices,
        const float *temporallyUnstructuredTimes)
    {
      if (ptrs.size() != attributeDataTypes.size()) {
        throw std::runtime_error(
            "addConstant() called with incorrect number of pointers");
      }

      if (byteStrides.size() &&
          byteStrides.size() != attributeDataTypes.size()) {
        throw std::runtime_error(
            "addConstant() called with incorrect number of byteStrides");
      }

      const size_t nodeIndex = numNodes();
      const size_t denseNodeIndex = repackNodes ? numDenseNodes : numNodes();
      this->level.push_back(level);
      this->origin.push_back(origin);
      format.push_back(VKL_FORMAT_INVALID);
      this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_INVALID);
      this->temporallyStructuredNumTimesteps.push_back(0);
      this->temporallyUnstructuredIndices.push_back(VKLData());
      this->temporallyUnstructuredTimes.push_back(VKLData());

      if (!repackNodes) {
        data.push_back(VKLData());
      }

      makeConstant(nodeIndex,
                   denseNodeIndex,
                   ptrs,
                   flags,
                   byteStrides,
                   temporallyStructuredNumTimesteps,
                   temporallyUnstructuredNumIndices,
                   temporallyUnstructuredIndices,
                   temporallyUnstructuredTimes);

      numDenseNodes++;

      return nodeIndex;
    }

    inline void VdbVolumeBuffers::makeConstant(
        size_t nodeIndex,
        size_t denseNodeIndex,
        const std::vector<void *> &ptrs,
        VKLDataCreationFlags flags,
        const std::vector<size_t> &byteStrides,
        uint32_t temporallyStructuredNumTimesteps,
        uint32_t temporallyUnstructuredNumIndices,
        const uint32_t *temporallyUnstructuredIndices,
        const float *temporallyUnstructuredTimes)
    {
      if (ptrs.size() != attributeDataTypes.size()) {
        throw std::runtime_error(
            "makeConstant() called with incorrect number of pointers");
      }

      if (byteStrides.size() &&
          byteStrides.size() != attributeDataTypes.size()) {
        throw std::runtime_error(
            "makeConstant() called with incorrect number of byteStrides");
      }

      format.at(nodeIndex) = VKL_FORMAT_DENSE_ZYX;

      // dense nodes only exist at the deepest level
      assert(level.at(nodeIndex) == VKL_VDB_NUM_LEVELS - 1);

      uint32_t dataSize = vklVdbLevelNumVoxels(level.at(nodeIndex));
      if (temporallyStructuredNumTimesteps > 1) {
        this->temporalFormat.at(nodeIndex) = VKL_TEMPORAL_FORMAT_STRUCTURED;
        this->temporallyStructuredNumTimesteps.at(nodeIndex) =
            temporallyStructuredNumTimesteps;
        dataSize *= temporallyStructuredNumTimesteps;
      } else if ((temporallyUnstructuredNumIndices > 0) &&
                 temporallyUnstructuredIndices && temporallyUnstructuredTimes) {
        this->temporalFormat.at(nodeIndex) = VKL_TEMPORAL_FORMAT_UNSTRUCTURED;
        this->temporallyUnstructuredIndices.at(nodeIndex) =
            vklNewData(device,
                       temporallyUnstructuredNumIndices,
                       VKL_UINT,
                       temporallyUnstructuredIndices,
                       flags,
                       0);
        dataSize =
            temporallyUnstructuredIndices[temporallyUnstructuredNumIndices - 1];
        this->temporallyUnstructuredTimes.at(nodeIndex) = vklNewData(
            device, dataSize, VKL_FLOAT, temporallyUnstructuredTimes, flags, 0);
      } else {
        this->temporalFormat.at(nodeIndex) = VKL_TEMPORAL_FORMAT_CONSTANT;
      }

      if (repackNodes) {
        if (this->temporalFormat[nodeIndex] != VKL_TEMPORAL_FORMAT_CONSTANT) {
          throw std::runtime_error(
              "repackNodes only supported with temporally constant data");
        }

        for (size_t a = 0; a < attributeDataTypes.size(); a++) {
          const size_t elementSize = attributeElementSizes[a];
          const size_t nodeSize =
              elementSize * vklVdbLevelNumVoxels(VKL_VDB_NUM_LEVELS - 1);

          const size_t requiredSize = (denseNodeIndex + 1) * nodeSize;

          if (repackedDenseNodes[a].size() < requiredSize) {
            static bool warnOnce = false;

            if (!warnOnce) {
              std::cerr
                  << "VdbVolumeBuffers: resizing dense node memory; this is "
                     "slow, use reserve() to pre-allocate memory!"
                  << std::endl;
              warnOnce = true;
            }

            repackedDenseNodes[a].resize(requiredSize);
          }

          if (!byteStrides.size() ||
              (byteStrides[a] == 0 || byteStrides[a] == elementSize)) {
            // naturally strided, use a single memcpy()
            std::memcpy(repackedDenseNodes[a].data() + denseNodeIndex * nodeSize,
                        ptrs[a],
                        nodeSize);
          } else {
            // unnaturally strided, set elements individually...
            char *dstBegin =
                repackedDenseNodes[a].data() + denseNodeIndex * nodeSize;

            for (size_t i = 0; i < vklVdbLevelNumVoxels(VKL_VDB_NUM_LEVELS - 1);
                 i++) {
              std::memcpy(dstBegin + i * elementSize,
                          static_cast<char *>(ptrs[a]) + i * byteStrides[a],
                          attributeElementSizes[a]);
            }
          }
        }

      } else {
        // for default (not repacked) data

        if (flags == VKL_DATA_SHARED_BUFFER) {
          isUsingSharedData = true;
        }

        if (data.at(nodeIndex))
          vklRelease(data.at(nodeIndex));

        // only use array-of-arrays when we have multiple attributes
        if (ptrs.size() == 1) {
          data.at(nodeIndex) =
              vklNewData(device,
                         dataSize,
                         attributeDataTypes[0],
                         ptrs[0],
                         flags,
                         byteStrides.size() ? byteStrides[0] : 0);
        } else {
          std::vector<VKLData> attributesData;

          for (size_t i = 0; i < ptrs.size(); i++) {
            attributesData.push_back(
                vklNewData(device,
                           dataSize,
                           attributeDataTypes[i],
                           ptrs[i],
                           flags,
                           byteStrides.size() ? byteStrides[i] : 0));
          }

          data.at(nodeIndex) = vklNewData(device,
                                          attributesData.size(),
                                          VKL_DATA,
                                          attributesData.data(),
                                          VKL_DATA_DEFAULT);

          for (size_t i = 0; i < attributesData.size(); i++) {
            vklRelease(attributesData[i]);
          }
        }
      }
    }

    inline VKLVolume VdbVolumeBuffers::createVolume(bool commit) const
    {
      VKLVolume volume = vklNewVolume(device, "vdb");

      vklSetParam2(volume, "indexToObject", VKL_AFFINE3F, &indexToObject);

      if (!activeVoxelsBoundingBox.empty()) {
        vklSetParam2(
            volume, "indexClippingBounds", VKL_BOX3I, &activeVoxelsBoundingBox);
      }

      // Create the data buffer from our pointers.
      const size_t numNodes = level.size();

      // Note: We do not rely on shared buffers for leaf data because this
      // means the buffer
      //       object can change safely, including replacing leaf data.
      //       This also means that the VdbVolumeBuffers object can be
      //       destroyed after creating the volume.
      assert(level.size() == numNodes);
      VKLData levelData = vklNewData(
          device, numNodes, VKL_UINT, level.data(), VKL_DATA_DEFAULT);
      vklSetData2(volume, "node.level", levelData);
      vklRelease(levelData);

      assert(origin.size() == numNodes);
      VKLData originData = vklNewData(
          device, numNodes, VKL_VEC3I, origin.data(), VKL_DATA_DEFAULT);
      vklSetData2(volume, "node.origin", originData);
      vklRelease(originData);

      assert(format.size() == numNodes);
      VKLData formatData = vklNewData(
          device, numNodes, VKL_UINT, format.data(), VKL_DATA_DEFAULT);
      vklSetData2(volume, "node.format", formatData);
      vklRelease(formatData);

      assert(temporalFormat.size() == numNodes);
      VKLData temporalFormatData = vklNewData(
          device, numNodes, VKL_UINT, temporalFormat.data(), VKL_DATA_DEFAULT);
      vklSetData2(volume, "node.temporalFormat", temporalFormatData);
      vklRelease(temporalFormatData);

      assert(temporallyStructuredNumTimesteps.size() == numNodes);
      VKLData temporallyStructuredNumTimestepsData =
          vklNewData(device,
                     numNodes,
                     VKL_INT,
                     temporallyStructuredNumTimesteps.data(),
                     VKL_DATA_DEFAULT);
      vklSetData2(volume,
                 "node.temporallyStructuredNumTimesteps",
                 temporallyStructuredNumTimestepsData);
      vklRelease(temporallyStructuredNumTimestepsData);

      assert(temporallyUnstructuredIndices.size() == numNodes);
      VKLData temporallyUnstructuredIndicesData =
          vklNewData(device,
                     numNodes,
                     VKL_DATA,
                     temporallyUnstructuredIndices.data(),
                     VKL_DATA_DEFAULT);
      vklSetData2(volume,
                 "node.temporallyUnstructuredIndices",
                 temporallyUnstructuredIndicesData);
      vklRelease(temporallyUnstructuredIndicesData);
      for (auto &buf: temporallyUnstructuredIndices) {
        if (buf) {
          vklRelease(buf);
        }
      }

      assert(temporallyUnstructuredTimes.size() == numNodes);
      VKLData temporallyUnstructuredTimesData =
          vklNewData(device,
                     numNodes,
                     VKL_DATA,
                     temporallyUnstructuredTimes.data(),
                     VKL_DATA_DEFAULT);
      vklSetData2(volume,
                 "node.temporallyUnstructuredTimes",
                 temporallyUnstructuredTimesData);
      vklRelease(temporallyUnstructuredTimesData);
      for (auto &buf: temporallyUnstructuredTimes) {
        if (buf) {
          vklRelease(buf);
        }
      }

      if (repackNodes) {
        // dense nodes
        if (numDenseNodes > 0) {
          std::vector<VKLData> repackedDenseNodesData;

          const size_t nodeNumElements =
              vklVdbLevelNumVoxels(VKL_VDB_NUM_LEVELS - 1);

          for (size_t a = 0; a < attributeDataTypes.size(); a++) {
            const size_t expectedByteSize =
                numDenseNodes * nodeNumElements * attributeElementSizes[a];

            if (expectedByteSize > repackedDenseNodes[a].size()) {
              throw std::runtime_error("repackedDenseNodes has incorrect size");
            }

            VKLData d = vklNewData(device,
                                   numDenseNodes * nodeNumElements,
                                   attributeDataTypes[a],
                                   repackedDenseNodes[a].data(),
                                   VKL_DATA_DEFAULT);
            repackedDenseNodesData.push_back(d);
          }

          VKLData dataData = vklNewData(device,
                                        repackedDenseNodesData.size(),
                                        VKL_DATA,
                                        repackedDenseNodesData.data(),
                                        VKL_DATA_DEFAULT);
          vklSetData2(volume, "nodesPackedDense", dataData);
          vklRelease(dataData);

          for (const auto &d : repackedDenseNodesData) {
            vklRelease(d);
          }
        }

        // tiles
        if (numTileNodes > 0) {
          std::vector<VKLData> repackedTilesData;

          for (size_t a = 0; a < attributeDataTypes.size(); a++) {
            const size_t expectedByteSize =
                numTileNodes * attributeElementSizes[a];

            if (expectedByteSize > repackedTiles[a].size()) {
              throw std::runtime_error("repackedTiles has incorrect size");
            }

            VKLData d = vklNewData(device,
                                   numTileNodes,
                                   attributeDataTypes[a],
                                   repackedTiles[a].data(),
                                   VKL_DATA_DEFAULT);
            repackedTilesData.push_back(d);
          }

          VKLData dataData = vklNewData(device,
                                        repackedTilesData.size(),
                                        VKL_DATA,
                                        repackedTilesData.data(),
                                        VKL_DATA_DEFAULT);
          vklSetData2(volume, "nodesPackedTile", dataData);
          vklRelease(dataData);

          for (const auto &d : repackedTilesData) {
            vklRelease(d);
          }
        }

      } else {
        assert(data.size() == numNodes);
        VKLData dataData = vklNewData(
            device, numNodes, VKL_DATA, data.data(), VKL_DATA_DEFAULT);
        vklSetData2(volume, "node.data", dataData);
        vklRelease(dataData);
      }

      if (commit) {
        vklCommit2(volume);
      }

      return volume;
    }

    inline bool VdbVolumeBuffers::usingSharedData() const
    {
      return isUsingSharedData;
    }

    inline VKLDevice VdbVolumeBuffers::getVKLDevice() const
    {
      return device;
    }

    }  // namespace vdb
  }  // namespace utility
}  // namespace openvkl
