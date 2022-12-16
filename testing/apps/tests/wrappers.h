// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <functional>

namespace openvkl {
  namespace testing {

    static const size_t maxNumIntervals = 999;

    template <class T>
    T *allocate(size_t elements)
    {
#if OPENVKL_TESTING_GPU
      T *ptr = sycl::malloc_shared<T>(elements, getSyclQueue());
#endif
#if OPENVKL_TESTING_CPU
      T *ptr = new T[elements]();
#endif
      memset(ptr, 0, elements * sizeof(T));
      return ptr;
    }

    template <class T>
    void deallocate(T *ptr)
    {
#if OPENVKL_TESTING_GPU
      sycl::free(ptr, getSyclQueue());
#endif
#if OPENVKL_TESTING_CPU
      delete[] ptr;
#endif
    }

    template <class T>
    inline void executeTestFunction(T testFunction)
    {
#if OPENVKL_TESTING_GPU
      getSyclQueue().single_task(testFunction).wait();
#endif

#if OPENVKL_TESTING_CPU
      testFunction();
#endif
    }

    inline float vklComputeSampleWrapper(const VKLSampler *sampler,
                                         const vkl_vec3f *objectCoordinates,
                                         unsigned int attributeIndex,
                                         float time)
    {
#ifdef OPENVKL_TESTING_CPU
      return vklComputeSample(sampler, objectCoordinates, attributeIndex, time);
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
