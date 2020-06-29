// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbVolume.h"
#include <cstring>
#include <set>
#include "../../common/export_util.h"
#include "../common/logging.h"
#include "VdbLeafAccessObserver.h"
#include "VdbSampler.h"
#include "VdbSampler_ispc.h"
#include "openvkl/vdb.h"
#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/memory/malloc.h"
#include "rkcommon/tasking/AsyncTask.h"

namespace openvkl {
  namespace ispc_driver {

    /*
     * Centralized allocation helpers.
     */
    template <class T>
    T *allocate(size_t size, size_t &bytesAllocated)
    {
      const size_t numBytes = size * sizeof(T);
      bytesAllocated += numBytes;
      T *buf = reinterpret_cast<T *>(rkcommon::memory::alignedMalloc(numBytes));
      if (!buf)
        throw std::bad_alloc();
      std::memset(buf, 0, numBytes);
      return buf;
    }

    template <class T>
    void deallocate(T *&ptr)
    {
      rkcommon::memory::alignedFree(ptr);
      ptr = nullptr;
    }

    // -------------------------------------------------------------------------

    template <int W>
    VdbVolume<W>::VdbVolume()
    {
      this->ispcEquivalent = CALL_ISPC(VdbVolume_create);
    }

    template <int W>
    VdbVolume<W>::VdbVolume(VdbVolume<W> &&other)
    {
      using std::swap;
      swap(bounds, other.bounds);
      swap(name, other.name);
      swap(valueRange, other.valueRange);
      swap(leafData, other.leafData);
      swap(leafDataISPC, other.leafDataISPC);
      swap(grid, other.grid);
      swap(bytesAllocated, other.bytesAllocated);
    }

    template <int W>
    VdbVolume<W> &VdbVolume<W>::operator=(VdbVolume<W> &&other)
    {
      if (this != &other) {
        using std::swap;
        swap(bounds, other.bounds);
        swap(name, other.name);
        swap(valueRange, other.valueRange);
        swap(leafData, other.leafData);
        swap(leafDataISPC, other.leafDataISPC);
        swap(grid, other.grid);
        swap(bytesAllocated, other.bytesAllocated);
      }
      return *this;
    }

    template <int W>
    VdbVolume<W>::~VdbVolume()
    {
      cleanup();
      CALL_ISPC(VdbVolume_destroy, this->ispcEquivalent);
    }

    template <int W>
    void VdbVolume<W>::cleanup()
    {
      if (grid) {
        // Note: There are VKL_VDB_NUM_LEVELS-1 slots for the
        //       level buffers! Leaves are not stored in the hierarchy!
        for (uint32_t l = 0; (l + 1) < vklVdbNumLevels(); ++l) {
          VdbLevel &level = grid->levels[l];
          deallocate(level.voxels);
          deallocate(level.valueRange);
          deallocate(level.leafIndex);
        }
        deallocate(grid->usageBuffer);
        deallocate(grid);
      }
      bytesAllocated = 0;
    }

    template <int W>
    std::string VdbVolume<W>::toString() const
    {
      return "openvkl::VdbVolume";
    }

    template <class... Args>
    void runtimeError(Args &&... args)
    {
      std::ostringstream os;
      int dummy[sizeof...(Args)] = {(os << std::forward<Args>(args), 0)...};
      throw std::runtime_error(os.str());
    }

    /*
     * Compute the grid bounding box and count the number of leaves per level.
     */
    box3i computeBbox(uint64_t numLeaves,
                      const DataT<uint32_t> &leafLevel,
                      const DataT<vec3i> &leafOrigin)
    {
      box3i bbox = box3i();
      for (size_t i = 0; i < numLeaves; ++i) {
        bbox.extend(leafOrigin[i]);
        bbox.extend(leafOrigin[i] + vec3ui(vklVdbLevelRes(leafLevel[i])));
      }
      return bbox;
    }

