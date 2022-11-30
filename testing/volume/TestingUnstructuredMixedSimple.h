// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingVolume.h"

namespace openvkl {
  namespace testing {

    struct UnstructuredVolumeMixedSimple : public TestingVolume
    {
      UnstructuredVolumeMixedSimple(){};

      void generateVKLVolume(VKLDevice device)
      {
        // define hexahedron parameters
        const float hSize = .4f;
        const float hX = -.5f, hY = -.5f, hZ = 1.f;

        // define wedge parameters
        const float wSize = .4f;
        const float wX = .5f, wY = -.5f, wZ = 1.f;

        // define tetrahedron parameters
        const float tSize = .4f;
        const float tX = .5f, tY = .5f, tZ = 1.f;

        // define pyramid parameters
        const float pSize = .4f;
        const float pX = -.5f, pY = .5f, pZ = 1.f;

        // define vertex positions
        std::vector<vec3f> vertices = {
            // hexahedron
            {-hSize + hX, -hSize + hY, hSize + hZ},  // bottom quad
            {hSize + hX, -hSize + hY, hSize + hZ},
            {hSize + hX, -hSize + hY, -hSize + hZ},
            {-hSize + hX, -hSize + hY, -hSize + hZ},
            {-hSize + hX, hSize + hY, hSize + hZ},  // top quad
            {hSize + hX, hSize + hY, hSize + hZ},
            {hSize + hX, hSize + hY, -hSize + hZ},
            {-hSize + hX, hSize + hY, -hSize + hZ},

            // wedge
            {-wSize + wX, -wSize + wY, wSize + wZ},  // bottom triangle
            {wSize + wX, -wSize + wY, 0.f + wZ},
            {-wSize + wX, -wSize + wY, -wSize + wZ},
            {-wSize + wX, wSize + wY, wSize + wZ},  // top triangle
            {wSize + wX, wSize + wY, 0.f + wZ},
            {-wSize + wX, wSize + wY, -wSize + wZ},

            // tetrahedron
            {-tSize + tX, -tSize + tY, tSize + tZ},
            {tSize + tX, -tSize + tY, 0.f + tZ},
            {-tSize + tX, -tSize + tY, -tSize + tZ},
            {-tSize + tX, tSize + tY, 0.f + tZ},

            // pyramid
            {-pSize + pX, -pSize + pY, pSize + pZ},
            {pSize + pX, -pSize + pY, pSize + pZ},
            {pSize + pX, -pSize + pY, -pSize + pZ},
            {-pSize + pX, -pSize + pY, -pSize + pZ},
            {pSize + pX, pSize + pY, 0.f + pZ}};

        // define per-vertex values
        std::vector<float> vertexValues = {// hexahedron
                                           0.f,
                                           0.f,
                                           0.f,
                                           0.f,
                                           0.f,
                                           1.f,
                                           1.f,
                                           0.f,

                                           // wedge
                                           0.f,
                                           0.f,
                                           0.f,
                                           1.f,
                                           0.f,
                                           1.f,

                                           // tetrahedron
                                           1.f,
                                           0.f,
                                           1.f,
                                           0.f,

                                           // pyramid
                                           0.f,
                                           1.f,
                                           1.f,
                                           0.f,
                                           0.f};

        // define vertex indices for both shared and separate case
        std::vector<uint32_t> indicesSharedVert = {// hexahedron
                                                   0,
                                                   1,
                                                   2,
                                                   3,
                                                   4,
                                                   5,
                                                   6,
                                                   7,

                                                   // wedge
                                                   1,
                                                   9,
                                                   2,
                                                   5,
                                                   12,
                                                   6,

                                                   // tetrahedron
                                                   5,
                                                   12,
                                                   6,
                                                   17,

                                                   // pyramid
                                                   4,
                                                   5,
                                                   6,
                                                   7,
                                                   17};

        std::vector<uint32_t> &indices = indicesSharedVert;

        // define cell offsets in indices array
        std::vector<uint32_t> cells = {0, 8, 14, 18};

        // define cell types
        std::vector<uint8_t> cellTypes = {
            VKL_HEXAHEDRON, VKL_WEDGE, VKL_TETRAHEDRON, VKL_PYRAMID};

        volume = vklNewVolume(device, "unstructured");

        VKLData cellData =
            vklNewData(device, cells.size(), VKL_UINT, cells.data());
        vklSetData2(volume, "cell.index", cellData);
        vklRelease(cellData);

        VKLData celltypeData =
            vklNewData(device, cellTypes.size(), VKL_UCHAR, cellTypes.data());
        vklSetData2(volume, "cell.type", celltypeData);
        vklRelease(celltypeData);

        VKLData valuesData = vklNewData(
            device, vertexValues.size(), VKL_FLOAT, vertexValues.data());
        vklSetData2(volume, "vertex.data", valuesData);
        vklRelease(valuesData);

        VKLData vtxPositionsData =
            vklNewData(device, vertices.size(), VKL_VEC3F, vertices.data());
        vklSetData2(volume, "vertex.position", vtxPositionsData);
        vklRelease(vtxPositionsData);

        VKLData topologyData =
            vklNewData(device, indices.size(), VKL_UINT, indices.data());
        vklSetData2(volume, "index", topologyData);
        vklRelease(topologyData);

        vklCommit2(volume);

        computedValueRange = computeValueRange(
            VKL_FLOAT, vertexValues.data(), vertexValues.size());
      };

      range1f getComputedValueRange() const
      {
        if (computedValueRange.empty()) {
          throw std::runtime_error(
              "computedValueRange only available after VKL volume is "
              "generated");
        }

        return computedValueRange;
      };

     private:
      range1f computedValueRange = range1f(rkcommon::math::empty);
    };
  }  // namespace testing
}  // namespace openvkl
