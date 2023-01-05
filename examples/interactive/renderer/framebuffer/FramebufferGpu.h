// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "BufferGpu.h"
#include "Framebuffer.h"

namespace openvkl {
  namespace examples {
    class FramebufferGpu : public Framebuffer
    {
     public:
      BufferGpu &getBackBuffer()
      {
        return backBuffer;
      }
      void resize(size_t w, size_t h) override final
      {
        backBuffer.resize(w, h);
        Framebuffer::resize(w, h);
      }

     private:
      BufferGpu backBuffer;
    };

  }  // namespace examples
}  // namespace openvkl
