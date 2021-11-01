// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

  #include "TransferFunction.h"
  #include <rkcommon/math/box.h>
  #include <rkcommon/math/vec.h>
  #include <list>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    struct Scene;

    struct RendererParams
    {
      int attributeIndex{0};
      float time {0.f};

      // When this is on, ask all renderers to use this framebuffer size.
      bool fixedFramebufferSize {false};
      vec2i framebufferSize {1024, 768};

      // Only render pixels in this range.
      bool restrictPixelRange {false};
      region2i pixelRange {vec2i(0, 0), vec2i(1024, 768)};

      TransferFunction transferFunction;

      void parseCommandLine(std::list<std::string> &args);
      void usage() const;
    };

  }  // namespace examples
}  // namespace openvkl
