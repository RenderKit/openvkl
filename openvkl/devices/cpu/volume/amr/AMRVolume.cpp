// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.h"
#include "../../common/export_util.h"
#include "../common/Data.h"
#include "AMRSampler.h"
// rkcommon
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/getEnvVar.h"
// ispc exports
#include "AMRVolume_ispc.h"
// stl
#include <map>
#include <set>

namespace openvkl {
  namespace cpu_device {

    struct AMRLeafNodeUserData
    {
      range1f range;
      float cellWidth;
      vec3f gridSpacing;
    };

    struct AMRLeafNode : public LeafNodeSingle
    {
      AMRLeafNode(unsigned id,
                  const box3fa &bounds,
                  const range1f &range,
                  const float cellWidth,
                  const vec3f &gridSpacing)
          : LeafNodeSingle(id, bounds, range)
      {
        // ISPC-side code assumes the same layout as LeafNodeSingle
        static_assert(sizeof(AMRLeafNode) == sizeof(LeafNodeSingle),
                      "AMRLeafNode incompatible with LeafNode");

        nominalLength.x = -cellWidth * gridSpacing.x;
        nominalLength.y = cellWidth * gridSpacing.y;
        nominalLength.z = cellWidth * gridSpacing.z;
      }

      static void *create(RTCThreadLocalAllocator alloc,
                          const RTCBuildPrimitive *prims,
                          size_t numPrims,
                          void *userPtr)
      {
        assert(numPrims == 1);

        auto id          = (uint64_t(prims->geomID) << 32) | prims->primID;
        auto range       = ((AMRLeafNodeUserData *)userPtr)[id].range;
        auto cellWidth   = ((AMRLeafNodeUserData *)userPtr)[id].cellWidth;
        auto gridSpacing = ((AMRLeafNodeUserData *)userPtr)[id].gridSpacing;

        void *ptr = rtcThreadLocalAlloc(alloc, sizeof(AMRLeafNode), 16);
        return (void *)new (ptr) AMRLeafNode(
            id, *(const box3fa *)prims, range, cellWidth, gridSpacing);
      }
    };

    template <int W>
    AMRVolume<W>::AMRVolume(Device *device)
        : AddStructShared<UnstructuredVolumeBase<W>, ispc::AMRVolume>(device)
    {
      ispc::AMRVolume *self = static_cast<ispc::AMRVolume *>(this->getSh());

      memset(self, 0, sizeof(ispc::AMRVolume));

      CALL_ISPC(AMRVolume_Constructor, self);
      self->super.super.type        = ispc::DeviceVolumeType::VOLUME_TYPE_AMR;
      this->SharedStructInitialized = true;
    }

    template <int W>
    AMRVolume<W>::~AMRVolume()
    {
      if (this->SharedStructInitialized) {
        CALL_ISPC(AMRVolume_Destructor, this->getSh());
        this->SharedStructInitialized = false;
      }

      if (rtcBVH)
        rtcReleaseBVH(rtcBVH);
      if (rtcDevice)
        rtcReleaseDevice(rtcDevice);
    }

    template <int W>
    std::string AMRVolume<W>::toString() const
    {
      return "openvkl::AMRVolume";
    }

