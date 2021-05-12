// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/openvkl.h>
#include <openvkl/vdb.h>
#include <openvkl/vdb_util/VdbVolumeBuffers.h>

#include <openvdb/openvdb.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

static_assert((VKL_VDB_NUM_LEVELS == 4) && (VKL_VDB_LOG_RES_0 == 6) &&
                  (VKL_VDB_LOG_RES_1 == 5) && (VKL_VDB_LOG_RES_2 == 4) &&
                  (VKL_VDB_LOG_RES_3 == 3),
              "You must compile Open VKL with vdb log resolution 6;5;4;3 to "
              "use OpenVdbGrid");

namespace openvkl {
  namespace utility {
    namespace vdb {

    using vec3i = rkcommon::math::vec3i;
    using vec3f = rkcommon::math::vec3f;

    /*
     * Helper functions for interfacing with VdbVolumeBuffers.
     */
    inline size_t addTile(VdbVolumeBuffers &volumeBuffers,
                          uint32_t level,
                          const vec3i &origin,
                          const float *p)
    {
      return volumeBuffers.addTile(level, origin, {(void *)p});
    }

    inline size_t addTile(VdbVolumeBuffers &volumeBuffers,
                          uint32_t level,
                          const vec3i &origin,
                          const openvdb::Vec3s *p)
    {
      const float &x = p->x();
      const float &y = p->y();
      const float &z = p->z();
      return volumeBuffers.addTile(
          level, origin, {(void *)&x, (void *)&y, (void *)&z});
    }

    inline size_t addConstant(VdbVolumeBuffers &volumeBuffers,
                              uint32_t level,
                              const vec3i &origin,
                              const float *p)
    {
      return volumeBuffers.addConstant(
          level, origin, {(void *)p}, VKL_DATA_SHARED_BUFFER);
    }

    inline size_t addConstant(VdbVolumeBuffers &volumeBuffers,
                              uint32_t level,
                              const vec3i &origin,
                              const openvdb::Vec3s *p)
    {
      return volumeBuffers.addConstant(
          level,
          origin,
          {(void *)(p->asPointer() + 0),
           (void *)(p->asPointer() + 1),
           (void *)(p->asPointer() + 2)},
          VKL_DATA_SHARED_BUFFER,
          std::vector<size_t>(3, sizeof(openvdb::Vec3s)));
    }

    inline void makeConstant(VdbVolumeBuffers &volumeBuffers,
                             size_t index,
                             const float *p)
    {
      volumeBuffers.makeConstant(index, {(void *)p}, VKL_DATA_SHARED_BUFFER);
    }

    inline void makeConstant(VdbVolumeBuffers &volumeBuffers,
                             size_t index,
                             const openvdb::Vec3s *p)
    {
      volumeBuffers.makeConstant(
          index,
          {(void *)(p->asPointer() + 0),
           (void *)(p->asPointer() + 1),
           (void *)(p->asPointer() + 2)},
          VKL_DATA_SHARED_BUFFER,
          std::vector<size_t>(3, sizeof(openvdb::Vec3s)));
    }

    /*
     * For specializations of OpenVdbGrid.
     */
    template <typename VdbFieldType>
    struct OpenVdbGridTypes
    {
      using openvdbNativeGrid                          = void;
      using openvdbTree                                = void;
      const std::string openvdbTreeString              = "";
      const std::vector<VKLDataType> vklAttributeTypes = {};
    };

    template <>
    struct OpenVdbGridTypes<float>
    {
      using openvdbNativeGrid                          = openvdb::FloatGrid;
      using openvdbTree                                = openvdb::FloatTree;
      const std::string openvdbTreeString              = "Tree_float_5_4_3";
      const std::vector<VKLDataType> vklAttributeTypes = {VKL_FLOAT};
    };

    template <>
    struct OpenVdbGridTypes<openvdb::Vec3s>
    {
      using openvdbNativeGrid                          = openvdb::Vec3SGrid;
      using openvdbTree                                = openvdb::Vec3STree;
      const std::string openvdbTreeString              = "Tree_vec3s_5_4_3";
      const std::vector<VKLDataType> vklAttributeTypes = {
          VKL_FLOAT, VKL_FLOAT, VKL_FLOAT};
    };

