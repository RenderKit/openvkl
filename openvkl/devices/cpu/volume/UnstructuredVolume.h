// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "UnstructuredBVH.h"
#include "UnstructuredVolume_ispc.h"
#include "Volume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct UnstructuredVolume : public Volume<W>
    {
      ~UnstructuredVolume();

      std::string toString() const override;

      void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      unsigned int getNumAttributes() const override;

      range1f getValueRange(unsigned int attributeIndex) const override;

      box4f getCellBBox(size_t id);

      const Node *getNodeRoot() const;

      int getBvhDepth() const;

     private:
      void buildBvhAndCalculateBounds();

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

      Ref<const DataT<uint32_t>> index32;
      Ref<const DataT<uint64_t>> index64;

      Ref<const DataT<uint32_t>> cellIndex32;
      Ref<const DataT<uint64_t>> cellIndex64;

      Ref<const DataT<float>> cellValue;
      Ref<const DataT<uint8_t>> cellType;

      Ref<const DataT<float>> background;

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
      int bvhDepth{0};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string UnstructuredVolume<W>::toString() const
    {
      return "openvkl::UnstructuredVolume";
    }

    template <int W>
    inline box3f UnstructuredVolume<W>::getBoundingBox() const
    {
      return bounds;
    }

    template <int W>
    inline unsigned int UnstructuredVolume<W>::getNumAttributes() const
    {
      return 1;
    }

    template <int W>
    inline range1f UnstructuredVolume<W>::getValueRange(
        unsigned int attributeIndex) const
    {
      throwOnIllegalAttributeIndex(this, attributeIndex);
      return valueRange;
    }

    template <int W>
    inline const Node *UnstructuredVolume<W>::getNodeRoot() const
    {
      return rtcRoot;
    }

    template <int W>
    inline int UnstructuredVolume<W>::getBvhDepth() const
    {
      return bvhDepth;
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getCellOffset(uint64_t id) const
    {
      return (cell32Bit ? (*cellIndex32)[id] : (*cellIndex64)[id]) +
             indexPrefixed;
    }

    template <int W>
    inline uint64_t UnstructuredVolume<W>::getVertexId(uint64_t id) const
    {
      return index32Bit ? (*index32)[id] : (*index64)[id];
    }

  }  // namespace cpu_device
}  // namespace openvkl
