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

#include "SharedStructuredVolume.h"
#include <ospray/ospcommon/tasking/parallel_for.h>
#include "GridAccelerator_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    SharedStructuredVolume<W>::~SharedStructuredVolume()
    {
      if (ispcEquivalent) {
        ispc::SharedStructuredVolume_Destructor(ispcEquivalent);
      }
    }

    template <int W>
    void SharedStructuredVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      if (!ispcEquivalent) {
        ispcEquivalent = ispc::SharedStructuredVolume_Constructor();
      }

      voxelData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "voxelData", nullptr);

      if (!voxelData) {
        throw std::runtime_error("no voxelData set on volume");
      }

      if (voxelData->dataType != VKL_FLOAT) {
        throw std::runtime_error(
            "SharedStructuredVolume currently only supports VKL_FLOAT volumes");
      }

      if (voxelData->size() != this->dimensions.product()) {
        throw std::runtime_error(
            "incorrect voxelData size for provided volume dimensions");
      }

      ispc::SharedStructuredVolume_set(ispcEquivalent,
                                       (float *)voxelData->data,
                                       (const ispc::vec3i &)this->dimensions,
                                       (const ispc::vec3f &)this->gridOrigin,
                                       (const ispc::vec3f &)this->gridSpacing);

      buildAccelerator();
    }

    template <int W>
    void SharedStructuredVolume<W>::buildAccelerator()
    {
      void *accelerator =
          ispc::SharedStructuredVolume_createAccelerator(ispcEquivalent);

      vec3i bricksPerDimension;
      bricksPerDimension.x =
          ispc::GridAccelerator_getBricksPerDimension_x(accelerator);
      bricksPerDimension.y =
          ispc::GridAccelerator_getBricksPerDimension_y(accelerator);
      bricksPerDimension.z =
          ispc::GridAccelerator_getBricksPerDimension_z(accelerator);

      const int numTasks =
          bricksPerDimension.x * bricksPerDimension.y * bricksPerDimension.z;
      tasking::parallel_for(numTasks, [&](int taskIndex) {
        ispc::GridAccelerator_build(accelerator, taskIndex);
      });
    }

    VKL_REGISTER_VOLUME(SharedStructuredVolume<4>, shared_structured_volume_4)
    VKL_REGISTER_VOLUME(SharedStructuredVolume<8>, shared_structured_volume_8)
    VKL_REGISTER_VOLUME(SharedStructuredVolume<16>, shared_structured_volume_16)

  }  // namespace ispc_driver
}  // namespace openvkl
