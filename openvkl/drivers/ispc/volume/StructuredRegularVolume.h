// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      std::string toString() const override;

      void commit() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string StructuredRegularVolume<W>::toString() const
    {
      return "openvkl::StructuredRegularVolume";
    }

  }  // namespace ispc_driver
}  // namespace openvkl
