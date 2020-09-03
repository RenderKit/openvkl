// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../sampler/Sampler.h"
#include "../../observer/ObserverRegistry.h"
#include "../common/simd.h"
#include "VdbGrid.h"
#include "VdbSampleConfig.h"
#include "VdbVolume.h"
#include "openvkl/openvkl.h"
#include "openvkl/vdb.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbSampler : public Sampler<W>
    {
      explicit VdbSampler(VdbVolume<W> &volume);

      ~VdbSampler() override;

      void commit() override;

      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples,
                         unsigned int attributeIndex) const override final;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples,
                          unsigned int attributeIndex) const override final;

      void computeSampleN(unsigned int N,
                          const vvec3fn<1> *objectCoordinates,
                          float *samples,
                          unsigned int attributeIndex) const override final;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients,
                            unsigned int attributeIndex) const override final;

      void computeGradientN(unsigned int N,
                            const vvec3fn<1> *objectCoordinates,
                            vvec3fn<1> *gradients,
                            unsigned int attributeIndex) const override final;

      Observer<W> *newObserver(const char *type) override;

      ObserverRegistry<W> &getLeafAccessObserverRegistry() {
        return leafAccessObservers;
      }

      private:
        Ref<VdbVolume<W>> volume;
        ObserverRegistry<W> leafAccessObservers;
        VdbSampleConfig config;
    };

  }  // namespace ispc_driver
}  // namespace openvkl
