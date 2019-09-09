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

    AMRVolume::AMRVolume()
    {
      ispcEquivalent = ispc::AMRVolume_create(this);
    }

    std::string AMRVolume::toString() const
    {
      return "ospray::AMRVolume";
    }

    //! Allocate storage and populate the volume.
    void AMRVolume::commit()
    {
      Volume::commit();

      voxelRange = getParam<vec2f>("voxelRange", vec2f(FLT_MAX, -FLT_MAX));

      amrMethod = getParam<VKLAMRMethod>("method", VKL_AMR_CURRENT);

      if (amrMethod == VKL_AMR_CURRENT)
        ispc::AMR_install_current(this->ispcEquivalent);
      else if (amrMethod == VKL_AMR_FINEST)
        ispc::AMR_install_finest(this->ispcEquivalent);
      else if (amrMethod == VKL_AMR_OCTANT)
        ispc::AMR_install_octant(this->ispcEquivalent);

      if (data != nullptr)  // TODO: support data updates
        return;

      blockBoundsData = (Data *)getParam<ManagedObject::VKL_PTR>("block.bounds", nullptr);
      if (blockBoundsData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.bounds' array");

      refinementLevelsData = (Data *)getParam<ManagedObject::VKL_PTR>("block.level", nullptr);
      if (refinementLevelsData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.level' array");

      cellWidthsData = (Data *)getParam<ManagedObject::VKL_PTR>("block.cellWidth", nullptr);
      if (cellWidthsData.ptr == nullptr)
        throw std::runtime_error(
            "amr volume must have 'block.cellWidth' array");

      blockDataData = (Data *)getParam<ManagedObject::VKL_PTR>("block.data", nullptr);
      if (blockDataData.ptr == nullptr)
        throw std::runtime_error("amr volume must have 'block.data' array");

      data  = make_unique<amr::AMRData>(*blockBoundsData,
                                       *refinementLevelsData,
                                       *cellWidthsData,
                                       *blockDataData);
      accel = make_unique<amr::AMRAccel>(*data);

      float coarsestCellWidth = *std::max_element(
          cellWidthsData->begin<float>(), cellWidthsData->end<float>());

      float samplingStep = 0.1f * coarsestCellWidth;

      bounds = accel->worldBounds;

      const vec3f gridSpacing = getParam<vec3f>("gridSpacing", vec3f(1.f));
      const vec3f gridOrigin  = getParam<vec3f>("gridOrigin", vec3f(0.f));

      voxelType = (VKLDataType)getParam<int>("voxelType", VKL_UNKNOWN);

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
        throw std::runtime_error("amr volume 'voxelType' is invalid type " +
                                 stringForType(voxelType) +
                                 ". Must be one of: VKL_UCHAR, VKL_SHORT, "
                                 "VKL_USHORT, VKL_FLOAT, VKL_DOUBLE");
      }

      ispc::AMRVolume_set(ispcEquivalent,
                          (ispc::box3f &)bounds,
                          samplingStep,
                          (const ispc::vec3f &)gridOrigin,
                          (const ispc::vec3f &)gridSpacing);

      ispc::AMRVolume_setAMR(ispcEquivalent,
                             accel->node.size(),
                             &accel->node[0],
                             accel->leaf.size(),
                             &accel->leaf[0],
                             accel->level.size(),
                             &accel->level[0],
                             voxelType,
                             (ispc::box3f &)bounds);

      tasking::parallel_for(accel->leaf.size(), [&](size_t leafID) {
        ispc::AMRVolume_computeValueRangeOfLeaf(ispcEquivalent, leafID);
      });
    }

    VKL_REGISTER_VOLUME(AMRVolume, AMRVolume);
    VKL_REGISTER_VOLUME(AMRVolume, amr_volume);

  }  // namespace ispc_driver
}  // namespace openvkl
