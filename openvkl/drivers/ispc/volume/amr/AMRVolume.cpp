// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "AMRVolume.h"
#include "../common/Data.h"
// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"
// ispc exports
#include "AMRVolume_ispc.h"
#include "method_current_ispc.h"
#include "method_finest_ispc.h"
#include "method_octant_ispc.h"
// stl
#include <map>
#include <set>

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    AMRVolume<W>::AMRVolume()
    {
      this->ispcEquivalent = ispc::AMRVolume_create(this);
    }

    template <int W>
    std::string AMRVolume<W>::toString() const
    {
      return "openvkl::AMRVolume";
    }

    template <int W>
    //! Allocate storage and populate the volume.
    void AMRVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      voxelRange = this->template getParam<vec2f>("voxelRange",
                                                  vec2f(FLT_MAX, -FLT_MAX));

      amrMethod =
          this->template getParam<VKLAMRMethod>("method", VKL_AMR_CURRENT);

      if (amrMethod == VKL_AMR_CURRENT)
        ispc::AMR_install_current(this->ispcEquivalent);
      else if (amrMethod == VKL_AMR_FINEST)
        ispc::AMR_install_finest(this->ispcEquivalent);
      else if (amrMethod == VKL_AMR_OCTANT)
        ispc::AMR_install_octant(this->ispcEquivalent);

      if (data != nullptr)  // TODO: support data updates
        return;

      blockBoundsData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "block.bounds", nullptr);
      if (blockBoundsData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.bounds' array");

      postLogMessage(VKL_LOG_DEBUG) << "commit(): bounds type: " << stringForType(blockBoundsData.ptr->dataType);
      postLogMessage(VKL_LOG_DEBUG) << "commit(): bounds num: " << blockBoundsData.ptr->numItems;

      refinementLevelsData =
          (Data *)this->template getParam<ManagedObject::VKL_PTR>("block.level",
                                                                  nullptr);
      if (refinementLevelsData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.level' array");


      cellWidthsData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "block.cellWidth", nullptr);
      if (cellWidthsData.ptr == nullptr)
        throw std::runtime_error(
            "amr volume must have 'block.cellWidth' array");

      blockDataData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "block.data", nullptr);
      if (blockDataData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.data' array");

      data = make_unique<amr::AMRData>(*blockBoundsData,
                                       *refinementLevelsData,
                                       *cellWidthsData,
                                       *blockDataData);

      accel = make_unique<amr::AMRAccel>(*data);

      std::cout << "AMRAccel:" << std::endl;
      std::cout << *accel << std::endl;

      float coarsestCellWidth = *std::max_element(
          cellWidthsData->begin<float>(), cellWidthsData->end<float>());

      float samplingStep = 0.1f * coarsestCellWidth;

      bounds = accel->worldBounds;

      const vec3f gridSpacing =
          this->template getParam<vec3f>("gridSpacing", vec3f(1.f));
      const vec3f gridOrigin =
          this->template getParam<vec3f>("gridOrigin", vec3f(0.f));

      voxelType =
          (VKLDataType)this->template getParam<int>("voxelType", VKL_UNKNOWN);

      switch (voxelType) {
      case VKL_UCHAR:
        break;
      case VKL_SHORT:
        break;
      case VKL_USHORT:
        break;
      case VKL_FLOAT:
        break;
      case VKL_DOUBLE:
        break;
      default:
        throw std::runtime_error(
            "amr volume 'voxelType' has invalid type."
            "Must be one of: VKL_UCHAR, VKL_SHORT, "
            "VKL_USHORT, VKL_FLOAT, VKL_DOUBLE");
      }

      ispc::AMRVolume_set(this->ispcEquivalent,
                          (ispc::box3f &)bounds,
                          samplingStep,
                          (const ispc::vec3f &)gridOrigin,
                          (const ispc::vec3f &)gridSpacing);

      ispc::AMRVolume_setAMR(this->ispcEquivalent,
                             accel->node.size(),
                             &accel->node[0],
                             accel->leaf.size(),
                             &accel->leaf[0],
                             accel->level.size(),
                             &accel->level[0],
                             voxelType,
                             (ispc::box3f &)bounds);

      postLogMessage(VKL_LOG_DEBUG) << "commit(): num leaves: " << accel->leaf.size();

      for (size_t leafID = 0; leafID < accel->leaf.size(); leafID++)
        ispc::AMRVolume_computeValueRangeOfLeaf(this->ispcEquivalent, leafID);
      /*
      tasking::parallel_for(accel->leaf.size(), [&](size_t leafID) {
        ispc::AMRVolume_computeValueRangeOfLeaf(this->ispcEquivalent, leafID);
      });
      */

      postLogMessage(VKL_LOG_DEBUG) << "commit(): got past the segfault!";
    }

    template <int W>
    void AMRVolume<W>::computeSampleV(const int *valid,
                                      const vvec3fn<W> &objectCoordinates,
                                      vfloatn<W> &samples) const
    {
      postLogMessage(VKL_LOG_DEBUG) << "AMRVolume::computeSampleV";
      ispc::AMRVolume_sample_export(
          valid, ispcEquivalent, &objectCoordinates, &samples);
    }

    template <int W>
    vec3f AMRVolume<W>::computeGradient(const vec3f &objectCoordinates) const
    {
      THROW_NOT_IMPLEMENTED;
    }

    template <int W>
    box3f AMRVolume<W>::getBoundingBox() const
    {
      return bounds;
    }

    VKL_REGISTER_VOLUME(AMRVolume<4>, amr_volume_4);
    VKL_REGISTER_VOLUME(AMRVolume<8>, amr_volume_8);
    VKL_REGISTER_VOLUME(AMRVolume<16>, amr_volume_16);

  }  // namespace ispc_driver
}  // namespace openvkl
