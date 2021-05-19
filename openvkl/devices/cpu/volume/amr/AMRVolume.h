// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../UnstructuredBVH.h"
#include "../Volume.h"
#include "AMRAccel.h"
#include "rkcommon/memory/RefCount.h"

using namespace rkcommon::memory;

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct AMRVolume : public Volume<W>
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

      int getMaxIteratorDepth() const;

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

      // for interval iteration
      int maxIteratorDepth{0};

      RTCBVH rtcBVH{0};
      RTCDevice rtcDevice{0};
      Node *rtcRoot{nullptr};

      void buildBvh();
    };

  }  // namespace cpu_device
}  // namespace openvkl
