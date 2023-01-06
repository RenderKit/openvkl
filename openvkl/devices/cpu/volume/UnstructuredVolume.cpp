// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "UnstructuredVolume.h"
#include <algorithm>
#include "../common/Data.h"
#include "UnstructuredSampler.h"
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/tasking/parallel_for.h"

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
  namespace cpu_device {

    static void tabIndent(int indent)
    {
      for (int i = 0; i < indent; i++)
        std::cerr << "\t";
    }

    static void dumpBVH(Node *root, int indent = 0)
    {
      if (root->nominalLength.x < 0) {
        auto leaf = (LeafNodeSingle *)root;
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
      if (this->SharedStructInitialized) {
        this->SharedStructInitialized = false;
      }

      if (rtcBVH)
        rtcReleaseBVH(rtcBVH);
      if (rtcDevice)
        rtcReleaseDevice(rtcDevice);
    }

    template <int W>
    void UnstructuredVolume<W>::commit()
    {
      UnstructuredVolumeBase<W>::commit();

      // hex method planar/nonplanar

      vertexPosition = this->template getParamDataT<vec3f>("vertex.position");
      vertexValue = this->template getParamDataT<float>("vertex.data", nullptr);
      indexPrefixed = this->template getParam<bool>("indexPrefixed", false);
      cellValue     = this->template getParamDataT<float>("cell.data", nullptr);
      cellType = this->template getParamDataT<uint8_t>("cell.type", nullptr);

      background = this->template getParamDataT<float>(
          "background", 1, VKL_BACKGROUND_UNDEFINED);

      if (!vertexValue && !cellValue) {
        throw std::runtime_error(
            "unstructured volume must have 'vertex.data' or 'cell.data'");
      }
      if ((!indexPrefixed && !cellType) || (indexPrefixed && cellType)) {
        throw std::runtime_error(
            "unstructured volume must have one of 'cell.type' or "
            "'indexPrefixed'");
      }

      // index data can be 32 or 64 bit
      Ref<const Data> index = this->template getParam<Data *>("index");

      switch (index->dataType) {
      case VKL_UINT:
        index32Bit = true;
        index32    = this->template getParamDataT<uint32_t>("index");
        break;
      case VKL_ULONG:
        index32Bit = false;
        index64    = this->template getParamDataT<uint64_t>("index");
        break;
      default:
        throw std::runtime_error("unstructured volume unsupported index type");
      }

      // cell.index data can be 32 or 64 bit
      Ref<const Data> cellIndex = this->template getParam<Data *>("cell.index");

      switch (cellIndex->dataType) {
      case VKL_UINT:
        cell32Bit   = true;
        cellIndex32 = this->template getParamDataT<uint32_t>("cell.index");
        break;
      case VKL_ULONG:
        cell32Bit   = false;
        cellIndex64 = this->template getParamDataT<uint64_t>("cell.index");
        break;
      default:
        throw std::runtime_error(
            "unstructured volume unsupported cell.index type");
      }
      nCells = cellIndex->size();

      if (cellType) {
        if (nCells != cellType->size())
          throw std::runtime_error(
              "unstructured volume #cells does not match #cell.type");
      } else {
        generatedCellType =
            make_buffer_shared_unique<uint8_t>(this->getDevice(), nCells);

        for (int i = 0; i < nCells; i++) {
          auto index = cell32Bit ? (*cellIndex32)[i] : (*cellIndex64)[i];
          switch (getVertexId(index)) {
          case 4:
            generatedCellType->sharedPtr()[i] = VKL_TETRAHEDRON;
            break;
          case 8:
            generatedCellType->sharedPtr()[i] = VKL_HEXAHEDRON;
            break;
          case 6:
            generatedCellType->sharedPtr()[i] = VKL_WEDGE;
            break;
          case 5:
            generatedCellType->sharedPtr()[i] = VKL_PYRAMID;
            break;
          default:
            throw std::runtime_error(
                "unstructured volume unsupported cell vertex count");
            break;
          }
        }

        Data *d  = new Data(this->getDevice(),
                           nCells,
                           VKL_UCHAR,
                           generatedCellType->sharedPtr(),
                           VKL_DATA_SHARED_BUFFER,
                           0);
        cellType = &(d->as<uint8_t>());
        d->refDec();
      }

      hexIterative = this->template getParam<bool>("hexIterative", false);

      bool needTolerances = false;
      for (int i = 0; i < nCells; i++) {
        auto cell = (*cellType)[i];
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
        if (!faceNormals || faceNormals->size() == 0) {
          calculateFaceNormals();
        }
      } else {
        if (faceNormals && faceNormals->size() != 0) {
          faceNormals = make_buffer_shared_unique<vec3f>(this->getDevice(), 0);
        }
      }

      buildBvhAndCalculateBounds();

      computeOverlappingNodeMetadata(rtcRoot);

      if (!this->SharedStructInitialized) {
        ispc::VKLUnstructuredVolume *self =
            static_cast<ispc::VKLUnstructuredVolume *>(this->getSh());
        memset(self, 0, sizeof(ispc::VKLUnstructuredVolume));
        self->super.super.type =
            ispc::DeviceVolumeType::VOLUME_TYPE_UNSTRUCTURED;
        this->SharedStructInitialized = true;
      }

      this->setBackground(background->data());

      CALL_ISPC(VKLUnstructuredVolume_set,
                this->getSh(),
                (const ispc::box3f &)bounds,
                ispc(vertexPosition),
                index32Bit ? ispc(index32) : ispc(index64),
                index32Bit,
                ispc(vertexValue),
                ispc(cellValue),
                cell32Bit ? ispc(cellIndex32) : ispc(cellIndex64),
                cell32Bit,
                indexPrefixed,
                ispc(cellType),
                (void *)(rtcRoot),
                (!faceNormals || faceNormals->size() == 0)
                    ? nullptr
                    : (const ispc::vec3f *)faceNormals->sharedPtr(),
                (!iterativeTolerance || iterativeTolerance->size() == 0)
                    ? nullptr
                    : iterativeTolerance->sharedPtr(),
                hexIterative);
    }

    template <int W>
    Sampler<W> *UnstructuredVolume<W>::newSampler()
    {
      return new UnstructuredSampler<W>(this->getDevice(), *this);
    }

    template <int W>
    box4f UnstructuredVolume<W>::getCellBBox(size_t id)
    {
      // get cell offset in the vertex indices array
      uint64_t cOffset = getCellOffset(id);

      // iterate through cell vertices
      box4f bBox;
      uint32_t maxIdx = getVerticesCount((*cellType)[id]);
      for (uint32_t i = 0; i < maxIdx; i++) {
        // get vertex index
        uint64_t vId = getVertexId(cOffset + i);

        // build 4 dimensional vertex with its position and value
        const vec3f &v = (*vertexPosition)[vId];
        float val      = cellValue ? (*cellValue)[id] : (*vertexValue)[vId];
        vec4f p        = vec4f(v.x, v.y, v.z, val);

        // extend bounding box
        if (i == 0)
          bBox.upper = bBox.lower = p;
        else
          bBox.extend(p);
      }

      return bBox;
    }

    static inline void errorFunction(void *userPtr,
                                     enum RTCError error,
                                     const char *str)
    {
      Device *device = reinterpret_cast<Device *>(userPtr);
      LogMessageStream(device, VKL_LOG_WARNING)
          << "error " << error << ": " << str << std::endl;
    }

    template <int W>
    void UnstructuredVolume<W>::buildBvhAndCalculateBounds()
    {
      rtcDevice = rtcNewDevice(NULL);
      if (!rtcDevice) {
        throw std::runtime_error("cannot create device");
      }
      rtcSetDeviceErrorFunction(rtcDevice, errorFunction, this->device.ptr);

      AlignedVector<RTCBuildPrimitive> prims;
      AlignedVector<range1f> range;

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

      // conservatively allocate all the space the BVH might need upfront
      bvhBuildAllocator = make_unique<BvhBuildAllocator>(
          this->getDevice(), sizeof(InnerNode) * (nCells * 2 - 1));

      userPtrStruct myUPS{range.data(), bvhBuildAllocator.get()};

      rtcBVH = rtcNewBVH(rtcDevice);
      if (!rtcBVH) {
        throw std::runtime_error("bvh creation failure");
      }

      RTCBuildArguments arguments      = rtcDefaultBuildArguments();
      arguments.byteSize               = sizeof(arguments);
      arguments.buildFlags             = RTC_BUILD_FLAG_NONE;
      arguments.buildQuality           = RTC_BUILD_QUALITY_LOW;
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
      arguments.createLeaf             = LeafNodeSingle::create;
      arguments.splitPrimitive         = nullptr;
      arguments.buildProgress          = nullptr;
      arguments.userPtr                = &myUPS;

      rtcRoot = (Node *)rtcBuildBVH(&arguments);
      if (!rtcRoot) {
        throw std::runtime_error("bvh build failure");
      }

      if (rtcRoot->nominalLength.x < 0) {
        auto &val = ((LeafNode *)rtcRoot)->bounds;
        bounds    = box3f(val.lower, val.upper);
      } else {
        auto &vals = ((InnerNode *)rtcRoot)->bounds;
        bounds     = box3f(vals[0].lower, vals[0].upper);
        bounds.extend(box3f(vals[1].lower, vals[1].upper));
      }
      valueRange = rtcRoot->valueRange;

      addLevelToNodes(rtcRoot, 0);

      bvhDepth = getMaxNodeLevel(rtcRoot);
    }

    template <int W>
    void UnstructuredVolume<W>::calculateIterativeTolerance()
    {
      iterativeTolerance =
          make_buffer_shared_unique<float>(this->getDevice(), nCells);
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
      tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
        switch ((*cellType)[taskIndex]) {
        case VKL_HEXAHEDRON:
          if (hexIterative)
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
        const vec3f &v0  = (*vertexPosition)[vId0];
        const vec3f &v1  = (*vertexPosition)[vId1];
        const float dist = length(v0 - v1);
        longest          = std::max(longest, dist);
      }

      const float volumeBound = longest * longest * longest;
      const float determinantTolerance =
          1e-20 < .00001 * volumeBound ? 1e-20 : .00001 * volumeBound;

      iterativeTolerance->sharedPtr()[cellId] = determinantTolerance;
    }

    template <int W>
    void UnstructuredVolume<W>::calculateFaceNormals()
    {
      // Allocate memory for normal vectors
      uint64_t numNormals = nCells * 6;
      faceNormals =
          make_buffer_shared_unique<vec3f>(this->getDevice(), numNormals);

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
      tasking::parallel_for(nCells, [&](uint64_t taskIndex) {
        switch ((*cellType)[taskIndex]) {
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
        const vec3f &v0 = (*vertexPosition)[vId0];
        const vec3f &v1 = (*vertexPosition)[vId1];
        const vec3f &v2 = (*vertexPosition)[vId2];

        // Calculate normal
        faceNormals->sharedPtr()[cellId * 6 + i] =
            normalize(cross(v0 - v1, v2 - v1));
      }
    }

    VKL_REGISTER_VOLUME(UnstructuredVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_unstructured_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