    /*
     * This class demonstrates how a grid can be loaded from OpenVDB .vdb
     * files, and then forwarded to OpenVKL as a "vdb" volume.
     */
    template <typename VdbFieldType>
    class OpenVdbGrid
    {
      using openvdbNativeGrid =
          typename OpenVdbGridTypes<VdbFieldType>::openvdbNativeGrid;

      using openvdbTree = typename OpenVdbGridTypes<VdbFieldType>::openvdbTree;

      std::vector<VKLDataType> vklAttributeTypes =
          OpenVdbGridTypes<VdbFieldType>().vklAttributeTypes;

      std::string openvdbTreeString =
          OpenVdbGridTypes<VdbFieldType>().openvdbTreeString;

     public:
      /*
       * We support deferred loading of leaf data. We use this struct to
       * remind ourselves where the deferred nodes are.
       */
      struct Deferred
      {
        size_t index{0};
        const openvdb::tree::LeafBuffer<VdbFieldType, 3> *leafBuffer{nullptr};

        Deferred() = default;
        Deferred(size_t index,
                 const openvdb::tree::LeafBuffer<VdbFieldType, 3> *leafBuffer)
            : index(index), leafBuffer(leafBuffer)
        {
        }
      };

      /*
       * This builder can traverse the OpenVDB tree, generating
       * nodes for us along the way.
       */
      template <typename VdbNodeType, typename Enable = void>
      struct Builder
      {
        using ChildNodeType = typename VdbNodeType::ChildNodeType;
        static constexpr uint32_t level{VKL_VDB_NUM_LEVELS - 1 -
                                        VdbNodeType::LEVEL};
        static constexpr uint32_t nextLevel{level + 1};

        static void visit(const VdbNodeType &vdbNode,
                          VdbVolumeBuffers &volumeBuffers,
                          std::vector<Deferred> &deferred)
        {
          for (auto it = vdbNode.cbeginValueOn(); it; ++it) {
            const auto &coord = it.getCoord();
            const vec3i origin(coord[0], coord[1], coord[2]);
            addTile(volumeBuffers, nextLevel, origin, &*it);
          }

          for (auto it = vdbNode.cbeginChildOn(); it; ++it)
            Builder<ChildNodeType>::visit(*it, volumeBuffers, deferred);
        }
      };

      /*
       * We stop the recursion one level above the leaf level.
       * By default, this is level 2.
       */
      template <typename VdbNodeType>
      struct Builder<VdbNodeType,
                     typename std::enable_if<(VdbNodeType::LEVEL == 1)>::type>
      {
        static constexpr uint32_t level{VKL_VDB_NUM_LEVELS - 1 -
                                        VdbNodeType::LEVEL};
        static constexpr uint32_t nextLevel{level + 1};
        static_assert(nextLevel == VKL_VDB_NUM_LEVELS - 1,
                      "OpenVKL is not compiled to match OpenVDB tree type");

        static void visit(const VdbNodeType &vdbNode,
                          VdbVolumeBuffers &volumeBuffers,
                          std::vector<Deferred> &deferred)
        {
          const uint32_t storageRes = vklVdbLevelStorageRes(level);
          const uint32_t childRes   = vklVdbLevelRes(nextLevel);
          const auto *vdbVoxels     = vdbNode.getTable();
          const vec3i origin        = vec3i(
              vdbNode.origin()[0], vdbNode.origin()[1], vdbNode.origin()[2]);

          // Note: OpenVdb stores data in z-major order!
          uint64_t vIdx = 0;
          const static VdbFieldType one =
              VdbFieldType(1.f);  // Deferred leaves.
          for (uint32_t x = 0; x < storageRes; ++x)
            for (uint32_t y = 0; y < storageRes; ++y)
              for (uint32_t z = 0; z < storageRes; ++z, ++vIdx) {
                const bool isTile  = vdbNode.isValueMaskOn(vIdx);
                const bool isChild = vdbNode.isChildMaskOn(vIdx);
                const bool isDeferred =
                    isChild && (deferred.capacity() > deferred.size());

                if (!(isTile || isChild))
                  continue;

                const auto &nodeUnion   = vdbVoxels[vIdx];
                const vec3i childOrigin = origin + childRes * vec3i(x, y, z);
                if (isTile)
                  addTile(volumeBuffers,
                          nextLevel,
                          childOrigin,
                          &nodeUnion.getValue());
                else if (isDeferred) {
                  const size_t idx =
                      addTile(volumeBuffers, nextLevel, childOrigin, &one);
                  deferred.emplace_back(idx, &nodeUnion.getChild()->buffer());
                } else if (isChild) {
                  addConstant(volumeBuffers,
                              nextLevel,
                              childOrigin,
                              nodeUnion.getChild()->buffer().data());
                }
              }
        }
      };

