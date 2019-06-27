// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "UnstructuredVolume.h"
#include "../common/Data.h"
#include "ospcommon/tasking/parallel_for.h"

// Map cell type to its vertices count
inline uint32_t getVerticesCount(uint8_t cellType)
{
  switch (cellType) {
  case VKL_TETRAHEDRON:
    return 4;
  case VKL_HEXAHEDRON:
    return 8;
  case VKL_WEDGE:
    return 6;
  case VKL_PYRAMID:
    return 5;
  }

  // Unknown cell type
  return 1;
}

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    UnstructuredVolume<W>::~UnstructuredVolume()
    {
    }

    template <int W>
    void UnstructuredVolume<W>::commit()
    {
      Volume<W>::commit();

      // hex method planar/nonplanar

      vertexPosition = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "vertex.position", nullptr);
      vertexValue = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "vertex.value", nullptr);

      index = (Data *)this->template getParam<ManagedObject::VKL_PTR>("index",
                                                                      nullptr);
      indexPrefixed = this->template getParam<bool>("indexPrefixed", false);

      cellIndex = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "cell.index", nullptr);
      cellValue = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "cell.value", nullptr);
      cellType = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "cell.type", nullptr);

      if (!vertexPosition) {
        throw std::runtime_error("unstructured volume must have 'vertex.position'");
      }
      if (!index) {
        throw std::runtime_error("unstructured volume must have 'index'");
      }
      if (!vertexValue && !cellValue) {
        throw std::runtime_error(
            "unstructured volume must have 'vertex.value' or 'cell.value'");
      }
      if ((!indexPrefixed && !cellType) || (indexPrefixed && cellType)) {
        throw std::runtime_error(
            "unstructured volume must have one of 'cell.type' or "
            "'indexPrefixed'");
      }

      if (vertexPosition->dataType != VKL_FLOAT3) {
        throw std::runtime_error("unstructured volume unsupported vertex type");
      }

      switch (index->dataType) {
      case VKL_UINT:
        index32Bit = true;
        break;
      case VKL_ULONG:
        index32Bit = false;
        break;
      default:
        throw std::runtime_error("unstructured volume unsupported index type");
      }

      switch (cellIndex->dataType) {
      case VKL_UINT:
        cell32Bit = true;
        break;
      case VKL_ULONG:
        cell32Bit = false;
        break;
      default:
        throw std::runtime_error("unstructured volume unsupported cell type");
      }
      nCells = cellIndex->size();

      if (cellType) {
        if (nCells != cellType->size())
          throw std::runtime_error(
              "unstructured volume #cells does not match #cell.type");
      } else {
        cellType = new Data(nCells, VKL_UCHAR, nullptr, VKL_DATA_DEFAULT);
        uint8_t *typeArray = (uint8_t *)cellType->data;
        for (int i = 0; i < nCells; i++) {
          auto index = readInteger(cellIndex->data, cell32Bit, i);
          switch (getVertexId(index)) {
          case 4:
            typeArray[i] = VKL_TETRAHEDRON;
            break;
          case 8:
            typeArray[i] = VKL_HEXAHEDRON;
            break;
          case 6:
            typeArray[i] = VKL_WEDGE;
            break;
          case 5:
            typeArray[i] = VKL_PYRAMID;
            break;
          default:
            throw std::runtime_error(
                "unstructured volume unsupported cell vertex count");
            break;
          }
        }
      }

      auto precompute =
          this->template getParam<bool>("precomputedNormals", false);
      if (precompute) {
        if (faceNormals.empty()) {
          calculateFaceNormals();
        }
      } else {
        if (!faceNormals.empty()) {
          faceNormals.clear();
          faceNormals.shrink_to_fit();
        }
      }

      auto hexIterative = this->template getParam<bool>("hexIterative", false);

      buildBvhAndCalculateBounds();

      if (!ispcEquivalent) {
        ispcEquivalent = ispc::VKLUnstructuredVolume_Constructor();
      }

      ispc::VKLUnstructuredVolume_set(
          ispcEquivalent,
          (const ispc::box3f &)bounds,
          (const ispc::vec3f *)vertexPosition->data,
          (const uint32_t *)index->data,
          index32Bit,
          vertexValue ? (const float *)vertexValue->data : nullptr,
          cellValue ? (const float *)cellValue->data : nullptr,
          (const uint32_t *)cellIndex->data,
          cell32Bit,
          indexPrefixed,
          (const uint8_t *)cellType->data,
          bvh.rootRef(),
          bvh.nodePtr(),
          bvh.itemListPtr(),
          faceNormals.empty() ? nullptr : (const ispc::vec3f *)faceNormals.data(),
          hexIterative);
    }

    template <int W>
    box4f UnstructuredVolume<W>::getCellBBox(size_t id)
    {
      // get cell offset in the vertex indices array
      uint64_t cOffset = getCellOffset(id);

      // iterate through cell vertices
      box4f bBox;
      uint32_t maxIdx = getVerticesCount(((uint8_t *)(cellType->data))[id]);
      for (uint32_t i = 0; i < maxIdx; i++) {
        // get vertex index
        uint64_t vId = getVertexId(cOffset + i);

        // build 4 dimensional vertex with its position and value
        vec3f &v  = ((vec3f *)(vertexPosition->data))[vId];
        float val = cellValue ? ((float *)(cellValue->data))[id]
                              : ((float *)(vertexValue->data))[vId];
        vec4f p = vec4f(v.x, v.y, v.z, val);

        // extend bounding box
        if (i == 0)
          bBox.upper = bBox.lower = p;
        else
          bBox.extend(p);
      }

      return bBox;
    }

    template <int W>
    void UnstructuredVolume<W>::buildBvhAndCalculateBounds()
    {
      std::vector<int64> primID(nCells);
      std::vector<box4f> primBounds(nCells);

      box4f bounds4 = empty;

      for (uint64_t i = 0; i < nCells; i++) {
        primID[i]       = i;
        auto cellBounds = getCellBBox(i);
        if (i == 0) {
          bounds4 = cellBounds;
        } else {
          bounds4.extend(cellBounds);
        }
        primBounds[i] = cellBounds;
      }

      bounds.lower = vec3f(bounds4.lower.x, bounds4.lower.y, bounds4.lower.z);
      bounds.upper = vec3f(bounds4.upper.x, bounds4.upper.y, bounds4.upper.z);

      bvh.build(primBounds.data(), primID.data(), nCells);
    }

    template <int W>
    void UnstructuredVolume<W>::calculateFaceNormals()
    {
      // Allocate memory for normal vectors
      uint64_t numNormals = nCells * 6;
      faceNormals.resize(numNormals);

      // Define vertices order for normal calculation
      const uint32_t tetrahedronFaces[4][3] = {
          {2, 0, 1}, {3, 1, 0}, {3, 2, 1}, {2, 3, 0}};
      const uint32_t hexahedronFaces[6][3] = {
          {3, 0, 1}, {5, 1, 0}, {6, 2, 1}, {7, 3, 2}, {7, 4, 0}, {6, 5, 4}};
      const uint32_t wedgeFaces[5][3] = {
          {2, 0, 1}, {4, 1, 0}, {5, 2, 1}, {5, 3, 0}, {5, 4, 3}};
      const uint32_t pyramidFaces[5][3] = {
          {3, 0, 1}, {4, 1, 0}, {4, 2, 1}, {4, 3, 2}, {3, 4, 0}};

      // Build all normals
      uint8_t *typeArray = (uint8_t *)cellType->data;
      tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
        switch (typeArray[taskIndex]) {
        case VKL_TETRAHEDRON:
          calculateCellNormals(taskIndex, tetrahedronFaces, 4);
          break;
        case VKL_HEXAHEDRON:
          calculateCellNormals(taskIndex, hexahedronFaces, 6);
          break;
        case VKL_WEDGE:
          calculateCellNormals(taskIndex, wedgeFaces, 5);
          break;
        case VKL_PYRAMID:
          calculateCellNormals(taskIndex, pyramidFaces, 5);
          break;
        }
      });
    }

    // Calculate all normals for arbitrary polyhedron
    // based on given vertices order
    template <int W>
    void UnstructuredVolume<W>::calculateCellNormals(const uint64_t cellId,
                                                     const uint32_t faces[6][3],
                                                     const uint32_t facesCount)
    {
      // Get cell offset in the vertex indices array
      uint64_t cOffset = getCellOffset(cellId);

      // Iterate through all faces
      for (uint32_t i = 0; i < facesCount; i++) {
        // Retrieve vertex positions
        uint64_t vId0   = getVertexId(cOffset + faces[i][0]);
        uint64_t vId1   = getVertexId(cOffset + faces[i][1]);
        uint64_t vId2   = getVertexId(cOffset + faces[i][2]);
        const vec3f &v0 = ((const vec3f *)(vertexPosition->data))[vId0];
        const vec3f &v1 = ((const vec3f *)(vertexPosition->data))[vId1];
        const vec3f &v2 = ((const vec3f *)(vertexPosition->data))[vId2];

        // Calculate normal
        faceNormals[cellId * 6 + i] = normalize(cross(v0 - v1, v2 - v1));
      }
    }

    VKL_REGISTER_VOLUME(UnstructuredVolume<4>, unstructured_4)
    VKL_REGISTER_VOLUME(UnstructuredVolume<8>, unstructured_8)
    VKL_REGISTER_VOLUME(UnstructuredVolume<16>, unstructured_16)

  }  // namespace ispc_driver
}  // namespace openvkl
