// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include <embree3/rtcore.h>
#include "../common/Data.h"
#include "../common/math.h"
#include "UnstructuredVolume_ispc.h"
#include "Volume.h"

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

      void commit() override;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override;

      box3f getBoundingBox() const override;

      range1f getValueRange() const override;

      box4f getCellBBox(size_t id);

     private:
      void buildBvhAndCalculateBounds();

      // Read 32/64-bit integer value from given array
      uint64_t readInteger(const void *array, bool is32Bit, uint64_t id) const;

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

      Data *vertexPosition{nullptr};
      Data *vertexValue{nullptr};

      Data *index{nullptr};

      Data *cellIndex{nullptr};
      Data *cellValue{nullptr};
      Data *cellType{nullptr};

      bool index32Bit{false};
      bool cell32Bit{false};
      bool indexPrefixed{false};
      bool hexIterative{false};

      std::vector<vec3f> faceNormals;
      std::vector<float> iterativeTolerance;

      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline void UnstructuredVolume<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      ispc::VKLUnstructuredVolume_sample_export((const int *)&valid,
                                                this->ispcEquivalent,
                                                &objectCoordinates,
                                                &samples);
    }

    template <int W>
    inline void UnstructuredVolume<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      ispc::VKLUnstructuredVolume_gradient_export((const int *)&valid,
                                                  this->ispcEquivalent,
                                                  &objectCoordinates,
                                                  &gradients);
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
    inline uint64_t UnstructuredVolume<W>::readInteger(const void *array,
                                                       bool is32Bit,
                                                       uint64_t id) const
    {
      if (!is32Bit)
        return ((const uint64_t *)(array))[id];
      else
        return ((const uint32_t *)(array))[id];
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getCellOffset(uint64_t id) const
    {
      return readInteger(cellIndex->data, cell32Bit, id) + indexPrefixed;
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getVertexId(uint64_t id) const
    {
      return readInteger(index->data, index32Bit, id);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
