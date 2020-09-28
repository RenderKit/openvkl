// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "StructuredVolume.h"
#include "../iterator/DefaultIterator.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredSphericalVolume : public StructuredVolume<W>
    {
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

  }  // namespace ispc_driver
}  // namespace openvkl
