// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Volume.h"
#include "AMRAccel.h"
#include "ospcommon/memory/RefCount.h"

using namespace ospcommon::memory;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct AMRVolume : public Volume<W>
    {
      AMRVolume();
      ~AMRVolume() override = default;

      std::string toString() const override;

      void commit() override;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;
      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override;
      box3f getBoundingBox() const override;
      range1f getValueRange() const override;

      std::unique_ptr<amr::AMRData> data;
      std::unique_ptr<amr::AMRAccel> accel;

      Ref<const Data> blockDataData;
      Ref<const Data> blockBoundsData;
      Ref<const Data> refinementLevelsData;
      Ref<const Data> cellWidthsData;
      VKLDataType voxelType;
      range1f valueRange{empty};
      box3f bounds;

      VKLAMRMethod amrMethod;
    };

  }  // namespace ispc_driver
}  // namespace openvkl
