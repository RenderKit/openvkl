// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl_testing.h"

#include "Buffer.h"
#include "Stats.h"

namespace openvkl {
  namespace examples {
    class BufferDisplay : public Buffer
    {
     public:
      ~BufferDisplay()
      {
        deallocateBuffer();
      }

      vec4f *getRgba() override final
      {
        return bufRgba;
      }
      const vec4f *getRgba() const override final
      {
        return bufRgba;
      }

      void resize(size_t _w, size_t _h) override final
      {
        const size_t numPixels = _w * _h;
        deallocateBuffer();
        allocateBuffer(numPixels);
        w = _w;
        h = _h;
      }

      Stats &getStats()
      {
        return stats;
      }

      const Stats &getStats() const
      {
        return stats;
      }

     private:
      vec4f *bufRgba = nullptr;
      Stats stats;

      inline void allocateBuffer(const size_t numPixels)
      {
        assert(bufRgba == nullptr);
#ifdef OPENVKL_TESTING_GPU
        // sycl::malloc_host gives better perf when buffer is copied from GPU
        // than "normal" allocation in host memory.
        bufRgba = sycl::malloc_host<vec4f>(numPixels, getSyclQueue());
#endif
#ifdef OPENVKL_TESTING_CPU
        bufRgba = new vec4f[numPixels];
#endif
      }
      inline void deallocateBuffer()
      {
#ifdef OPENVKL_TESTING_GPU
        sycl::free(bufRgba, getSyclQueue());
#endif
#ifdef OPENVKL_TESTING_CPU
        delete[] bufRgba;
#endif
        bufRgba = nullptr;
      }
    };
  }  // namespace examples
}  // namespace openvkl