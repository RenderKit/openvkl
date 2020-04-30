// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../sampler/Sampler.h"
#include "../common/simd.h"
#include "VdbGrid.h"
#include "VdbSampleConfig.h"
#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbSampler : public Sampler<W>
    {
      VdbSampler(const VdbGrid *grid, const VdbSampleConfig &defaultConfig);

      ~VdbSampler() override;

      void commit() override;

      // samplers can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples) const override final;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients) const override final;

      const VdbGrid *grid{nullptr};
      VdbSampleConfig config;
    };

  }  // namespace ispc_driver
}  // namespace openvkl

