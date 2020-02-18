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

#include "StructuredRegularVolume.h"
#include "../common/export_util.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    void StructuredRegularVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = CALL_ISPC(SharedStructuredVolume_Constructor);

        if (!this->ispcEquivalent) {
          throw std::runtime_error(
              "could not create ISPC-side object for StructuredRegularVolume");
        }
      }

      bool success = CALL_ISPC(SharedStructuredVolume_set,
                               this->ispcEquivalent,
                               this->voxelData->data,
                               this->voxelData->dataType,
                               (const ispc::vec3i &)this->dimensions,
                               ispc::structured_regular,
                               (const ispc::vec3f &)this->gridOrigin,
                               (const ispc::vec3f &)this->gridSpacing);

      if (!success) {
        CALL_ISPC(SharedStructuredVolume_Destructor, this->ispcEquivalent);
        this->ispcEquivalent = nullptr;

        throw std::runtime_error("failed to commit StructuredRegularVolume");
      }

      // must be last
      this->buildAccelerator();
    }

    VKL_REGISTER_VOLUME(StructuredRegularVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_structured_regular_, VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl
