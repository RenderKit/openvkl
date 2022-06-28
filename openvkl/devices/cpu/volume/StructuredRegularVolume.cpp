// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "StructuredRegularVolume.h"
#include "../common/export_util.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    void StructuredRegularVolume<W>::commit()
    {
      if (!this->SharedStructInitialized) {
        SharedStructuredVolume_Constructor(this->getSh());
        this->SharedStructInitialized = true;

        if (!this->SharedStructInitialized) {
          throw std::runtime_error(
              "could not initialized device-side object for StructuredRegularVolume");
        }
      }
      StructuredVolume<W>::commit();

      std::vector<const ispc::Data1D *> ispcAttributesData =
          ispcs(this->attributesData);

      bool success = SharedStructuredVolume_set(
                               this->getSh(),
                               ispcAttributesData.size(),
                               ispcAttributesData.data(),
                               this->temporallyStructuredNumTimesteps,
                               ispc(this->temporallyUnstructuredIndices),
                               ispc(this->temporallyUnstructuredTimes),
                               this->dimensions,
                               ispc::structured_regular,
                               this->gridOrigin,
                               this->gridSpacing,
                               (ispc::VKLFilter)this->filter);

      if (!success) {
        SharedStructuredVolume_Destructor(this->getSh());
        this->SharedStructInitialized = false;

        throw std::runtime_error("failed to commit StructuredRegularVolume");
      }

      this->setBackground(this->background->data());

      // must be last
      this->buildAccelerator();
    }

    // this is the old / legacy structured regular implementation!
    VKL_REGISTER_VOLUME(StructuredRegularVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_structuredRegularLegacy_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
