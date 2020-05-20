// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../iterator/UnstructuredIterator.h"
#include "UnstructuredVolume_ispc.h"
#include "Volume.h"
#include "embree3/rtcore.h"

namespace openvkl {
  namespace ispc_driver {

    struct Node
    {
      // 1 + 2 float = 3x4 = 18 bytes
      float nominalLength;  // set to negative for LeafNode;
      range1f valueRange;
    };

    struct LeafNode : public Node
    {
      // 18 + 8 + 4 * 6 bytes = 50
      box3fa bounds;
      uint64_t cellID;

      LeafNode(unsigned id, const box3fa &bounds, const range1f &range)
          : cellID(id), bounds(bounds)
      {
        nominalLength = -reduce_min(bounds.upper - bounds.lower);
        valueRange    = range;
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          const RTCBuildPrimitive *prims,
                          size_t numPrims,
                          void *userPtr)
      {
        assert(numPrims == 1);

        auto id    = (uint64_t(prims->geomID) << 32) | prims->primID;
        auto range = ((range1f *)userPtr)[id];

        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(LeafNode), 16);
        return (void *)new (ptr) LeafNode(id, *(const box3fa *)prims, range);
      }
    };

    struct InnerNode : public Node
    {
      // 18 + 16 + 2 * 4 * 6 = 82 bytes
      box3fa bounds[2];
      Node *children[2];

      InnerNode()
      {
        children[0] = children[1] = nullptr;
        bounds[0] = bounds[1] = empty;
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          unsigned int numChildren,
                          void *userPtr)
      {
        assert(numChildren == 2);
        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(InnerNode), 16);
        return (void *)new (ptr) InnerNode;
      }

      static void setChildren(void *nodePtr,
                              void **childPtr,
                              unsigned int numChildren,
                              void *userPtr)
      {
        assert(numChildren == 2);
        auto innerNode = (InnerNode *)nodePtr;
        for (size_t i = 0; i < 2; i++)
          innerNode->children[i] = (Node *)childPtr[i];
        innerNode->nominalLength =
            min(fabs(innerNode->children[0]->nominalLength),
                fabs(innerNode->children[1]->nominalLength));
        innerNode->valueRange = innerNode->children[0]->valueRange;
        innerNode->valueRange.extend(innerNode->children[1]->valueRange);
      }

      static void setBounds(void *nodePtr,
                            const RTCBounds **bounds,
                            unsigned int numChildren,
                            void *userPtr)
      {
        assert(numChildren == 2);
        for (size_t i = 0; i < 2; i++)
          ((InnerNode *)nodePtr)->bounds[i] = *(const box3fa *)bounds[i];
      }
    };

    template <int W>
    struct UnstructuredVolume : public Volume<W>
    {
      ~UnstructuredVolume();

      std::string toString() const override;

      void commit() override;

      void initIntervalIteratorV(
          const vintn<W> &valid,
          vVKLIntervalIteratorN<W> &iterator,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalIteratorN<W> &iterator,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients) const override;

      box3f getBoundingBox() const override;

      range1f getValueRange() const override;

      box4f getCellBBox(size_t id);

      const Node *getNodeRoot() const
      {
        return rtcRoot;
      }

     private:
      void buildBvhAndCalculateBounds();

      // Read 32/64-bit integer value from given array
      uint64_t readInteger(Ref<const Data> array,
                           bool is32Bit,
                           uint64_t id) const;

      // Read from index arrays that could have 32/64-bit element size
      uint64_t getCellOffset(uint64_t id) const;
      uint64_t getVertexId(uint64_t id) const;

      void calculateCellNormals(const uint64_t cellId,
                                const uint32_t faces[6][3],
                                const uint32_t facesCount);
      void calculateFaceNormals();

      void calculateTolerance(const uint64_t cellId,
                              const uint32_t edge[][2],
                              const uint32_t count);
      void calculateIterativeTolerance();

     protected:
      uint64_t nCells{0};
      box3f bounds{empty};
      range1f valueRange{empty};

      Ref<const DataT<vec3f>> vertexPosition;
      Ref<const DataT<float>> vertexValue;

      Ref<const Data> index;

      Ref<const Data> cellIndex;
      Ref<const DataT<float>> cellValue;
      Ref<const DataT<uint8_t>> cellType;

      bool index32Bit{false};
      bool cell32Bit{false};
      bool indexPrefixed{false};
      bool hexIterative{false};

      // used only if an explicit cell type array is not provided
      std::vector<uint8_t> generatedCellType;

      std::vector<vec3f> faceNormals;
      std::vector<float> iterativeTolerance;

      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string UnstructuredVolume<W>::toString() const
    {
      return "openvkl::UnstructuredVolume";
    }

    template <int W>
    inline void UnstructuredVolume<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      CALL_ISPC(VKLUnstructuredVolume_sample_export,
                static_cast<const int *>(valid),
                this->ispcEquivalent,
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void UnstructuredVolume<W>::initIntervalIteratorV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLIntervalIterator<UnstructuredIterator<W>>(
          iterator, valid, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void UnstructuredVolume<W>::iterateIntervalV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        vVKLIntervalN<W> &interval,
        vintn<W> &result)
    {
      UnstructuredIterator<W> *ri =
          fromVKLIntervalIterator<UnstructuredIterator<W>>(&iterator);

      ri->iterateInterval(valid, result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<W> *>(ri->getCurrentInterval());
    }

    template <int W>
    inline void UnstructuredVolume<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      CALL_ISPC(VKLUnstructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                this->ispcEquivalent,
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline void UnstructuredVolume<W>::computeGradientN(
        unsigned int N,
        const vvec3fn<1> *objectCoordinates,
        vvec3fn<1> *gradients) const
    {
      CALL_ISPC(Volume_gradient_N_export,
                this->ispcEquivalent,
                N,
                (ispc::vec3f *)objectCoordinates,
                (ispc::vec3f *)gradients);
    }

    template <int W>
    inline box3f UnstructuredVolume<W>::getBoundingBox() const
    {
      return bounds;
    }

    template <int W>
    inline range1f UnstructuredVolume<W>::getValueRange() const
    {
      return valueRange;
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::readInteger(Ref<const Data> array,
                                                       bool is32Bit,
                                                       uint64_t id) const
    {
      if (!is32Bit)
        return array->as<uint64_t>()[id];
      else
        return array->as<uint32_t>()[id];
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getCellOffset(uint64_t id) const
    {
      return readInteger(cellIndex, cell32Bit, id) + indexPrefixed;
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getVertexId(uint64_t id) const
    {
      return readInteger(index, index32Bit, id);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
