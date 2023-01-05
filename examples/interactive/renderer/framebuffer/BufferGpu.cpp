// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BufferGpu.h"

namespace {
  inline vec4f linearToSrgba(const vec4f &rgba)
  {
    const float gamma = 1.f / 2.2f;
    return vec4f(sycl::native::powr(rgba.x * rgba.w, gamma),
                 sycl::native::powr(rgba.y * rgba.w, gamma),
                 sycl::native::powr(rgba.z * rgba.w, gamma),
                 1.f);
  }
}  // namespace

namespace openvkl {
  namespace examples {

    void BufferGpu::resize(size_t _w, size_t _h)
    {
      const size_t numPixels = _w * _h;

      deallocateBuffers();
      allocateBuffers(numPixels);

      w = _w;
      h = _h;
    }

    sycl::event BufferGpu::invokeGpuTonemap()
    {
      auto rgbaBuffer         = getRgba();
      auto outRgbaBuffer      = getRgbaOutput();
      auto weightBuffer       = getWeight();
      auto gpuTonemapFunction = [=](auto idx) {
        const float weight = weightBuffer[idx];
        if (weight <= 0.f) {
          outRgbaBuffer[idx] = vec4f(0.f, 0.f, 0.f, 1.f);
          return;
        }

        const vec4f rgba    = rgbaBuffer[idx];
        const float rweight = sycl::native::recip(weight);
        outRgbaBuffer[idx]  = linearToSrgba(rweight * rgba);
      };

      return syclQueue.parallel_for(getWidth() * getHeight(),
                                    gpuTonemapFunction);
    }

    void BufferGpu::tonemap(BufferDisplay &outputBuffer)
    {
      const auto startFunction   = Stats::Clock::now();
      const auto gpuTonemapEvent = invokeGpuTonemap();
      syclQueue.wait();
      const auto endTonemap = Stats::Clock::now();

      const auto copyBufferEvent =
          syclQueue.memcpy(outputBuffer.getRgba(),
                           getRgbaOutput(),
                           sizeof(vec4f) * getWidth() * getHeight());
      syclQueue.wait();
      const auto endCopying = Stats::Clock::now();

      outputBuffer.getStats().tonemapTime = endTonemap - startFunction;
      outputBuffer.getStats().copyTime    = endCopying - endTonemap;
    }

  }  // namespace examples
}  // namespace openvkl