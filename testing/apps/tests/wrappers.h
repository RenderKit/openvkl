// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <functional>

namespace openvkl {
  namespace testing {

    inline float vklComputeSampleWrapper(const VKLSampler *sampler,
                                         const vkl_vec3f *objectCoordinates,
                                         unsigned int attributeIndex,
                                         float time)
    {
#ifdef OPENVKL_TESTING_CPU
      return vklComputeSample(
          sampler, objectCoordinates, attributeIndex, time);
#endif

#ifdef OPENVKL_TESTING_GPU
      // we can't pass pointers to SYCL kernel
      const VKLSampler samplerObj          = *sampler;
      const vkl_vec3f objectCoordinatesObj = *objectCoordinates;

      float *gpuResult = sycl::malloc_shared<float>(1, getSyclQueue());
      getSyclQueue()
          .single_task([=]() {
            *gpuResult = vklComputeSample(
                &samplerObj, &objectCoordinatesObj, attributeIndex, time);
          })
          .wait();
      float scalarSampledValue = *gpuResult;
      sycl::free(gpuResult, getSyclQueue());
      return scalarSampledValue;
#endif
    }

  }  // namespace testing
}  // namespace openvkl
