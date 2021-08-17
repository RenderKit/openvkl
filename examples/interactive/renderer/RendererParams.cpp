// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RendererParams.h"
#include "CommandLine.h"
#include "Scene.h"

#include "openvkl_testing.h"

#include <cstring>
#include <iostream>

using namespace openvkl::testing;

namespace openvkl {
  namespace examples {

    void RendererParams::usage() const
    {
      std::cerr << "\t-valueRange <lower> <upper>\n"
                   "\t-framebufferSize <x> <y>\n"
                   "\t-pixelRange <xMin> <yMin> <xMax> <yMax>\n";
      std::cerr << std::flush;
    }

    void RendererParams::parseCommandLine(std::list<std::string> &args)
    {
      range1f initialValueRange{0.f, 1.f};

      for (auto it = args.begin(); it != args.end();) {
        const std::string arg = *it;
        if (arg == "-framebufferSize") {
          std::tie(framebufferSize.x, framebufferSize.y) =
              cmd::consume_2<int, int>(args, it);
          fixedFramebufferSize = true;
        } else if (arg == "-pixelRange") {
          std::tie(pixelRange.lower.x,
                   pixelRange.lower.y,
                   pixelRange.upper.x,
                   pixelRange.upper.y) =
              cmd::consume_4<int, int, int, int>(args, it);
          restrictPixelRange = true;
          if (!fixedFramebufferSize) {
            fixedFramebufferSize = true;
            framebufferSize.x    = pixelRange.upper.x;
            framebufferSize.y    = pixelRange.upper.y;
          }
        } else if (arg == "-valueRange") {
          std::tie(initialValueRange.lower, initialValueRange.upper) =
              cmd::consume_2<float, float>(args, it);
        } else if (arg == "-time") {
          time = cmd::consume_1<float>(args, it);
        } else if (arg == "-attribute") {
          attributeIndex = cmd::consume_1<int>(args, it);
        } else {
          ++it;
        }
      }

      transferFunction.valueRange = initialValueRange;
    }

  }  // namespace examples
}  // namespace openvkl

