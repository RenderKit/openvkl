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

#include "UnstructuredVolume.h"
#include "../common/Data.h"
#include "ospcommon/containers/AlignedVector.h"
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

    static void tabIndent(int indent)
    {
      for (int i = 0; i < indent; i++)
        std::cerr << "\t";
    }

    static void dumpBVH(Node *root, int indent = 0)
    {
      if (root->nominalLength < 0) {
        auto leaf = (LeafNode *)root;
        tabIndent(indent);
        std::cerr << "id: " << leaf->cellID << " bounds: " << leaf->bounds
                  << " range: " << leaf->valueRange
                  << " nom: " << leaf->nominalLength << std::endl;
      } else {
        auto inner = (InnerNode *)root;
        tabIndent(indent);
        std::cerr << "range: " << inner->valueRange
                  << " nom: " << inner->nominalLength << std::endl;

        tabIndent(indent);
        std::cerr << "bounds[0]: " << inner->bounds[0] << std::endl;
        dumpBVH(inner->children[0], indent + 1);

        tabIndent(indent);
        std::cerr << "bounds[1]: " << inner->bounds[1] << std::endl;
        dumpBVH(inner->children[1], indent + 1);
      }
    }

    template <int W>
    UnstructuredVolume<W>::~UnstructuredVolume()
    {
      if (rtcBVH)
        rtcReleaseBVH(rtcBVH);
      if (rtcDevice)
        rtcReleaseDevice(rtcDevice);
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
        throw std::runtime_error(
            "unstructured volume must have 'vertex.position'");
      }
      if (!index) {
        throw std::runtime_error("unstructured volume must have 'index'");
      }
      if (!cellIndex) {
        throw std::runtime_error("unstructured volume must have 'cellIndex'");
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

      if (vertexPosition->dataType != VKL_VEC3F) {
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

      hexIterative = this->template getParam<bool>("hexIterative", false);

      bool needTolerances = false;
      for (int i = 0; i < nCells; i++) {
        auto cell = ((uint8_t *)cellType->data)[i];
        if (cell == VKL_WEDGE || cell == VKL_PYRAMID ||
            (cell == VKL_HEXAHEDRON && hexIterative)) {
          needTolerances = true;
          break;
        }
      }

      if (needTolerances)
        calculateIterativeTolerance();

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

      buildBvhAndCalculateBounds();

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = ispc::VKLUnstructuredVolume_Constructor();
      }

      ispc::VKLUnstructuredVolume_set(
          this->ispcEquivalent,
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
          (void *)(rtcRoot),
          faceNormals.empty() ? nullptr
                              : (const ispc::vec3f *)faceNormals.data(),
          iterativeTolerance.empty() ? nullptr : iterativeTolerance.data(),
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

    void errorFunction(void *userPtr, enum RTCError error, const char *str)
    {
      LogMessageStream(VKL_LOG_WARNING)
          << "error " << error << ": " << str << std::endl;
    }

    template <int W>
    void UnstructuredVolume<W>::buildBvhAndCalculateBounds()
    {
      rtcDevice = rtcNewDevice(NULL);
      if (!rtcDevice) {
        throw std::runtime_error("cannot create device");
      }
      rtcSetDeviceErrorFunction(rtcDevice, errorFunction, NULL);

      containers::AlignedVector<RTCBuildPrimitive> prims;
      containers::AlignedVector<range1f> range;
      prims.resize(nCells);
      range.resize(nCells);

      tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
        box4f bound              = getCellBBox(taskIndex);
        prims[taskIndex].lower_x = bound.lower.x;
        prims[taskIndex].lower_y = bound.lower.y;
        prims[taskIndex].lower_z = bound.lower.z;
        prims[taskIndex].geomID  = taskIndex >> 32;
        prims[taskIndex].upper_x = bound.upper.x;
        prims[taskIndex].upper_y = bound.upper.y;
        prims[taskIndex].upper_z = bound.upper.z;
        prims[taskIndex].primID  = taskIndex & 0xffffffff;
        range[taskIndex]         = range1f(bound.lower.w, bound.upper.w);
      });

      rtcBVH = rtcNewBVH(rtcDevice);
      if (!rtcBVH) {
        throw std::runtime_error("bvh creation failure");
      }

      RTCBuildArguments arguments      = rtcDefaultBuildArguments();
      arguments.byteSize               = sizeof(arguments);
      arguments.buildFlags             = RTC_BUILD_FLAG_NONE;
      arguments.buildQuality           = RTC_BUILD_QUALITY_MEDIUM;
      arguments.maxBranchingFactor     = 2;
      arguments.maxDepth               = 1024;
      arguments.sahBlockSize           = 1;
      arguments.minLeafSize            = 1;
      arguments.maxLeafSize            = 1;
      arguments.traversalCost          = 1.0f;
      arguments.intersectionCost       = 10.0f;
      arguments.bvh                    = rtcBVH;
      arguments.primitives             = prims.data();
      arguments.primitiveCount         = prims.size();
      arguments.primitiveArrayCapacity = prims.size();
      arguments.createNode             = InnerNode::create;
      arguments.setNodeChildren        = InnerNode::setChildren;
      arguments.setNodeBounds          = InnerNode::setBounds;
      arguments.createLeaf             = LeafNode::create;
      arguments.splitPrimitive         = nullptr;
      arguments.buildProgress          = nullptr;
      arguments.userPtr                = range.data();

      rtcRoot = (Node *)rtcBuildBVH(&arguments);
      if (!rtcRoot) {
        throw std::runtime_error("bvh build failure");
      }

      if (rtcRoot->nominalLength < 0) {
        auto &val = ((LeafNode *)rtcRoot)->bounds;
        bounds    = box3f(val.lower, val.upper);
      } else {
        auto &vals = ((InnerNode *)rtcRoot)->bounds;
        bounds     = box3f(vals[0].lower, vals[0].upper);
        bounds.extend(box3f(vals[1].lower, vals[1].upper));
      }
      valueRange = rtcRoot->valueRange;
    }

    template <int W>
    void UnstructuredVolume<W>::calculateIterativeTolerance()
    {
      iterativeTolerance.resize(nCells);
      const uint32_t wedgeEdges[9][2]   = {{0, 1},
                                         {1, 2},
                                         {2, 0},
                                         {3, 4},
                                         {4, 5},
                                         {5, 3},
                                         {0, 3},
                                         {1, 4},
                                         {2, 5}};
      const uint32_t pyramidEdges[8][2] = {
          {0, 1}, {1, 2}, {2, 3}, {3, 0}, {0, 4}, {1, 4}, {2, 4}, {3, 4}};
      const uint32_t hexDiagonals[4][2] = {{0, 6}, {1, 7}, {2, 4}, {3, 5}};

      // Build all tolerances
      uint8_t *typeArray = (uint8_t *)cellType->data;
      tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
        switch (typeArray[taskIndex]) {
        case VKL_HEXAHEDRON:
          if (!hexIterative)
            calculateTolerance(taskIndex, hexDiagonals, 4);
          break;
        case VKL_WEDGE:
          calculateTolerance(taskIndex, wedgeEdges, 9);
          break;
        case VKL_PYRAMID:
          calculateTolerance(taskIndex, pyramidEdges, 8);
          break;
        default:
          break;
        }
      });
    }

    template <int W>
    void UnstructuredVolume<W>::calculateTolerance(const uint64_t cellId,
                                                   const uint32_t edge[][2],
                                                   const uint32_t count)
    {
      uint64_t cOffset = getCellOffset(cellId);

      float longest = 0;
      for (int i = 0; i < count; i++) {
        uint64_t vId0    = getVertexId(cOffset + edge[i][0]);
        uint64_t vId1    = getVertexId(cOffset + edge[i][1]);
        const vec3f &v0  = ((const vec3f *)(vertexPosition->data))[vId0];
        const vec3f &v1  = ((const vec3f *)(vertexPosition->data))[vId1];
        const float dist = length(v0 - v1);
        longest          = std::max(longest, dist);
      }

      const float volumeBound = longest * longest * longest;
      const float determinantTolerance =
          1e-20 < .00001 * volumeBound ? 1e-20 : .00001 * volumeBound;

      iterativeTolerance[cellId] = determinantTolerance;
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
