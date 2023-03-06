// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "GridAccelerator_ispc.h"
#include "SharedStructuredVolume_ispc.h"

#include "../common/export_util.h"
#include "StructuredRegularVolume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    void StructuredRegularVolume<W>::commit()
    {
      if (!this->SharedStructInitialized) {
        ispc::SharedStructuredVolume *self =
            static_cast<ispc::SharedStructuredVolume *>(this->getSh());

        memset(self, 0, sizeof(ispc::SharedStructuredVolume));

        SharedStructuredVolume_Constructor(self);

        self->super.type =
            ispc::DeviceVolumeType::VOLUME_TYPE_STRUCTURED_REGULAR_LEGACY;

        this->SharedStructInitialized = true;
      }

      StructuredVolume<W>::commit();

      std::vector<const ispc::Data1D *> ispcAttributesData =
          ispcs(this->attributesData);

      bool success =
          SharedStructuredVolume_set(this->getSh(),
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
                        CONCAT1(internal_structuredRegularLegacy_,
                                VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
