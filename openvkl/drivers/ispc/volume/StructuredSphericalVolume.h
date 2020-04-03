// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredSphericalVolume : public StructuredVolume<W>
    {
      void commit() override;

      // note, although we construct the GridAccelerator for all structured
      // volumes (including this one), we only use it here for computing
      // value range. we don't yet use it for iterators since the nextCell()
      // implementation is only correct for structured regular volumes.

    };

  }  // namespace ispc_driver
}  // namespace openvkl
