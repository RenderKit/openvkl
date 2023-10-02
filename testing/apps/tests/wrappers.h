// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <functional>

namespace openvkl {
  namespace testing {

    static const size_t maxNumIntervals = 999;
    static const size_t maxNumHits      = 999;

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

    inline void vklComputeSampleMWrapper(const VKLSampler *sampler,
                                         const vkl_vec3f *objectCoordinates,
                                         float *samples,
                                         unsigned int M,
                                         const unsigned int *attributeIndices,
                                         float time)
    {
#ifdef OPENVKL_TESTING_CPU
      vklComputeSampleM(
          sampler, objectCoordinates, samples, M, attributeIndices, time);
      return;
#endif

#ifdef OPENVKL_TESTING_GPU
      // we can't pass pointers to SYCL kernel
      const VKLSampler samplerObj          = *sampler;
      const vkl_vec3f objectCoordinatesObj = *objectCoordinates;

      unsigned int *attributeIndicesGPU =
          sycl::malloc_shared<unsigned int>(M, getSyclQueue());
      std::memcpy(
          attributeIndicesGPU, attributeIndices, M * sizeof(unsigned int));

      float *gpuResult = sycl::malloc_shared<float>(M, getSyclQueue());
      getSyclQueue()
          .single_task([=]() {
            vklComputeSampleM(&samplerObj,
                              &objectCoordinatesObj,
                              gpuResult,
                              M,
                              attributeIndicesGPU,
                              time);
          })
          .wait();
      for (int a = 0; a < M; a++) {
        samples[a] = gpuResult[a];
      }
      sycl::free(attributeIndicesGPU, getSyclQueue());
      sycl::free(gpuResult, getSyclQueue());
      return;
#endif
    }

    inline vkl_vec3f vklComputeGradientWrapper(
        const VKLSampler *sampler,
        const vkl_vec3f *objectCoordinates,
        unsigned int attributeIndex,
        float time)
    {
#ifdef OPENVKL_TESTING_CPU
      return vklComputeGradient(
          sampler, objectCoordinates, attributeIndex, time);
#endif

#ifdef OPENVKL_TESTING_GPU
      // we can't pass pointers to SYCL kernel
      const VKLSampler samplerObj          = *sampler;
      const vkl_vec3f objectCoordinatesObj = *objectCoordinates;

      vkl_vec3f *gpuResult = sycl::malloc_shared<vkl_vec3f>(1, getSyclQueue());
      getSyclQueue()
          .single_task([=]() {
            *gpuResult = vklComputeGradient(
                &samplerObj, &objectCoordinatesObj, attributeIndex, time);
          })
          .wait();
      vkl_vec3f scalarGradientValue = *gpuResult;
      sycl::free(gpuResult, getSyclQueue());
      return scalarGradientValue;
#endif
    }

  }  // namespace testing
}  // namespace openvkl
