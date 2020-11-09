// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "StructuredRegularVolume.h"
#include "../common/export_util.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    void StructuredRegularVolume<W>::commit()
    {
      StructuredVolume<W>::commit();

      if (!this->ispcEquivalent) {
        this->ispcEquivalent = CALL_ISPC(SharedStructuredVolume_Constructor);

        if (!this->ispcEquivalent) {
          throw std::runtime_error(
              "could not create ISPC-side object for StructuredRegularVolume");
        }
      }

      std::vector<const ispc::Data1D *> ispcAttributesData =
          ispcs(this->attributesData);

      bool success = CALL_ISPC(SharedStructuredVolume_set,
                               this->ispcEquivalent,
                               ispcAttributesData.size(),
                               ispcAttributesData.data(),
                               this->temporallyStructuredNumTimesteps,
                               ispc(this->temporallyUnstructuredIndices),
                               ispc(this->temporallyUnstructuredTimes),
                               (const ispc::vec3i &)this->dimensions,
                               ispc::structured_regular,
                               (const ispc::vec3f &)this->gridOrigin,
                               (const ispc::vec3f &)this->gridSpacing,
                               (ispc::VKLFilter)this->filter);

      if (!success) {
        CALL_ISPC(SharedStructuredVolume_Destructor, this->ispcEquivalent);
        this->ispcEquivalent = nullptr;

        throw std::runtime_error("failed to commit StructuredRegularVolume");
      }

      // must be last
      this->buildAccelerator();
    }

    VKL_REGISTER_VOLUME(StructuredRegularVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_structuredRegular_, VKL_TARGET_WIDTH))

  }  // namespace ispc_driver
}  // namespace openvkl
