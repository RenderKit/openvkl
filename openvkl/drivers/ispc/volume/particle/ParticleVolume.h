// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/export_util.h"
#include "../../iterator/UnstructuredIterator.h"
#include "../UnstructuredVolume.h"
#include "../Volume.h"
#include "../common/Data.h"
#include "../common/math.h"
#include "ParticleVolume_ispc.h"
#include "embree3/rtcore.h"

namespace openvkl {
  namespace ispc_driver {

    struct ParticleLeafNode : public Node
    {
      box3fa bounds;
      uint64_t cellID;

      ParticleLeafNode(unsigned id, const box3fa &bounds, const float &radius)
          : cellID(id), bounds(bounds)
      {
        // avoid inheriting LeafNode, and instead ensure size / layout is
        // compatible. this avoids size increase due to vtable, and also
        // maintains compatibility with ISPC-side code.
        static_assert(sizeof(ParticleLeafNode) == sizeof(LeafNode),
                      "ParticleLeafNode incompatible with LeafNode");
        static_assert(
            offsetof(ParticleLeafNode, bounds) == offsetof(LeafNode, bounds),
            "ParticleLeafNode incompatible with LeafNode");
        static_assert(
            offsetof(ParticleLeafNode, cellID) == offsetof(LeafNode, cellID),
            "ParticleLeafNode incompatible with LeafNode");

        nominalLength = -radius;

        // note that valueRange will be set separately
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          const RTCBuildPrimitive *prims,
                          size_t numPrims,
                          void *userPtr)
      {
        assert(numPrims == 1);

        auto id     = (uint64_t(prims->geomID) << 32) | prims->primID;
        auto radius = ((float *)userPtr)[id];

        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(ParticleLeafNode), 16);
        return (void *)new (ptr)
            ParticleLeafNode(id, *(const box3fa *)prims, radius);
      }
    };

    template <int W>
    struct ParticleVolume : public Volume<W>
    {
      ~ParticleVolume();

      void commit() override;

      const IteratorFactory<W, IntervalIterator> &getIntervalIteratorFactory()
          const override final
      {
        return intervalIteratorFactory;
      }

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      range1f getValueRange() const override;

      const Node *getNodeRoot() const
      {
        return rtcRoot;
      }

     private:
      void buildBvhAndCalculateBounds();
      void computeValueRanges();

     protected:
      box3f bounds{empty};
      range1f valueRange{empty};

      Ref<const DataT<vec3f>> positions;
      Ref<const DataT<float>> radii;
      Ref<const DataT<float>> weights;
      float radiusSupportFactor;
      float clampMaxCumulativeValue;

      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};

     private:
        UnstructuredIntervalIteratorFactory<W> intervalIteratorFactory;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline box3f ParticleVolume<W>::getBoundingBox() const
    {
      return bounds;
    }

    template <int W>
    inline range1f ParticleVolume<W>::getValueRange() const
    {
      return valueRange;
    }

    // Helper functions ///////////////////////////////////////////////////////

    inline void populateLeafNodes(Node *root,
                                  std::vector<LeafNode *> &leafNodes)
    {
      if (root->nominalLength < 0) {
        leafNodes.push_back((LeafNode *)root);
      } else {
        auto inner = (InnerNode *)root;
        populateLeafNodes(inner->children[0], leafNodes);
        populateLeafNodes(inner->children[1], leafNodes);
      }
    }

    inline void accumulateNodeValueRanges(Node *root)
    {
      if (root->nominalLength < 0) {
        // leaf node with pre-computed value range
        return;
      }

      auto inner = (InnerNode *)root;
      accumulateNodeValueRanges(inner->children[0]);
      accumulateNodeValueRanges(inner->children[1]);

      inner->valueRange = inner->children[0]->valueRange;
      inner->valueRange.extend(inner->children[1]->valueRange);

      // assuming the children bounding boxes do not completely fill this node's
      // bounding box, we would have regions of zero value
      inner->valueRange.extend(0.f);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
