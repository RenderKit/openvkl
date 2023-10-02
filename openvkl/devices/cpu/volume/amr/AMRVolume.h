// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UnstructuredBVH.h"
#include "../UnstructuredVolumeBase.h"
#include "../Volume.h"
#include "AMRAccel.h"
#include "AMRVolumeShared.h"
#include "openvkl/devices/common/StructShared.h"
#include "rkcommon/memory/RefCount.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct AMRVolume
        : public AddStructShared<UnstructuredVolumeBase<W>, ispc::AMRVolume>
    {
      AMRVolume(Device *);
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

      rkcommon::memory::Ref<const DataT<Data *>> blockDataData;
      rkcommon::memory::Ref<const DataT<box3i>> blockBoundsData;
      rkcommon::memory::Ref<const DataT<int>> refinementLevelsData;
      rkcommon::memory::Ref<const DataT<float>> cellWidthsData;
      VKLDataType voxelType;
      range1f valueRange{empty};
      box3f bounds;
      vec3f origin;
      vec3f spacing;

      VKLAMRMethod amrMethod{VKL_AMR_CURRENT};

      rkcommon::memory::Ref<const DataT<float>> background;

      // for interval iteration
      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};
      int bvhDepth{0};
      std::unique_ptr<BvhBuildAllocator> bvhBuildAllocator;

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
