// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Scheduler.h"
#include <rkcommon/math/vec.h>

namespace openvkl {
  namespace examples {
    class Buffer : public Scheduler::Lockable
    {
     public:
      using vec4f = rkcommon::math::vec4f;

      size_t getWidth() const
      {
        return w;
      }
      size_t getHeight() const
      {
        return h;
      }

      virtual vec4f *getRgba() = 0;

      virtual const vec4f *getRgba() const = 0;

      virtual void resize(size_t _w, size_t _h) = 0;

     protected:
      size_t w{0};
      size_t h{0};
    };
  }  // namespace examples
}  // namespace openvkl