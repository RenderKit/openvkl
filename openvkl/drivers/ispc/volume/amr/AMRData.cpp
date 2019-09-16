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

// amr base
#include "AMRData.h"

#include <iostream>

namespace openvkl {
  namespace ispc_driver {
    namespace amr {

      /*! initialize an internal brick representation from input
          brickinfo and corresponding input data pointer */
      AMRData::Brick::Brick(const BrickInfo &info, const float *data)
      {
        this->box       = info.box;
        this->level     = info.level;
        this->cellWidth = info.cellWidth;

        this->value            = data;
        this->dims             = this->box.size() + vec3i(1);
        this->gridToWorldScale = 1.f / this->cellWidth;
        this->worldBounds =
            box3f(vec3f(this->box.lower) * this->cellWidth,
                  vec3f(this->box.upper + vec3i(1)) * this->cellWidth);
        this->worldToGridScale = rcp(this->worldBounds.size());
        this->f_dims           = vec3f(this->dims);
      }

      /*! this structure defines only the format of the INPUT of amr
        data - ie, what we get from the scene graph or applicatoin */
      AMRData::AMRData(const Data &blockBoundsData,
                       const Data &refinementLevelsData,
                       const Data &cellWidthsData,
                       const Data &blockDataData)
      {
        size_t numBricks;
        if (blockBoundsData.dataType == VKL_BOX3I) {
          numBricks = blockBoundsData.numItems;
        } else {  // assuming VKL_INT
          numBricks = blockBoundsData.numItems / 6;
        }
        // ALOK: putting the arrays back into a struct for now
        // TODO: turn this all into arrays
        const box3i *blockBounds    = (const box3i *)blockBoundsData.data;
        postLogMessage(VKL_LOG_DEBUG) << "bounds[0] " << blockBounds[0];
        const int *refinementLevels = (const int *)refinementLevelsData.data;
        postLogMessage(VKL_LOG_DEBUG) << "levels[0] " << refinementLevels[0];
        const float *cellWidths     = (const float *)cellWidthsData.data;
        postLogMessage(VKL_LOG_DEBUG) << "widths[0] " << cellWidths[0];

        const Data **allBlocksData = (const Data **)blockDataData.data;
        for (size_t i = 0; i < numBricks; i++) {
          AMRData::BrickInfo blockInfo;
          blockInfo.box       = blockBounds[i];
          blockInfo.level     = refinementLevels[i];
          blockInfo.cellWidth = cellWidths[refinementLevels[i]];
          brick.emplace_back(blockInfo, (const float *)allBlocksData[i]->data);
        }
      }

    }  // namespace amr
  }    // namespace ispc_driver
}  // namespace openvkl

