// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "VdbVolume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct DenseVdbVolume : public VdbVolume<W>
    {
      DenseVdbVolume<W>(Device *device);
      std::string toString() const override;

      void commit() override;

     protected:
      void initIndexSpaceTransforms() override final;
      void initLeafNodeData() override final;

     private:
      void parseStructuredVolumeParameters();

      // structured volume parameters
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      vec3i indexOrigin;
      std::vector<Ref<const Data>> attributesData;
      VKLTemporalFormat temporalFormat;
      int temporallyStructuredNumTimesteps;
      Ref<const Data> temporallyUnstructuredIndices;
      Ref<const DataT<float>> temporallyUnstructuredTimes;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    DenseVdbVolume<W>::DenseVdbVolume(Device *device) : VdbVolume<W>(device)
    {
      ispc::VdbVolume *self = static_cast<ispc::VdbVolume *>(this->getSh());

      // should already be initialized in VdbVolume constructor
      assert(this->SharedStructInitialized);

      self->super.type = ispc::DeviceVolumeType::VOLUME_TYPE_STRUCTURED_REGULAR;
    }

    template <int W>
    inline std::string DenseVdbVolume<W>::toString() const
    {
      return "openvkl::DenseVdbVolume";
    }

  }  // namespace cpu_device
}  // namespace openvkl