    /*
     * Bin leaves per level (returns indices into the input leaf array).
     */
    std::vector<std::vector<uint64_t>> binLeavesPerLevel(
        uint64_t numLeaves, const DataT<uint32_t> &leafLevel)
    {
      std::vector<size_t> numLeavesPerLevel(vklVdbNumLevels(), 0);
      for (uint64_t i = 0; i < numLeaves; ++i) {
        if (leafLevel[i] == 0)
          runtimeError("there must not be any leaf nodes on level 0");
        numLeavesPerLevel[leafLevel[i]]++;
      }

      // Sort leaves by level.
      std::vector<std::vector<uint64_t>> binnedLeaves(vklVdbNumLevels());
      ;
      for (uint64_t l = 1; l < vklVdbNumLevels();
           ++l)  // level 0 has no leaves!
        binnedLeaves[l].reserve(numLeavesPerLevel[l]);
      for (uint64_t i = 0; i < numLeaves; ++i)
        binnedLeaves[leafLevel[i]].push_back(i);
      return binnedLeaves;
    }

    /*
     * Compute the root node origin from the bounding box.
     */
    vec3i computeRootOrigin(const box3i &bbox)
    {
      const vec3ui bboxRes = bbox.upper - bbox.lower;
      if (bboxRes.x > vklVdbLevelRes(0) || bboxRes.y > vklVdbLevelRes(0) ||
          bboxRes.z > vklVdbLevelRes(0)) {
        runtimeError("input leaves do not fit into a single root level node");
      }
      return vec3i(
          vklVdbLevelRes(1) *
              (int)std::floor(bbox.lower.x / (float)vklVdbLevelRes(1)),
          vklVdbLevelRes(1) *
              (int)std::floor(bbox.lower.y / (float)vklVdbLevelRes(1)),
          vklVdbLevelRes(1) *
              (int)std::floor(bbox.lower.z / (float)vklVdbLevelRes(1)));
    }

    /*
     * We don't want to deal with the complexity of negative
     * indices in our tree, so only consider offsets relative to the root node
     * origin.
     * This function computes these offsets.
     */
    inline std::vector<vec3ui> computeLeafOffsets(
        uint64_t numLeaves,
        const DataT<vec3i> &leafOrigin,
        const vec3ui &rootOrigin)
    {
      std::vector<vec3ui> leafOffsets(numLeaves);
      for (size_t i = 0; i < numLeaves; ++i)
        leafOffsets[i] = static_cast<vec3ui>(leafOrigin[i] - rootOrigin);
      return leafOffsets;
    }

    inline vec3ui offsetToNodeOrigin(
        const vec3ui &offset,  // offset from the root origin.
        uint32_t level)        // the level the node is on.
    {
      // We get the inner node origin from a given (leaf) voxel offset
      // by masking out lower bits.
      const uint32_t mask = ~(vklVdbLevelRes(level) - 1);
      return vec3ui(offset.x & mask, offset.y & mask, offset.z & mask);
    }

    inline vec3ui offsetToVoxelIndex(const vec3ui &offset, uint32_t level)
    {
      // The lower bits contain the offset from the node origin. We then
      // shift by the log child resolution to obtain the voxel index.
      const uint32_t mask = vklVdbLevelRes(level) - 1;
      return vec3ui((offset.x & mask) >> vklVdbLevelTotalLogRes(level + 1),
                    (offset.y & mask) >> vklVdbLevelTotalLogRes(level + 1),
                    (offset.z & mask) >> vklVdbLevelTotalLogRes(level + 1));
    }

    inline uint64_t offsetToLinearVoxelIndex(const vec3ui &offset,
                                             uint32_t level)
    {
      // The lower bits contain the offset from the node origin. We then
      // shift by the log child resolution to obtain the voxel index.
      const vec3ui vi = offsetToVoxelIndex(offset, level);
      return (((uint64_t)vi.x) << (2 * vklVdbLevelResShift(level))) +
             (((uint64_t)vi.y) << vklVdbLevelResShift(level)) +
             ((uint64_t)vi.z);
    }

