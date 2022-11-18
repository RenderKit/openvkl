// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"
#include "../iterator/DefaultIterator.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct StructuredSphericalVolume : public StructuredVolume<W>
    {
      StructuredSphericalVolume<W>(Device *device) : StructuredVolume<W>(device) {};
      std::string toString() const override;

      void commit() override;

      Sampler<W> *newSampler() override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline std::string StructuredSphericalVolume<W>::toString() const
    {
      return "openvkl::StructuredSphericalVolume";
    }

  }  // namespace cpu_device
}  // namespace openvkl
