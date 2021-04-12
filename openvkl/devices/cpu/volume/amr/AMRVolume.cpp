// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.h"
#include "../../common/export_util.h"
#include "../common/Data.h"
#include "AMRSampler.h"
// rkcommon
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/getEnvVar.h"
// ispc exports
#include "AMRVolume_ispc.h"
// stl
#include <map>
#include <set>

namespace openvkl {
  namespace cpu_device {

    template <int W>
    AMRVolume<W>::AMRVolume()
    {
      this->ispcEquivalent = CALL_ISPC(AMRVolume_create, this);
    }

    template <int W>
    AMRVolume<W>::~AMRVolume()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(AMRVolume_Destructor, this->ispcEquivalent);
      }
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

      if (data != nullptr)  // TODO: support data updates
        return;

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

      CALL_ISPC(AMRVolume_set,
                this->ispcEquivalent,
                (ispc::box3f &)bounds,
                samplingStep,
                (const ispc::vec3f &)gridOrigin,
                (const ispc::vec3f &)gridSpacing);

      CALL_ISPC(AMRVolume_setAMR,
                this->ispcEquivalent,
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
            AMRVolume_computeValueRangeOfLeaf, this->ispcEquivalent, leafID);
      });

      // compute value range over the full volume
      for (const auto &l : accel->leaf) {
        valueRange.extend(l.valueRange);
      }
    }

    template <int W>
    Sampler<W> *AMRVolume<W>::newSampler()
    {
      return new AMRSampler<W>(this);
    }

    template <int W>
    box3f AMRVolume<W>::getBoundingBox() const
    {
      return box3f(vec3f(origin+bounds.lower),
                   vec3f(origin+(bounds.upper-bounds.lower)*spacing));
    }

    template <int W>
    unsigned int AMRVolume<W>::getNumAttributes() const
    {
      return 1;
    }

    template <int W>
    range1f AMRVolume<W>::getValueRange() const
    {
      return valueRange;
    }

    template <int W>
    VKLAMRMethod AMRVolume<W>::getAMRMethod() const
    {
      return amrMethod;
    }

    VKL_REGISTER_VOLUME(AMRVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_amr_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