    /*
     * Initialize all (inner) levels. To do this, we must
     * count the number of inner nodes per level, and allocate buffers for
     * voxels and auxiliary data.
     */
    void allocateInnerLevels(
        const std::vector<vec3ui> &leafOffsets,
        const std::vector<std::vector<uint64_t>> &binnedLeaves,
        std::vector<uint64_t> &capacity,
        VdbGrid *grid,
        size_t &bytesAllocated)
    {
      // Comparator for voxel coordinates. We use this for sorting.
      const auto isLess = [](const vec3i &a, const vec3i &b) {
        return (a.x < b.x) || ((a.x == b.x) && (a.y < b.y)) ||
               ((a.x == b.x) && (a.y == b.y) && (a.z < b.z));
      };

      // Origins on the previous level. These are offsets from grid->rootOrigin.
      std::vector<vec3ui> oldInnerOrigins;

      // From the leaf level, go upwards quantizing leaf origins
      // to the respective level storage resolution, and count all
      // active nodes.
      for (int i = 0; i < vklVdbNumLevels() - 1; ++i) {
        // We traverse bottom-to-top, starting at the leaf level (we will update
        // the parent level!).
        const int l = vklVdbNumLevels() - i - 1;

        // Quantize all of this level's leaf origins to the node size, mapping
        // offsets to inner node origins. We can do this using simple masking
        // because node resolutions are powers of two.
        std::vector<vec3ui> innerOrigins;
        innerOrigins.reserve(oldInnerOrigins.size() + binnedLeaves[l].size());
        for (uint64_t leaf : binnedLeaves[l])
          innerOrigins.push_back(offsetToNodeOrigin(leafOffsets[leaf], l - 1));

        // Also quanitize the child level's inner node origins.
        for (const vec3ui &org : oldInnerOrigins)
          innerOrigins.push_back(offsetToNodeOrigin(org, l - 1));

        // We now have a list of inner node origins on level l-1, but it
        // contains duplicates. Sort and remove duplicates, and store for next
        // iterations.
        std::sort(innerOrigins.begin(), innerOrigins.end(), isLess);
        const auto end = std::unique(innerOrigins.begin(), innerOrigins.end());
        const uint64_t levelNumInner = end - innerOrigins.begin();
        innerOrigins.resize(levelNumInner);
        oldInnerOrigins = std::move(innerOrigins);

        if (levelNumInner > 0) {
          assert(l > 1 ||
                 levelNumInner ==
                     1);  // This should be true at this point, but make sure...
          VdbLevel &level = grid->levels[l - 1];
          capacity[l - 1] = levelNumInner;
          const size_t totalNumVoxels =
              levelNumInner * vklVdbLevelNumVoxels(l - 1);
          level.voxels     = allocate<uint64_t>(totalNumVoxels, bytesAllocated);
          level.valueRange = allocate<range1f>(totalNumVoxels, bytesAllocated);
          level.leafIndex  = allocate<uint64>(totalNumVoxels, bytesAllocated);
          range1f empty;
          std::fill(level.valueRange, level.valueRange + totalNumVoxels, empty);
        }
      }
    }

    /*
     * Compute the value range for float leaves.
     */
    range1f computeValueRangeFloat(const VdbGrid *grid,
                                   VKLFormat format,
                                   uint32_t level,
                                   const vec3ui &offset,
                                   const DataT<float> &data)
    {
      range1f range;

      switch (format) {
      case VKL_FORMAT_TILE: {
        CALL_ISPC(VdbSampler_valueRangeTileFloat,
                  grid,
                  ispc(data),
                  reinterpret_cast<const ispc::vec3ui *>(&offset),
                  level,
                  reinterpret_cast<ispc::box1f *>(&range));
        break;
      }

      case VKL_FORMAT_CONSTANT_ZYX: {
        range1f leafRange;
        CALL_ISPC(VdbSampler_valueRangeConstantFloat,
                  grid,
                  ispc(data),
                  reinterpret_cast<const ispc::vec3ui *>(&offset),
                  level,
                  reinterpret_cast<ispc::box1f *>(&range));
        break;
      }

      default:
        runtimeError(
            "Only VKL_FORMAT_TILE and VKL_FORMAT_CONST are supported.");
      }

      return range;
    }

