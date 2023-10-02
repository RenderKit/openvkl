// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/vec.h>

#include "openvkl_testing.h"

#include "Buffer.h"
#include "BufferDisplay.h"

namespace openvkl {
  namespace examples {
    class BufferGpu : public Buffer
    {
     public:
      BufferGpu()
      {
        syclQueue = getSyclQueue();
      };

      ~BufferGpu()
      {
        deallocateBuffers();
      };

      vec4f *getRgba() override final
      {
        return bufRgba;
      }

      const vec4f *getRgba() const override final
      {
        return bufRgba;
      }

      vec4f *getRgbaOutput()
      {
        return bufRgbaOutput;
      }

      const vec4f *getRgbaOutput() const
      {
        return bufRgbaOutput;
      }

      float *getWeight()
      {
        return bufWeight;
      }

      const float *getWeight() const
      {
        return bufWeight;
      }

      void resize(size_t _w, size_t _h) override final;
      void tonemap(BufferDisplay &outputBuffer);

     private:
      sycl::queue syclQueue;
      vec4f *bufRgba       = nullptr;
      vec4f *bufRgbaOutput = nullptr;
      float *bufWeight     = nullptr;

      sycl::event invokeGpuTonemap();

      inline void allocateBuffers(const size_t numPixels)
      {
        assert(numPixels > 0);
        bufRgba       = sycl::malloc_device<vec4f>(numPixels, syclQueue);
        bufRgbaOutput = sycl::malloc_device<vec4f>(numPixels, syclQueue);
        bufWeight     = sycl::malloc_device<float>(numPixels, syclQueue);
      }

      inline void deallocateBuffers()
      {
        sycl::free(bufRgba, syclQueue);
        bufRgba = nullptr;

        sycl::free(bufRgbaOutput, syclQueue);
        bufRgbaOutput = nullptr;

        sycl::free(bufWeight, syclQueue);
        bufWeight = nullptr;
      }
    };
  }  // namespace examples
}  // namespace openvkl