     public:
      OpenVdbGrid() = default;

      /*
       * Load the given file.
       * If deferLeaves is true, then do not load leaf data but instead add
       * tiles.
       */
      OpenVdbGrid(VKLDevice device,
                  const std::string &path,
                  const std::string &field,
                  bool deferLeaves = false);

      /*
       * Load the given grid.
       * If deferLeaves is true, then do not load leaf data but instead add
       * tiles.
       */
      OpenVdbGrid(VKLDevice device,
                  typename openvdbNativeGrid::Ptr vdb,
                  bool deferLeaves = false);

      /*
       * To creat a VKLVolume, we simply set our parameters and commit.
       */
      VKLVolume createVolume(VKLFilter filter) const;

      /*
       * Return the number of nodes in this grid.
       * This includes tiles and leaf nodes.
       */
      size_t numNodes() const;

      /*
       * API for deferred loading.
       */
      size_t numDeferred() const;

      /*
       * Load all deferred nodes for which the leaf access observer has
       * seen access.
       */
      void loadDeferred(VKLObserver leafAccessObserver);

      /*
       * Load as many deferred nodes as possible in maxTimeMS milliseconds,
       * or load them all if maxTimeMS is 0.
       */
      void loadDeferred(size_t maxTimeMS);

      VKLDevice getVKLDevice() const;

     private:
      void loadTransform();
      void loadDeferredAt(size_t i);
      void loadFromGrid(typename openvdbNativeGrid::Ptr vdb,
                        bool deferLeaves = false);

