// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Framebuffer.h"
#include "Framebuffer_ispc.h"
#include "Renderer_ispc.h"

#include <rkcommon/math/rkmath.h>
#include <rkcommon/math/vec.h>
#include <rkcommon/tasking/parallel_for.h>

#include <algorithm>
#include <chrono>
#include <sstream>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    // -------------------------------------------------------------------------

    Framebuffer::Buffer::Buffer() : Scheduler::Lockable() {}

    Framebuffer::Buffer::Buffer(Buffer &&other)
    {
      using std::swap;
      swap(w, other.w);
      swap(h, other.h);
      swap(bufRgba, other.bufRgba);
      swap(bufWeight, other.bufWeight);
      swap(stats, other.stats);
    }

    Framebuffer::Buffer &Framebuffer::Buffer::operator=(Buffer &&other)
    {
      if (&other != this) {
        using std::swap;
        swap(w, other.w);
        swap(h, other.h);
        swap(bufRgba, other.bufRgba);
        swap(bufWeight, other.bufWeight);
        swap(stats, other.stats);
      }
      return *this;
    }

    Framebuffer::Buffer::~Buffer() = default;

    void Framebuffer::Buffer::resize(size_t _w, size_t _h)
    {
      const size_t numPixels = _w * _h;

      bufRgba.resize(numPixels);
      std::fill(bufRgba.begin(), bufRgba.end(), vec4f(0.f));

      bufWeight.resize(numPixels);
      std::fill(bufWeight.begin(), bufWeight.end(), 0.f);

      w = _w;
      h = _h;
    }

    void Framebuffer::Buffer::copy(const Buffer &other)
    {
      assert(w == other.w);
      assert(h == other.h);

      const auto start = Framebuffer::Stats::Clock::now();
      bufRgba = other.bufRgba;
      bufWeight = other.bufWeight;
      stats          = other.stats;
      const auto end = Framebuffer::Stats::Clock::now();
      stats.copyTime = end - start;
    }

    void Framebuffer::Buffer::tonemapInPlace()
    {
      const auto start = Framebuffer::Stats::Clock::now();

      const size_t numPixels    = w * h;
      const size_t pixelsPerJob = ispc::Renderer_pixelsPerJob();
      const size_t numJobs      = numPixels / pixelsPerJob;
      rkcommon::tasking::parallel_for(numJobs, [&](size_t i) {
        const size_t ib = i * pixelsPerJob;
        ispc::tonemap(static_cast<uint32_t>(ib), 
                      static_cast<uint32_t>(numPixels),
                      bufWeight.data(),
                      reinterpret_cast<ispc::vec4f *>(bufRgba.data()));
      });

      const auto end    = Framebuffer::Stats::Clock::now();
      stats.tonemapTime = end - start;
    }

  }  // namespace examples
}  // namespace openvkl