    template <int W>
    void AMRVolume<W>::commit()
    {
      amrMethod =
          (VKLAMRMethod)this->template getParam<int>("method", VKL_AMR_CURRENT);

      background = this->template getParamDataT<float>(
          "background", 1, VKL_BACKGROUND_UNDEFINED);

      if (data != nullptr)  // TODO: support data updates
      {
        this->setBackground(background->data());
        return;
      }

      cellWidthsData  = this->template getParamDataT<float>("cellWidth");
      blockBoundsData = this->template getParamDataT<box3i>("block.bounds");
      refinementLevelsData = this->template getParamDataT<int>("block.level");
      blockDataData        = this->template getParamDataT<Data *>("block.data");

      // determine voxelType from set of block data; they must all be the same
      std::set<VKLDataType> blockDataTypes;

      for (const auto &d : *blockDataData)
        blockDataTypes.insert(d->dataType);

      if (blockDataTypes.size() != 1)
        throw std::runtime_error(
            "all block.data entries must have same VKLDataType");

      voxelType = *blockDataTypes.begin();

      if (voxelType != VKL_FLOAT) {
        throw std::runtime_error(
            "AMR volume 'block.data' entries have invalid VKLDataType. Must be "
            "VKL_FLOAT");
      }

      // create the AMR data structure. This creates the logical blocks, which
      // contain the actual data and block-level metadata, such as cell width
      // and refinement level
      data = make_unique<amr::AMRData>(*blockBoundsData,
                                       *refinementLevelsData,
                                       *cellWidthsData,
                                       *blockDataData);

      // create the AMR acceleration structure. This creates a k-d tree
      // representation of the blocks in the AMRData object. In short, blocks at
      // the highest refinement level (i.e. with the most detail) are leaf
      // nodes, and parents have progressively lower resolution
      accel = make_unique<amr::AMRAccel>(*data);

      float coarsestCellWidth =
          *std::max_element(cellWidthsData->begin(), cellWidthsData->end());

      float samplingStep = 0.1f * coarsestCellWidth;

      bounds = accel->worldBounds;

      const vec3f gridOrigin =
          this->template getParam<vec3f>("gridOrigin", vec3f(0.f));
      origin = gridOrigin;

      const vec3f gridSpacing =
          this->template getParam<vec3f>("gridSpacing", vec3f(1.f));
      spacing = gridSpacing;

      this->setBackground(background->data());

      CALL_ISPC(AMRVolume_set,
                this->getSh(),
                (ispc::box3f &)bounds,
                samplingStep,
                (const ispc::vec3f &)gridOrigin,
                (const ispc::vec3f &)gridSpacing);

      CALL_ISPC(AMRVolume_setAMR,
                this->getSh(),
                accel->node.size(),
                &accel->node[0],
                accel->leaf.size(),
                &accel->leaf[0],
                accel->level.size(),
                &accel->level[0],
                voxelType,
                (ispc::box3f &)bounds);

      // parse the k-d tree to compute the voxel range of each leaf node.
      // This enables empty space skipping within the hierarchical structure
      tasking::parallel_for(accel->leaf.size(), [&](size_t leafID) {
        CALL_ISPC(
            AMRVolume_computeValueRangeOfLeaf, this->getSh(), leafID);
      });

      // compute value range over the full volume
      for (const auto &l : accel->leaf) {
        valueRange.extend(l.valueRange);
      }

      // need to do this after value ranges are known
      buildBvh();

      CALL_ISPC(AMRVolume_setBvh, this->getSh(), (void *)(rtcRoot));
    }

    template <int W>
    Sampler<W> *AMRVolume<W>::newSampler()
    {
      return new AMRSampler<W>(this->getDevice(), *this);
    }

    template <int W>
    box3f AMRVolume<W>::getBoundingBox() const
    {
      return box3f(vec3f(origin + bounds.lower),
                   vec3f(origin + (bounds.upper - bounds.lower) * spacing));
    }

    template <int W>
    unsigned int AMRVolume<W>::getNumAttributes() const
    {
      return 1;
    }

    template <int W>
    range1f AMRVolume<W>::getValueRange(unsigned int attributeIndex) const
    {
      throwOnIllegalAttributeIndex(this, attributeIndex);
      return valueRange;
    }

    template <int W>
    VKLAMRMethod AMRVolume<W>::getAMRMethod() const
    {
      return amrMethod;
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
    void AMRVolume<W>::buildBvh()
    {
      auto &leaves           = accel->leaf;
      const size_t numLeaves = leaves.size();

      rtcDevice = rtcNewDevice(NULL);
      if (!rtcDevice) {
        throw std::runtime_error("cannot create device");
      }
      rtcSetDeviceErrorFunction(rtcDevice, errorFunction, this->device.ptr);

      containers::AlignedVector<RTCBuildPrimitive> prims;
      containers::AlignedVector<AMRLeafNodeUserData> userData;
      prims.resize(numLeaves);
      userData.resize(numLeaves);

      tasking::parallel_for(numLeaves, [&](size_t taskIndex) {
        const auto &leaf = leaves[taskIndex];

        // leaf bounds are in AMR-space; transform into object-space
        const box3f bounds = box3f(origin + leaf.bounds.lower * spacing,
                                   origin + leaf.bounds.upper * spacing);

        prims[taskIndex].lower_x        = bounds.lower.x;
        prims[taskIndex].lower_y        = bounds.lower.y;
        prims[taskIndex].lower_z        = bounds.lower.z;
        prims[taskIndex].geomID         = taskIndex >> 32;
        prims[taskIndex].upper_x        = bounds.upper.x;
        prims[taskIndex].upper_y        = bounds.upper.y;
        prims[taskIndex].upper_z        = bounds.upper.z;
        prims[taskIndex].primID         = taskIndex & 0xffffffff;
        userData[taskIndex].range       = leaf.valueRange;
        userData[taskIndex].cellWidth   = leaf.brickList[0]->cellWidth;
        userData[taskIndex].gridSpacing = spacing;
      });

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
      arguments.createLeaf             = AMRLeafNode::create;
      arguments.splitPrimitive         = nullptr;
      arguments.buildProgress          = nullptr;
      arguments.userPtr                = userData.data();

      rtcRoot = (Node *)rtcBuildBVH(&arguments);
      if (!rtcRoot) {
        throw std::runtime_error("bvh build failure");
      }

      addLevelToNodes(rtcRoot, 0);

      bvhDepth = getMaxNodeLevel(rtcRoot);

      computeOverlappingNodeMetadata(rtcRoot);
    }

    VKL_REGISTER_VOLUME(AMRVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_amr_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
