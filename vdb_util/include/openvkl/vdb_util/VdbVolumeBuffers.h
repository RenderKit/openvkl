// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"
#include "ospcommon/math/AffineSpace.h"
#include "ospcommon/math/vec.h"

namespace openvkl {
  namespace vdb_util {

    using vec3i         = ospcommon::math::vec3i;
    using vec3f         = ospcommon::math::vec3f;
    using AffineSpace3f = ospcommon::math::AffineSpace3f;
    using LinearSpace3f = ospcommon::math::LinearSpace3f;

    /*
     * These are all the buffers we need. We will create VKLData objects
     * from these buffers, and then set those as parameters on the
     * VKLVolume object.
     */
    template <VKLDataType FieldType>
    struct VdbVolumeBuffers
    {
      static_assert(FieldType == VKL_FLOAT,
                    "vdb volumes only support VKL_FLOAT fields at the moment.");

     private:
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
       * The node format. This can be VKL_VDB_FORMAT_TILE or
       * VKL_VDB_FORMAT_CONSTANT at this point.
       */
      std::vector<VKLVdbLeafFormat> format;

      /*
       * The actual node data. Tiles have exactly one value,
       * constant nodes have vklVdbLevelRes(level)^3 =
       * vklVdbLevelNumVoxels(level) values.
       */
      std::vector<VKLData> data;

     public:
      /*
       * Construction / destruction.
       */
      VdbVolumeBuffers() = default;
      ~VdbVolumeBuffers()
      {
        clear();
      }

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

      size_t numNodes() const
      {
        return level.size();
      }

      /*
       * Clear all buffers.
       */
      void clear()
      {
        for (VKLData d : data)
          vklRelease(d);
        level.clear();
        origin.clear();
        format.clear();
        data.clear();
      }

      /*
       * Preallocate memory for numNodes nodes.
       * This helps reduce load times because only one allocation needs to be
       * made.
       */
      void reserve(size_t numNodes)
      {
        assert(level.empty());
        assert(origin.empty());
        assert(format.empty());
        assert(data.empty());

        level.reserve(numNodes);
        origin.reserve(numNodes);
        format.reserve(numNodes);
        data.reserve(numNodes);
      }

      /*
       * Add a new tile node.
       * Returns the new node's index.
       */
      size_t addTile(uint32_t level, const vec3i &origin, const void *ptr)
      {
        const size_t index = numNodes();
        this->level.push_back(level);
        this->origin.push_back(origin);
        format.push_back(VKL_VDB_FORMAT_TILE);
        data.push_back(vklNewData(1, FieldType, ptr, VKL_DATA_DEFAULT));
        return index;
      }

      /*
       * Add a new constant node.
       * Returns the new node's index.
       */
      size_t addConstant(uint32_t level,
                         const vec3i &origin,
                         const void *ptr,
                         VKLDataCreationFlags flags)
      {
        const size_t index = numNodes();
        this->level.push_back(level);
        this->origin.push_back(origin);
        format.push_back(VKL_VDB_FORMAT_INVALID);
        data.push_back(nullptr);
        makeConstant(index, ptr, flags);
        return index;
      }

      /*
       * Change the given node to a constant node.
       * This is useful for deferred loading.
       */
      void makeConstant(size_t index,
                        const void *ptr,
                        VKLDataCreationFlags flags)
      {
        if (data.at(index))
          vklRelease(data.at(index));
        data.at(index) = vklNewData(
            vklVdbLevelNumVoxels(level.at(index)), FieldType, ptr, flags);
        format.at(index) = VKL_VDB_FORMAT_CONSTANT;
      }

      /*
       * Create a VKLVolume from these buffers.
       */
      VKLVolume createVolume(VKLFilter filter) const
      {
        VKLVolume volume = vklNewVolume("vdb");
        vklSetInt(volume, "type", FieldType);
        vklSetInt(volume, "filter", filter);
        vklSetInt(volume, "maxSamplingDepth", vklVdbNumLevels() - 1);
        vklSetInt(volume, "maxIteratorDepth", 3);
        vklSetData(volume,
                   "indexToObject",
                   vklNewData(12, VKL_FLOAT, indexToObject, VKL_DATA_DEFAULT));

        // Create the data buffer from our pointers.
        const size_t numNodes = level.size();

        // Note: We do not rely on shared buffers for leaf data because this
        // means the buffer
        //       object can change safely, including replacing leaf data.
        //       This also means that the VdbVolumeBuffers object can be
        //       destroyed after creating the volume.
        vklSetData(
            volume,
            "level",
            vklNewData(numNodes, VKL_UINT, level.data(), VKL_DATA_DEFAULT));
        vklSetData(
            volume,
            "origin",
            vklNewData(numNodes, VKL_VEC3I, origin.data(), VKL_DATA_DEFAULT));
        vklSetData(
            volume,
            "format",
            vklNewData(numNodes, VKL_UINT, format.data(), VKL_DATA_DEFAULT));
        vklSetData(
            volume,
            "data",
            vklNewData(numNodes, VKL_DATA, data.data(), VKL_DATA_DEFAULT));

        vklCommit(volume);
        return volume;
      }
    };

  }  // namespace vdb_util
}  // namespace openvkl
