// Copyright 2020-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../common/export_util.h"
#include "../UnstructuredBVH.h"
#include "../UnstructuredVolume.h"
#include "../Volume.h"
#include "../common/Data.h"
#include "../common/math.h"
#include "ParticleVolume_ispc.h"

#define MAX_PRIMS_PER_LEAF VKL_TARGET_WIDTH

namespace openvkl {
  namespace cpu_device {

    struct ParticleLeafNode : public LeafNodeMulti
    {
      ParticleLeafNode(uint64_t numCells,
                       uint64_t *ids,
                       const box3fa &bounds,
                       const float &minRadius)
          : LeafNodeMulti(numCells, ids, bounds, empty)
      {
        // ISPC-side code assumes the same layout as LeafNode
        static_assert(sizeof(ParticleLeafNode) == sizeof(LeafNodeMulti),
                      "ParticleLeafNode incompatible with LeafNodeMulti");

        assert(minRadius > 0.f);

        nominalLength.x = -minRadius;
        nominalLength.y = minRadius;
        nominalLength.z = minRadius;

        // note that valueRange will be set separately in computeValueRanges()
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          const RTCBuildPrimitive *prims,
                          size_t numPrims,
                          void *userPtr)
      {
        assert(numPrims > 0 && numPrims <= MAX_PRIMS_PER_LEAF);

        uint64_t *ids = static_cast<uint64_t *>(
            rtcThreadLocalAlloc(alloc, numPrims * sizeof(uint64_t), 16));
        float minRadius = inf;
        box3fa bounds   = empty;

        for (size_t i = 0; i < numPrims; i++) {
          const uint64_t id =
              (uint64_t(prims[i].geomID) << 32) | prims[i].primID;
          const float radius = ((float *)userPtr)[id];
          const box3fa bound = *(const box3fa *)(&prims[i]);

          ids[i]    = id;
          minRadius = std::min(minRadius, radius);
          bounds.extend(bound);
        }

        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(ParticleLeafNode), 16);
        return (void *)new (ptr)
            ParticleLeafNode(numPrims, ids, bounds, minRadius);
      }
    };

    template <int W>
    struct ParticleVolume : public Volume<W>
    {
      ~ParticleVolume();

      void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      unsigned int getNumAttributes() const override;

      range1f getValueRange(unsigned int attributeIndex) const override;

      const Node *getNodeRoot() const;

      int getBvhDepth() const;

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
      bool estimateValueRanges;

      Ref<const DataT<float>> background;

      // number of particles included in the BVH, which will not include any
      // zero-radius particles
      size_t numBVHParticles{0};

      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};
      int bvhDepth{0};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline box3f ParticleVolume<W>::getBoundingBox() const
    {
      return bounds;
    }

    template <int W>
    unsigned int ParticleVolume<W>::getNumAttributes() const
    {
      return 1;
    }

    template <int W>
    inline range1f ParticleVolume<W>::getValueRange(
        unsigned int attributeIndex) const
    {
      throwOnIllegalAttributeIndex(this, attributeIndex);
      return valueRange;
    }

    template <int W>
    inline const Node *ParticleVolume<W>::getNodeRoot() const
    {
      return rtcRoot;
    }

    template <int W>
    inline int ParticleVolume<W>::getBvhDepth() const
    {
      return bvhDepth;
    }

    // Helper functions ///////////////////////////////////////////////////////

    // Only applicable to particle volumes; other unstructured types have value
    // ranges accumulated during build / construction
    inline void accumulateNodeValueRanges(Node *root)
    {
      if (isLeafNode(root)) {
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

  }  // namespace cpu_device
}  // namespace openvkl
