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

#include "../common/Data.h"
#include "../common/math.h"
#include "MinMaxBVH2.h"
#include "UnstructuredVolume_ispc.h"
#include "Volume.h"

namespace openvkl {
  namespace ispc_driver {

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

     private:
      box4f getCellBBox(size_t id);
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

      MinMaxBVH2 bvh;
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
