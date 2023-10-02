// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "BufferCpu.h"
#include "Framebuffer.h"

namespace openvkl {
  namespace examples {
    class FramebufferCpu : public Framebuffer
    {
     public:
      BufferCpu &getBackBuffer()
      {
        return backBuffer;
      }

      void resize(size_t w, size_t h) override final
      {
        backBuffer.resize(w, h);
        Framebuffer::resize(w, h);
      }

     private:
      BufferCpu backBuffer;
    };

  }  // namespace examples
}  // namespace openvkl