     private:
      std::unique_ptr<VdbVolumeBuffers> buffers;
      std::vector<Deferred> deferred;
      openvdb::GridBase::Ptr grid{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VdbFieldType>
    inline OpenVdbGrid<VdbFieldType>::OpenVdbGrid(VKLDevice device,
                                                  const std::string &path,
                                                  const std::string &field,
                                                  bool deferLeaves)
        : buffers(new VdbVolumeBuffers(device, vklAttributeTypes))
    {
      openvdb::initialize();  // Must initialize first! It's ok to do this
                              // multiple times.

      try {
        openvdb::io::File file(path);
        file.open();
        grid = file.readGrid(field);
        file.close();
      } catch (const std::exception &e) {
        throw std::runtime_error(e.what());
      }

      // We only support the default topology in this loader.
      if (grid->type() != openvdbTreeString)
        throw std::runtime_error(std::string("Incorrect tree type: ") +
                                 grid->type());

      // Preallocate memory for all leaves; this makes loading the tree much
      // faster.
      typename openvdbNativeGrid::Ptr vdb =
          openvdb::gridPtrCast<openvdbNativeGrid>(grid);
      loadFromGrid(vdb, deferLeaves);
    }

    template <typename VdbFieldType>
    inline OpenVdbGrid<VdbFieldType>::OpenVdbGrid(
        VKLDevice device, typename openvdbNativeGrid::Ptr vdb, bool deferLeaves)
        : buffers(new VdbVolumeBuffers(device, vklAttributeTypes))
    {
      openvdb::initialize();
      grid = vdb;
      loadFromGrid(vdb, deferLeaves);
    }

    template <typename VdbFieldType>
    inline VKLVolume OpenVdbGrid<VdbFieldType>::createVolume(
        VKLFilter filter) const
    {
      assert(buffers);
      return buffers->createVolume(filter);
    }

    template <typename VdbFieldType>
    inline size_t OpenVdbGrid<VdbFieldType>::numNodes() const
    {
      return buffers ? buffers->numNodes() : 0;
    }

    template <typename VdbFieldType>
    inline size_t OpenVdbGrid<VdbFieldType>::numDeferred() const
    {
      return deferred.size();
    }

    template <typename VdbFieldType>
    inline void OpenVdbGrid<VdbFieldType>::loadDeferred(
        VKLObserver leafAccessObserver)
    {
      if (!buffers || deferred.empty())
        return;

      const vkl_uint32 *buffer =
          static_cast<const vkl_uint32 *>(vklMapObserver(leafAccessObserver));
      if (!buffer)
        throw std::runtime_error("cannot map leaf access observer buffer.");

      assert(vklGetObserverNumElements(leafAccessObserver) ==
             buffers->numNodes());
      assert(vklGetObserverElementType(leafAccessObserver) == VKL_UINT);

      size_t i = 0;
      while (i < deferred.size())  // loadDeferredAt reduces deferred.size() !
      {
        if (buffer[deferred.at(i).index] >
            0)  // Do not increment i now, this method reduces size!.
          loadDeferredAt(i);
        else
          ++i;
      }

      vklUnmapObserver(leafAccessObserver);
    }

    template <typename VdbFieldType>
    inline void OpenVdbGrid<VdbFieldType>::loadDeferred(size_t maxTimeMS)
    {
      if (!buffers || deferred.empty())
        return;

      namespace chr  = std::chrono;
      using Clock    = chr::steady_clock;
      using TimeUnit = chr::milliseconds;
      const TimeUnit maxTime(maxTimeMS);
      const auto start = Clock::now();

      while (!deferred.empty() &&
             (maxTimeMS == 0 ||
              chr::duration_cast<TimeUnit>(Clock::now() - start) <= maxTime)) {
        loadDeferredAt(0);
      }
    }

    template <typename VdbFieldType>
    inline VKLDevice OpenVdbGrid<VdbFieldType>::getVKLDevice() const
    {
      if (buffers) {
        return buffers->getVKLDevice();
      } else {
        return nullptr;
      }
    }

    template <typename VdbFieldType>
    inline void OpenVdbGrid<VdbFieldType>::loadTransform()
    {
      assert(grid);
      const auto &indexToObject = grid->transform().baseMap();
      if (!indexToObject->isLinear())
        throw std::runtime_error(
            "OpenVKL only supports linearly transformed volumes");

      // Transpose; OpenVDB stores column major (and a full 4x4 matrix).
      const auto &ri2o = indexToObject->getAffineMap()->getMat4();
      const auto *i2o  = ri2o.asPointer();
      buffers->setIndexToObject(i2o[0],
                                i2o[1],
                                i2o[2],
                                i2o[4],
                                i2o[5],
                                i2o[6],
                                i2o[8],
                                i2o[9],
                                i2o[10],
                                i2o[12],
                                i2o[13],
                                i2o[14]);
    }

    template <typename VdbFieldType>
    inline void OpenVdbGrid<VdbFieldType>::loadDeferredAt(size_t i)
    {
      using std::swap;
      const Deferred &d  = deferred.at(i);
      const size_t index = d.index;
      assert(d.leafBuffer);

      makeConstant(*buffers, index, d.leafBuffer->data());

      // Having loaded the leaf, swap to the end and discard.
      const size_t newSize = deferred.size() - 1;
      swap(deferred.at(i), deferred.at(newSize));
      deferred.resize(newSize);
    }

    template <typename VdbFieldType>
    inline void OpenVdbGrid<VdbFieldType>::loadFromGrid(
        typename openvdbNativeGrid::Ptr vdb, bool deferLeaves)
    {
      const size_t numTiles  = vdb->tree().activeTileCount();
      const size_t numLeaves = vdb->tree().leafCount();
      buffers->reserve(numTiles + numLeaves);
      if (deferLeaves)
        deferred.reserve(numLeaves);

      loadTransform();

      const auto &root = vdb->tree().root();
      for (auto it = root.cbeginChildOn(); it; ++it)
        Builder<typename openvdbTree::RootNodeType::ChildNodeType>::visit(
            *it, *buffers, deferred);
    }

    // Type aliases ///////////////////////////////////////////////////////////

    using OpenVdbFloatGrid = OpenVdbGrid<float>;
    using OpenVdbVec3sGrid = OpenVdbGrid<openvdb::Vec3s>;

    }  // namespace vdb
  }  // namespace utility
}  // namespace openvkl
