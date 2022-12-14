// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../../api/Device.h"

namespace openvkl {
  namespace gpu_device {

    struct AddDeviceAPIs : public api::Device
    {
      /////////////////////////////////////////////////////////////////////////
      // Sampler //////////////////////////////////////////////////////////////
      /////////////////////////////////////////////////////////////////////////

      virtual void computeSample1(const int *valid,
                                  const VKLSampler *sampler,
                                  const vvec3fn<1> &objectCoordinates,
                                  float *samples,
                                  unsigned int attributeIndex,
                                  const float *times) = 0;

      virtual void computeSampleM1(const int *valid,
                                   const VKLSampler *sampler,
                                   const vvec3fn<1> &objectCoordinates,
                                   float *samples,
                                   unsigned int M,
                                   const unsigned int *attributeIndices,
                                   const float *times) = 0;

      virtual void computeGradient1(const int *valid,
                                    const VKLSampler *sampler,
                                    const vvec3fn<1> &objectCoordinates,
                                    vvec3fn<1> &gradients,
                                    unsigned int attributeIndex,
                                    const float *times) = 0;
    };

  }  // namespace gpu_device
}  // namespace openvkl
