// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/math.h"
#include "rkcommon/math/box.h"

using namespace rkcommon;

namespace openvkl {
  namespace ispc_driver {
    namespace amr {

      /*! this structure defines only the format of the INPUT of amr
          data - ie, what we get from the scene graph or application */
      struct AMRData
      {
        AMRData(const DataT<box3i> &blockBoundsData,
                const DataT<int> &refinementLevelsData,
                const DataT<float> &cellWidthsData,
                const DataT<Data *> &blockDataData);

        /*! this is how an app _specifies_ a brick (or better, the array
          of bricks); the brick data is specified through a separate
          array of data buffers (one data buffer per brick) */
        struct BrickInfo
        {
          /*! bounding box of integer coordinates of cells. note that
            this EXCLUDES the width of the rightmost cell: ie, a 4^3
            box at root level pos (0,0,0) would have a _box_ of
            [(0,0,0)-(3,3,3)] (because 3,3,3 is the highest valid
            coordinate in this box!), while its bounds would be
            [(0,0,0)-(4,4,4)]. Make sure to NOT use box.size() for the
            grid dimensions, since this will always be one lower than
            the dims of the grid.... */
          box3i box;
          //! level this brick is at
          int level;
          // width of each cell in this level
          float cellWidth;

          inline box3f worldBounds() const
          {
            return box3f(vec3f(box.lower) * cellWidth,
                         vec3f(box.upper + vec3i(1)) * cellWidth);
          }
        };

        struct Brick : public BrickInfo
        {
          /*! actual constructor from a brick info and data pointer */
          /*! initialize from given data */
          Brick(const BrickInfo &info, const DataT<float> &data);

          /* world bounds, including entire cells, and including
             level-specific cell width. ie, at root level cell width of
             1, a 4^3 root brick at (0,0,0) would have bounds
             [(0,0,0)-(4,4,4)] (as opposed to the 'box' value, see
             above!) */
          box3f worldBounds;

          //! pointer to the actual data values stored in this brick
          const ispc::Data1D *value{nullptr};
          //! dimensions of this box's data
          vec3i dims;
          //! scale factor from grid space to world space (ie,1.f/cellWidth)
          float gridToWorldScale;
          //! rcp(bounds.upper-bounds.lower);
          vec3f worldToGridScale;
          //! dimensions, in float
          vec3f f_dims;
        };

        //! our own, internal representation of a brick
        std::vector<Brick> brick;

        /*! compute world-space bounding box (lot in _logical_ space,
            but in _absolute_ space, with proper cell width as specified
            in each level */
        inline box3f worldBounds() const
        {
          box3f worldBounds = empty;
          for (const auto &b : brick)
            worldBounds.extend(b.worldBounds);
          return worldBounds;
        }
      };

    }  // namespace amr
  }    // namespace ispc_driver
}  // namespace openvkl
