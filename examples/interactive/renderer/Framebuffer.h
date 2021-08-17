// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Scheduler.h"

#include <rkcommon/containers/AlignedVector.h>
#include <rkcommon/math/vec.h>
#include <mutex>
#include <chrono>

namespace openvkl {
  namespace examples {

    class Framebuffer
    {
      public:
        using vec4f = rkcommon::math::vec4f;
        using vec2i = rkcommon::math::vec2i;

        struct Stats
        {
          using Clock = std::chrono::high_resolution_clock;

          Clock::duration frameTime{0};
          Clock::duration renderTime{0};
          Clock::duration copyTime{0};
          Clock::duration tonemapTime{0};
        };

        class Buffer : public Scheduler::Lockable
        {
          public:
            Buffer();
            Buffer(const Buffer &) = delete;
            Buffer& operator=(const Buffer &) = delete;
            Buffer(Buffer &&other);
            Buffer& operator=(Buffer &&other);
            ~Buffer();

            size_t getWidth() const { return w; }
            size_t getHeight() const { return h; }

            vec4f *getRgba() { return bufRgba.data(); }
            const vec4f *getRgba() const { return bufRgba.data(); }

            float *getWeight() { return bufWeight.data(); }
            const float *getWeight() const { return bufWeight.data(); }

            Stats &getStats() { return stats; }
            const Stats &getStats() const { return stats; }

            void tonemapInPlace();
            void copy(const Buffer &other);

          private:
            friend class Framebuffer;
            void resize(size_t _w, size_t _h);

          private:
            size_t w{0};
            size_t h{0};
            rkcommon::containers::AlignedVector<vec4f> bufRgba;
            rkcommon::containers::AlignedVector<float> bufWeight;
            Stats stats;
        };

        Buffer &getFrontBuffer() {
          return frontBuffer;
        }
        const Buffer &getFrontBuffer() const {
          return frontBuffer;
        }

        Buffer &getBackBuffer() {
          return backBuffer;
        }
        const Buffer &getBackBuffer() const {
          return backBuffer;
        }

        void resize(size_t w, size_t h)
        {
          backBuffer.resize(w, h);
          frontBuffer.resize(w, h);
          width = w;
          height = h;
        }

        size_t getWidth() const {
          return width;
        }

        size_t getHeight() const {
          return height;
        }

      private:
        size_t width{0};
        size_t height{0};
        Buffer frontBuffer;
        Buffer backBuffer;
    };

  }  // namespace examples
}  // namespace openvkl
