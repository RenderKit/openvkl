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

#pragma once

#include "AMRAccel.h"
#include "../StructuredRegularVolume.h"
#include "ospcommon/memory/RefCount.h"

using namespace ospcommon::memory;

typedef enum
{
  VKL_AMR_CURRENT,
  VKL_AMR_FINEST,
  VKL_AMR_OCTANT
} VKLAMRMethod;

namespace openvkl {
  namespace ispc_driver {

    struct AMRVolume : public StructuredRegularVolume<4>
    {
      AMRVolume();
      ~AMRVolume() override = default;

      std::string toString() const override;

      void commit() override;

      std::unique_ptr<amr::AMRData> data;
      std::unique_ptr<amr::AMRAccel> accel;

      Ref<Data> blockDataData;
      Ref<Data> blockBoundsData;
      Ref<Data> refinementLevelsData;
      Ref<Data> cellWidthsData;

      VKLDataType voxelType;

      //! Voxel value range (will be computed if not provided as a parameter).
      vec2f voxelRange;
      box3f bounds;

      VKLAMRMethod amrMethod;

      /*! Scale factor for the volume, mostly for internal use or data scaling
        benchmarking. Note that this must be set **before** calling
        'ospSetRegion' on the volume as the scaling is applied in that function.
      */
      vec3f scaleFactor{1};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
