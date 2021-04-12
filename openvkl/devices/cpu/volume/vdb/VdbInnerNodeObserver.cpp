// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VdbInnerNodeObserver.h"
#include <atomic>
#include "VdbGrid.h"
#include "VdbVolume.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace cpu_device {

    template <class F>
    void visitVoxels(const VdbGrid &grid, uint32_t maxDepth, const F &functor)
    {
      maxDepth = std::min<uint32_t>(maxDepth, VKL_VDB_NUM_LEVELS - 1);
      for (uint32_t l = 0; l <= maxDepth; ++l) {
        const VdbLevel &level     = grid.levels[l];
        const size_t numNodes     = level.numNodes;
        const size_t logRes       = vklVdbLevelLogRes(l);
        const uint32_t storageRes = (1 << logRes);

        for (size_t n = 0; n < numNodes; ++n) {
          tasking::parallel_for(storageRes, [&](uint32_t vx) {
            for (uint32_t vy = 0; vy < storageRes; ++vy)
              for (uint32_t vz = 0; vz < storageRes; ++vz) {
                const size_t voxelIdx =
                    (static_cast<size_t>(vx) << (2 * logRes)) +
                    (static_cast<size_t>(vy) << logRes) +
                    static_cast<size_t>(vz);
                const size_t nodeVoxelBaseIdx = (n << (3 * logRes));
                functor(l, n, voxelIdx + nodeVoxelBaseIdx, vx, vy, vz);
              }
          });
        }
      }
    }

    template <int W>
    VdbInnerNodeObserver<W>::VdbInnerNodeObserver(VdbVolume<W> &target)
        : Observer<W>(target)
    {
    }

    template <int W>
    VdbInnerNodeObserver<W>::~VdbInnerNodeObserver()
    {
      clear();
    }

    template <int W>
    const void *VdbInnerNodeObserver<W>::map()
    {
      if (!buffer) {
        commit();
      }
      return buffer;
    }

    template <int W>
    void VdbInnerNodeObserver<W>::unmap()
    {
    }

    template <int W>
    size_t VdbInnerNodeObserver<W>::getNumElements() const
    {
      return size;
    }

    template <int W>
    VKLDataType VdbInnerNodeObserver<W>::getElementType() const
    {
      return VKL_OBJECT;
    }

    template <int W>
    size_t VdbInnerNodeObserver<W>::getElementSize() const
    {
      return sizeof(float) * numFloats;
    }

    template <int W>
    void VdbInnerNodeObserver<W>::clear()
    {
      allocator.deallocate(buffer);
      numFloats = 0;
      size      = 0;
    }

    template <int W>
    void VdbInnerNodeObserver<W>::commit()
    {
      const uint32_t maxDepth = this->template getParam<int>("maxDepth", 1);

      clear();

      const VdbVolume<W> &volume = dynamic_cast<const VdbVolume<W> &>(*target);
      const VdbGrid *grid        = volume.getGrid();
      assert(grid);

      std::atomic<size_t> numOutputNodes(0);

      visitVoxels(*grid,
                  maxDepth,
                  [&](uint32_t l,
                      size_t n,
                      size_t vidx,
                      uint32_t vx,
                      uint32_t vy,
                      uint32_t vz) {
                    const VdbLevel &level = grid->levels[l];
                    const uint64_t voxel  = level.voxels[vidx];
                    if (vklVdbVoxelIsLeafPtr(voxel) ||
                        (l == maxDepth && !vklVdbVoxelIsEmpty(voxel))) {
                      ++numOutputNodes;
                    }
                  });

      // bb min, bb max, value range, value range, ...
      numFloats = 6 + 2 * grid->numAttributes;
      size      = numOutputNodes.load();
      buffer    = allocator.allocate<float>(size * numFloats);
      std::atomic<size_t> currentOutputNode(0);

      visitVoxels(
          *grid,
          maxDepth,
          [&](uint32_t l,
              size_t n,
              size_t vidx,
              uint32_t vx,
              uint32_t vy,
              uint32_t vz) {
            const VdbLevel &level = grid->levels[l];
            const uint64_t voxel  = level.voxels[vidx];

            if (vklVdbVoxelIsLeafPtr(voxel) ||
                (l == maxDepth && !vklVdbVoxelIsEmpty(voxel))) {
              const size_t outputIdx  = currentOutputNode++;
              const uint32_t totalRes = (1 << vklVdbLevelTotalLogRes(l + 1));
              const vec3ui offset     = totalRes * vec3ui(vx, vy, vz);
              const vec3i origin = grid->rootOrigin + level.origin[n] + offset;
              const vec3f bbMin(origin.x, origin.y, origin.z);
              const vec3f bbMax    = bbMin + vec3f(totalRes);
              const vec3f bbMinObj = xfmPoint(grid->indexToObject, bbMin);
              const vec3f bbMaxObj = xfmPoint(grid->indexToObject, bbMax);
              float *node          = buffer + outputIdx * numFloats;
              node[0]              = bbMinObj.x;
              node[1]              = bbMinObj.y;
              node[2]              = bbMinObj.z;
              node[3]              = bbMaxObj.x;
              node[4]              = bbMaxObj.y;
              node[5]              = bbMaxObj.z;
              const size_t vrIdx   = vidx * grid->numAttributes;
              for (uint32_t a = 0; a < grid->numAttributes; ++a) {
                node[6 + 2 * a] = level.valueRange[vrIdx + a].lower;
                node[7 + 2 * a] = level.valueRange[vrIdx + a].upper;
              }
            }
          });
      assert(currentOutputNode.load() == numOutputNodes.load());
    }

    template struct VdbInnerNodeObserver<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
