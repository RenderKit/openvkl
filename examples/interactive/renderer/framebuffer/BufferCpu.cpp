// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <rkcommon/tasking/parallel_for.h>

#include "BufferCpu.h"
#include "Framebuffer_ispc.h"
#include "Renderer_ispc.h"

namespace openvkl {
  namespace examples {

    void BufferCpu::resize(size_t _w, size_t _h)
    {
      const size_t numPixels = _w * _h;

      bufRgba.resize(numPixels);
      bufWeight.resize(numPixels);

      w = _w;
      h = _h;
    }

    void BufferCpu::tonemap(BufferDisplay &outputBuffer)
    {
      const auto start = Stats::Clock::now();

      const size_t numPixels    = w * h;
      const size_t pixelsPerJob = ispc::Renderer_pixelsPerJob();
      const size_t numJobs      = numPixels / pixelsPerJob;
      rkcommon::tasking::parallel_for(numJobs, [&](size_t i) {
        const size_t ib = i * pixelsPerJob;
        ispc::tonemap(static_cast<uint32_t>(ib),
                      static_cast<uint32_t>(numPixels),
                      bufWeight.data(),
                      reinterpret_cast<ispc::vec4f *>(bufRgba.data()),
                      reinterpret_cast<ispc::vec4f *>(outputBuffer.getRgba()));
      });

      const auto end                      = Stats::Clock::now();
      outputBuffer.getStats().tonemapTime = end - start;
    }

  }  // namespace examples
}  // namespace openvkl