    /*
     * Insert leaf nodes into the tree, creating inner nodes as needed.
     * This function does not allocate anything; allocateInnerLevels() has done
     * this already.
     */
    void insertLeavesFloat(
        const std::vector<vec3ui> &leafOffsets,
        const DataT<uint32_t> &leafFormat,
        const DataT<Data *> &leafData,
        const std::vector<AlignedISPCData1D> &leafDataISPC,
        const std::vector<std::vector<uint64_t>> &binnedLeaves,
        const std::vector<uint64_t> &capacity,
        VdbGrid *grid)
    {
      assert(capacity[0] == 1);
      grid->levels[0].numNodes = 1;

      for (size_t leafLevel = 0; leafLevel < binnedLeaves.size(); ++leafLevel) {
        const auto &leaves = binnedLeaves[leafLevel];
        for (uint64_t idx : leaves) {
          const auto format = static_cast<VKLFormat>(leafFormat[idx]);

          const vec3ui &offset = leafOffsets[idx];
          uint64_t nodeIndex   = 0;
          for (size_t l = 0; l < leafLevel; ++l) {
            VdbLevel &level = grid->levels[l];
            // PRECOND: nodeIndex is valid.
            assert(nodeIndex < level.numNodes);

            const uint64_t voxelIndex = offsetToLinearVoxelIndex(offset, l);
            // NOTE: If this is every greater than 2^32-1 then we will have to
            // use 64 bit addressing.
            const uint64_t v = nodeIndex * vklVdbLevelNumVoxels(l) + voxelIndex;
            assert(v < ((uint64_t)1) << 32);

            uint64_t &voxel = level.voxels[v];
            if (vklVdbVoxelIsLeafPtr(voxel) || vklVdbVoxelIsTile(voxel)) {
              assert(false);
              runtimeError(
                  "Attempted to insert a leaf node into a leaf node (level ",
                  l + 1,
                  ", origin ",
                  offsetToNodeOrigin(offset, l),
                  ")");

            } else if (vklVdbVoxelIsEmpty(voxel)) {
              const size_t nl = l + 1;
              if (nl < leafLevel) {
                nodeIndex = grid->levels[nl].numNodes++;
                assert(grid->levels[nl].numNodes <= capacity[nl]);
                voxel = vklVdbVoxelMakeChildPtr(nodeIndex);
              } else {
                if (format == VKL_FORMAT_TILE) {
                  voxel = vklVdbVoxelMakeTile(leafData[idx]->as<float>()[0]);
                } else if (format == VKL_FORMAT_CONSTANT_ZYX) {
                  if (grid->allLeavesCompact) {
                    voxel = vklVdbVoxelMakeLeafPtr(
                        leafData[idx]->as<float>().data(), format);
                  } else {
                    voxel =
                        vklVdbVoxelMakeLeafPtr(&leafDataISPC[idx].data, format);
                  }
                } else
                  assert(false);

                level.leafIndex[v] = idx;
              }
            } else {
              nodeIndex = vklVdbVoxelChildGetIndex(voxel);
              assert(nodeIndex < grid->levels[l + 1].numNodes);
            }
          }
        }
      }
    }

    /*
     * Compute the value range for the given nodes.
     * The tree must be fully initialized before calling this!
     * This function takes into account filter radius.
     */
    void computeValueRangesFloat(
        const std::vector<vec3ui> &leafOffsets,
        const DataT<uint32_t> &leafLevel,
        const DataT<uint32_t> &leafFormat,
        const DataT<Data *> &leafData,
        VdbGrid *grid)
    {
      // The value range computation is a big part of commit() cost. We
      // do it in parallel to make up for that as much as possible.
      std::vector<range1f> valueRanges(leafFormat.size());

      const size_t numLeaves = leafOffsets.size();
      tasking::parallel_for(numLeaves, [&](size_t idx) {
        const auto format    = static_cast<VKLFormat>(leafFormat[idx]);
        const vec3ui &offset = leafOffsets[idx];
        valueRanges.at(idx)  = computeValueRangeFloat(
            grid, format, leafLevel[idx], offset, leafData[idx]->as<float>());
      });

      for (size_t idx = 0; idx < numLeaves; ++idx) {
        const vec3ui &offset      = leafOffsets[idx];
        const range1f &valueRange = valueRanges[idx];

        uint64_t nodeIndex = 0;
        for (size_t l = 0; l < leafLevel[idx]; ++l) {
          VdbLevel &level = grid->levels[l];
          // PRECOND: nodeIndex is valid.
          assert(nodeIndex < level.numNodes);

          const uint64_t voxelIndex = offsetToLinearVoxelIndex(offset, l);
          // NOTE: If this is every greater than 2^32-1 then we will have to
          // use 64 bit addressing.
          const uint64_t v = nodeIndex * vklVdbLevelNumVoxels(l) + voxelIndex;
          assert(v < ((uint64_t)1) << 32);

          level.valueRange[v].extend(valueRange);

          uint64_t &voxel = level.voxels[v];
          assert(!vklVdbVoxelIsEmpty(voxel));

          if (vklVdbVoxelIsLeafPtr(voxel) || vklVdbVoxelIsTile(voxel))
            break;

          nodeIndex = vklVdbVoxelChildGetIndex(voxel);
          assert(nodeIndex < grid->levels[l + 1].numNodes);
        }
      }
    }

