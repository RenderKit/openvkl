// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UnstructuredBVH.h"
#include "../Volume.h"
#include "AMRAccel.h"
#include "rkcommon/memory/RefCount.h"
#include "AMRVolumeShared.h"
#include "../UnstructuredVolumeBase.h"
#include "openvkl/common/StructShared.h"

using namespace rkcommon::memory;

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct AMRVolume
        : public AddStructShared<UnstructuredVolumeBase<W>, ispc::AMRVolume>
    {
      AMRVolume();
      ~AMRVolume() override;

      std::string toString() const override;

      void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;
      unsigned int getNumAttributes() const override;
      range1f getValueRange(unsigned int attributeIndex) const override;

      VKLAMRMethod getAMRMethod() const;

      int getBvhDepth() const;

     private:
      std::unique_ptr<amr::AMRData> data;
      std::unique_ptr<amr::AMRAccel> accel;

      Ref<const DataT<Data *>> blockDataData;
      Ref<const DataT<box3i>> blockBoundsData;
      Ref<const DataT<int>> refinementLevelsData;
      Ref<const DataT<float>> cellWidthsData;
      VKLDataType voxelType;
      range1f valueRange{empty};
      box3f bounds;
      vec3f origin;
      vec3f spacing;

      VKLAMRMethod amrMethod{VKL_AMR_CURRENT};

      Ref<const DataT<float>> background;

      // for interval iteration
      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};
      int bvhDepth{0};

      void buildBvh();
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline int AMRVolume<W>::getBvhDepth() const
    {
      return bvhDepth;
    }

  }  // namespace cpu_device
}  // namespace openvkl
