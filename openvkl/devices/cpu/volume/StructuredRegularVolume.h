// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct StructuredRegularVolume : public StructuredVolume<W>
    {
      StructuredRegularVolume<W>(Device *device) : StructuredVolume<W>(device) {};
      std::string toString() const override;

      void commit() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string StructuredRegularVolume<W>::toString() const
    {
      return "openvkl::StructuredRegularVolume";
    }

  }  // namespace cpu_device
}  // namespace openvkl