    AffineSpace3f loadTransform(
        const Ref<const DataT<float>> &dataIndexToObject)
    {
      AffineSpace3f a(one);
      if (dataIndexToObject && dataIndexToObject->size() >= 12) {
        const DataT<float> &i2w = *dataIndexToObject;
        a.l                     = LinearSpace3f(vec3f(i2w[0], i2w[1], i2w[2]),
                            vec3f(i2w[3], i2w[4], i2w[5]),
                            vec3f(i2w[6], i2w[7], i2w[8]));
        a.p                     = vec3f(i2w[9], i2w[10], i2w[11]);
      }
      return a;
    }

    void writeTransform(const AffineSpace3f &a, float *buffer)
    {
      assert(buffer);
      buffer[0]  = a.l.row0().x;
      buffer[1]  = a.l.row0().y;
      buffer[2]  = a.l.row0().z;
      buffer[3]  = a.l.row1().x;
      buffer[4]  = a.l.row1().y;
      buffer[5]  = a.l.row1().z;
      buffer[6]  = a.l.row2().x;
      buffer[7]  = a.l.row2().y;
      buffer[8]  = a.l.row2().z;
      buffer[9]  = a.p.x;
      buffer[10] = a.p.y;
      buffer[11] = a.p.z;
    }

    template <int W>
    void VdbVolume<W>::commit()
    {
      cleanup();

      const int maxIteratorDepth =
          this->template getParam<int>("maxIteratorDepth", 3);

      Ref<const DataT<float>> dataIndexToObject =
          this->template getParamDataT<float>("indexToObject", nullptr);
      Ref<const DataT<uint32_t>> leafLevel =
          this->template getParamDataT<uint32_t>("node.level");
      Ref<const DataT<vec3i>> leafOrigin =
          this->template getParamDataT<vec3i>("node.origin");

      // 32 bit unsigned int values. The enum VKLFormat encodes supported
      // values for the format.
      Ref<const DataT<uint32_t>> leafFormat =
          this->template getParamDataT<uint32_t>("node.format");
      // 64 bit unsigned int values. Interpretation depends on leafFormat.
      leafData = this->template getParamDataT<Data *>("node.data");

      // Set up the global sample config.
      globalConfig.filter = (VKLFilter)this->template getParam<int>(
          "filter", VKL_FILTER_TRILINEAR);
      globalConfig.gradientFilter = (VKLFilter)this->template getParam<int>(
          "gradientFilter", globalConfig.filter);
      globalConfig.maxSamplingDepth = this->template getParam<int>(
          "maxSamplingDepth", VKL_VDB_NUM_LEVELS - 1);
      globalConfig.maxSamplingDepth =
          min(globalConfig.maxSamplingDepth, VKL_VDB_NUM_LEVELS - 1u);

      // Sanity checks.
      // We will assume that the following conditions hold downstream, so
      // better test them now.

      // Currently only VKL_FLOAT data is supported.
      std::set<VKLDataType> leafDataTypes;

      for (const auto &d : *leafData)
        leafDataTypes.insert(d->dataType);

      if (leafDataTypes.size() != 1)
        throw std::runtime_error(
            "all node.data arrays must have the same VKLDataType");

      const VKLDataType type = *leafDataTypes.begin();

      if (type != VKL_FLOAT)
        runtimeError("node.data arrays have data type ",
                     type,
                     " but only ",
                     VKL_FLOAT,
                     " (VKL_FLOAT) is supported.");

      const size_t numLeaves = leafLevel->size();
      if (leafOrigin->size() != numLeaves || leafFormat->size() != numLeaves ||
          leafData->size() != numLeaves) {
        runtimeError(
            "node.level, node.origin, node.format, and node.data must all have "
            "the same size");
      }

      grid       = allocate<VdbGrid>(1, bytesAllocated);
      grid->type = type;
      grid->maxIteratorDepth =
          min(max(maxIteratorDepth, 0), VKL_VDB_NUM_LEVELS - 1);
      grid->totalNumLeaves = numLeaves;

      // Determine if all leaf data is compact (non-strided)
      grid->allLeavesCompact = true;

      for (size_t i = 0; i < leafData->size(); i++) {
        if (!(*leafData)[i]->compact()) {
          grid->allLeavesCompact = false;
          break;
        }
      }

      const AffineSpace3f indexToObject = loadTransform(dataIndexToObject);
      writeTransform(indexToObject, grid->indexToObject);

      AffineSpace3f objectToIndex;
      objectToIndex.l = indexToObject.l.inverse();
      objectToIndex.p = -(objectToIndex.l * indexToObject.p);
      writeTransform(objectToIndex, grid->objectToIndex);

      const box3i bbox = computeBbox(numLeaves, *leafLevel, *leafOrigin);
      grid->rootOrigin = computeRootOrigin(bbox);

      // VKL requires a float bbox. This is stored on the base class Volume.
      bounds.lower = xfmPoint(grid->indexToObject, vec3f(bbox.lower));
      bounds.upper = xfmPoint(grid->indexToObject, vec3f(bbox.upper));

      const auto binnedLeaves = binLeavesPerLevel(numLeaves, *leafLevel);
      for (size_t i = 0; i < vklVdbNumLevels(); ++i)
        grid->numLeaves[i] = binnedLeaves[i].size();
      const auto leafOffsets =
          computeLeafOffsets(numLeaves, *leafOrigin, grid->rootOrigin);

      // Allocate buffers for all levels now, all in one go. This makes
      // inserting the nodes (below) much faster.
      std::vector<uint64_t> capacity(vklVdbNumLevels() - 1, 0);
      allocateInnerLevels(
          leafOffsets, binnedLeaves, capacity, grid, bytesAllocated);

      // Populate contiguous ispc::Data1D objects to be used in leaf pointers,
      // only needed if we have strided data
      if (!grid->allLeavesCompact) {
        leafDataISPC.resize(leafData->size());

        for (size_t i = 0; i < leafDataISPC.size(); i++) {
          leafDataISPC[i].data = (*leafData)[i]->template as<float>().ispc;
        }
      }

      // TODO: Support other types?
      insertLeavesFloat(leafOffsets,
                        *leafFormat,
                        *leafData,
                        leafDataISPC,
                        binnedLeaves,
                        capacity,
                        grid);

      CALL_ISPC(VdbVolume_setGrid,
                Volume<W>::getISPCEquivalent(),
                reinterpret_cast<const ispc::VdbGrid *>(grid),
                reinterpret_cast<const ispc::VdbSampleConfig *>(&globalConfig));

      computeValueRangesFloat(
          leafOffsets, *leafLevel, *leafFormat, *leafData, grid);

      valueRange = range1f();
      for (size_t i = 0; i < vklVdbLevelNumVoxels(0); ++i)
        valueRange.extend(grid->levels[0].valueRange[i]);
    }

    template <int W>
    VKLObserver VdbVolume<W>::newObserver(const char *type)
    {
      if (!grid)
        throw std::runtime_error(
            "Trying to create an observer on a vdb volume that was not "
            "committed.");

      const std::string t(type);
      if (t == "LeafNodeAccess") {
        if (!grid->usageBuffer)
          grid->usageBuffer =
              allocate<uint32>(grid->totalNumLeaves, bytesAllocated);
        return (VKLObserver) new VdbLeafAccessObserver(
            *this, grid->totalNumLeaves, grid->usageBuffer);
      } else {
        return Volume<W>::newObserver(type);
      }
    }

    template <int W>
    Sampler<W> *VdbVolume<W>::newSampler()
    {
      return new VdbSampler<W>(grid, globalConfig);
    }

    VKL_REGISTER_VOLUME(VdbVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_vdb_, VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl
