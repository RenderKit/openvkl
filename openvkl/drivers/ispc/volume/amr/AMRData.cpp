// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
        this->value     = data;
        this->dims      = this->box.size() + vec3i(1);
        this->f_dims    = vec3f(this->dims);

        this->worldBounds =
            box3f(vec3f(this->box.lower) * this->cellWidth,
                  vec3f(this->box.upper + vec3i(1)) * this->cellWidth);
        this->gridToWorldScale = 1.f / this->cellWidth;
        this->worldToGridScale = rcp(this->worldBounds.size());
      }

      /*! this structure defines only the format of the INPUT of amr
        data - ie, what we get from the scene graph or applicatoin */
      AMRData::AMRData(const Data &blockBoundsData,
                       const Data &refinementLevelsData,
                       const Data &cellWidthsData,
                       const Data &blockDataData)
      {
        size_t numBricks = blockBoundsData.numItems;

        // ALOK: putting the arrays back into a struct for now
        // TODO: turn this all into arrays

        const box3i *blockBounds    = (const box3i *)blockBoundsData.data;
        const int *refinementLevels = (const int *)refinementLevelsData.data;
        const float *cellWidths     = (const float *)cellWidthsData.data;
        const Data **allBlocksData  = (const Data **)blockDataData.data;

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

