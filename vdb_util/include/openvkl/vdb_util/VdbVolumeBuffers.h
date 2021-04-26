// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"
#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace vdb_util {

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
       */
      VdbVolumeBuffers(VKLDevice device,
                       const std::vector<VKLDataType> &attributeDataTypes);
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

      size_t numNodes() const;

      /*
       * Clear all buffers.
       */
      void clear();

      /*
       * Preallocate memory for numNodes nodes.
       * This helps reduce load times because only one allocation needs to be
       * made.
       */
      void reserve(size_t numNodes);

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
       * This is useful for deferred loading.
       */
      void makeConstant(size_t index,
                        const std::vector<void *> &ptrs,
                        VKLDataCreationFlags flags,
                        const std::vector<size_t> &byteStrides        = {},
                        uint32_t temporallyStructuredNumTimesteps     = 0,
                        uint32_t temporallyUnstructuredNumIndices     = 0,
                        const uint32_t *temporallyUnstructuredIndices = nullptr,
                        const float *temporallyUnstructuredTimes = nullptr);

      /*
       * Create a VKLVolume from these buffers.
       */
      VKLVolume createVolume(VKLFilter filter) const;

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
       * The grid transform (index space to object space).
       */
      float indexToObject[12] = {
          1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f};

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
       * VKL_FORMAT_CONSTANT_ZYX at this point.
       */
      std::vector<VKLFormat> format;

      /*
       * Temporal config. All of these buffers are optional, but we allocate
       * of them for simplicity.
       */
      std::vector<VKLTemporalFormat> temporalFormat;
      std::vector<uint32_t> temporallyStructuredNumTimesteps;
      std::vector<VKLData> temporallyUnstructuredIndices;
      std::vector<VKLData> temporallyUnstructuredTimes;

      /*
       * The actual node data. Tiles have exactly one value,
       * constant nodes have vklVdbLevelRes(level)^3 =
       * vklVdbLevelNumVoxels(level) values.
       */
      std::vector<VKLData> data;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline VdbVolumeBuffers::VdbVolumeBuffers(
        VKLDevice device, const std::vector<VKLDataType> &attributeDataTypes)
        : device(device), attributeDataTypes(attributeDataTypes)
    {
      for (const auto &dt : attributeDataTypes) {
        if (dt != VKL_HALF && dt != VKL_FLOAT) {
          throw std::runtime_error(
              "vdb volumes only support VKL_HALF and VKL_FLOAT attributes");
        }
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
      indexToObject[0]  = l00;
      indexToObject[1]  = l01;
      indexToObject[2]  = l02;
      indexToObject[3]  = l10;
      indexToObject[4]  = l11;
      indexToObject[5]  = l12;
      indexToObject[6]  = l20;
      indexToObject[7]  = l21;
      indexToObject[8]  = l22;
      indexToObject[9]  = p0;
      indexToObject[10] = p1;
      indexToObject[11] = p2;
    }

    inline size_t VdbVolumeBuffers::numNodes() const
    {
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
    }

    inline void VdbVolumeBuffers::reserve(size_t numNodes)
    {
      assert(level.empty());
      assert(origin.empty());
      assert(format.empty());
      assert(temporalFormat.empty());
      assert(temporallyStructuredNumTimesteps.empty());
      assert(temporallyUnstructuredIndices.empty());
      assert(temporallyUnstructuredTimes.empty());
      assert(data.empty());

      level.reserve(numNodes);
      origin.reserve(numNodes);
      format.reserve(numNodes);
      temporalFormat.reserve(numNodes);
      temporallyStructuredNumTimesteps.reserve(numNodes);
      temporallyUnstructuredIndices.reserve(numNodes);
      temporallyUnstructuredTimes.reserve(numNodes);
      data.reserve(numNodes);
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

      const size_t index = numNodes();
      this->level.push_back(level);
      this->origin.push_back(origin);
      format.push_back(VKL_FORMAT_TILE);

      uint32_t dataSize = 1;
      if (temporallyStructuredNumTimesteps > 1) {
        this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_STRUCTURED);
        this->temporallyStructuredNumTimesteps.push_back(
            temporallyStructuredNumTimesteps);
        this->temporallyUnstructuredIndices.push_back(nullptr);
        this->temporallyUnstructuredTimes.push_back(nullptr);
        dataSize = temporallyStructuredNumTimesteps;
      } else if (temporallyUnstructuredIndices && temporallyUnstructuredTimes) {
        this->temporallyStructuredNumTimesteps.push_back(0);
        this->temporallyUnstructuredIndices.push_back(nullptr);
        this->temporallyUnstructuredTimes.push_back(nullptr);
        throw std::runtime_error("NOT IMPLEMENTED");
        assert(false);
      } else {
        this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_CONSTANT);
      }

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

      const size_t index = numNodes();
      this->level.push_back(level);
      this->origin.push_back(origin);
      format.push_back(VKL_FORMAT_INVALID);
      this->temporalFormat.push_back(VKL_TEMPORAL_FORMAT_INVALID);
      this->temporallyStructuredNumTimesteps.push_back(0);
      this->temporallyUnstructuredIndices.push_back(nullptr);
      this->temporallyUnstructuredTimes.push_back(nullptr);
      data.push_back(nullptr);
      makeConstant(index,
                   ptrs,
                   flags,
                   byteStrides,
                   temporallyStructuredNumTimesteps,
                   temporallyUnstructuredNumIndices,
                   temporallyUnstructuredIndices,
                   temporallyUnstructuredTimes);
      return index;
    }

    inline void VdbVolumeBuffers::makeConstant(
        size_t index,
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

      format.at(index) = VKL_FORMAT_CONSTANT_ZYX;

      uint32_t dataSize = vklVdbLevelNumVoxels(level.at(index));
      if (temporallyStructuredNumTimesteps > 1) {
        this->temporalFormat.at(index) = VKL_TEMPORAL_FORMAT_STRUCTURED;
        this->temporallyStructuredNumTimesteps.at(index) =
            temporallyStructuredNumTimesteps;
        dataSize *= temporallyStructuredNumTimesteps;
      } else if ((temporallyUnstructuredNumIndices > 0) &&
                 temporallyUnstructuredIndices && temporallyUnstructuredTimes) {
        this->temporalFormat.at(index) = VKL_TEMPORAL_FORMAT_UNSTRUCTURED;
        this->temporallyUnstructuredIndices.at(index) =
            vklNewData(device,
                       temporallyUnstructuredNumIndices,
                       VKL_UINT,
                       temporallyUnstructuredIndices,
                       flags,
                       0);
        dataSize =
            temporallyUnstructuredIndices[temporallyUnstructuredNumIndices - 1];
        this->temporallyUnstructuredTimes.at(index) = vklNewData(
            device, dataSize, VKL_FLOAT, temporallyUnstructuredTimes, flags, 0);
      } else {
        this->temporalFormat.at(index) = VKL_TEMPORAL_FORMAT_CONSTANT;
      }

      if (data.at(index))
        vklRelease(data.at(index));

      // only use array-of-arrays when we have multiple attributes
      if (ptrs.size() == 1) {
        data.at(index) = vklNewData(device,
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

        data.at(index) = vklNewData(device,
                                    attributesData.size(),
                                    VKL_DATA,
                                    attributesData.data(),
                                    VKL_DATA_DEFAULT);

        for (size_t i = 0; i < attributesData.size(); i++) {
          vklRelease(attributesData[i]);
        }
      }
    }

    inline VKLVolume VdbVolumeBuffers::createVolume(VKLFilter filter) const
    {
      VKLVolume volume = vklNewVolume(device, "vdb");
      vklSetInt(volume, "filter", filter);

      VKLData transformData =
          vklNewData(device, 12, VKL_FLOAT, indexToObject, VKL_DATA_DEFAULT);
      vklSetData(volume, "indexToObject", transformData);
      vklRelease(transformData);

      // Create the data buffer from our pointers.
      const size_t numNodes = level.size();

      // Note: We do not rely on shared buffers for leaf data because this
      // means the buffer
      //       object can change safely, including replacing leaf data.
      //       This also means that the VdbVolumeBuffers object can be
      //       destroyed after creating the volume.
      VKLData levelData = vklNewData(
          device, numNodes, VKL_UINT, level.data(), VKL_DATA_DEFAULT);
      vklSetData(volume, "node.level", levelData);
      vklRelease(levelData);

      VKLData originData = vklNewData(
          device, numNodes, VKL_VEC3I, origin.data(), VKL_DATA_DEFAULT);
      vklSetData(volume, "node.origin", originData);
      vklRelease(originData);

      VKLData formatData = vklNewData(
          device, numNodes, VKL_UINT, format.data(), VKL_DATA_DEFAULT);
      vklSetData(volume, "node.format", formatData);
      vklRelease(formatData);

      VKLData temporalFormatData = vklNewData(
          device, numNodes, VKL_UINT, temporalFormat.data(), VKL_DATA_DEFAULT);
      vklSetData(volume, "node.temporalFormat", temporalFormatData);
      vklRelease(temporalFormatData);

      VKLData temporallyStructuredNumTimestepsData =
          vklNewData(device,
                     numNodes,
                     VKL_INT,
                     temporallyStructuredNumTimesteps.data(),
                     VKL_DATA_DEFAULT);
      vklSetData(volume,
                 "node.temporallyStructuredNumTimesteps",
                 temporallyStructuredNumTimestepsData);
      vklRelease(temporallyStructuredNumTimestepsData);

      VKLData temporallyUnstructuredIndicesData =
          vklNewData(device,
                     numNodes,
                     VKL_DATA,
                     temporallyUnstructuredIndices.data(),
                     VKL_DATA_DEFAULT);
      vklSetData(volume,
                 "node.temporallyUnstructuredIndices",
                 temporallyUnstructuredIndicesData);
      vklRelease(temporallyUnstructuredIndicesData);

      VKLData temporallyUnstructuredTimesData =
          vklNewData(device,
                     numNodes,
                     VKL_DATA,
                     temporallyUnstructuredTimes.data(),
                     VKL_DATA_DEFAULT);
      vklSetData(volume,
                 "node.temporallyUnstructuredTimes",
                 temporallyUnstructuredTimesData);
      vklRelease(temporallyUnstructuredTimesData);

      VKLData dataData = vklNewData(
          device, data.size(), VKL_DATA, data.data(), VKL_DATA_DEFAULT);
      vklSetData(volume, "node.data", dataData);
      vklRelease(dataData);

      vklCommit(volume);
      return volume;
    }

    inline VKLDevice VdbVolumeBuffers::getVKLDevice() const
    {
      return device;
    }

  }  // namespace vdb_util
}  // namespace openvkl
