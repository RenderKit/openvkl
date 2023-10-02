// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "BufferDisplay.h"

namespace openvkl {
  namespace examples {

    class Framebuffer
    {
     public:
      BufferDisplay &getFrontBuffer()
      {
        return frontBuffer;
      }
      const BufferDisplay &getFrontBuffer() const
      {
        return frontBuffer;
      }

      virtual void resize(size_t w, size_t h)
      {
        frontBuffer.resize(w, h);
        width  = w;
        height = h;
      }

      size_t getWidth() const
      {
        return width;
      }

      size_t getHeight() const
      {
        return height;
      }

     protected:
      size_t width{0};
      size_t height{0};
      /* It's used directly to display */
      BufferDisplay frontBuffer;
    };

  }  // namespace examples
}  // namespace openvkl
