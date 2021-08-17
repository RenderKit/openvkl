// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BatchApplication.h"
#include "renderer/Framebuffer.h"
#include "renderer/Renderer.h"
#include "renderer/Scene.h"
#include "renderer/Scheduler.h"

#include "rkcommon/utility/SaveImage.h"

namespace openvkl {
  namespace examples {

    BatchApplication::BatchApplication() {}

    BatchApplication::~BatchApplication() {}

    void BatchApplication::run(Scene &scene)
    {
      if (scene.rendererTypes.empty()) {
        scene.rendererTypes = {"density_path_tracer_ispc"};
      }

      auto &scheduler = scene.scheduler;
      auto &volume    = scene.volume;

      scene.rendererParams->fixedFramebufferSize = true;
      const vec2i resolution = scene.rendererParams->framebufferSize;

      std::cout << "Creating VKL objects ..." << std::endl;
      volume.updateVKLObjects();
      volume.printInfo();
      scene.camera->fitToScreen(volume.getBounds());
      scene.camera.incrementVersion();

      for (const auto &type : scene.rendererTypes) {
        auto rendererPtr = scene.createRenderer(type);
        if (!rendererPtr) {
          continue;
        }

        Renderer &renderer = *(rendererPtr.get());
        // This call will resize the framebuffer to our desired output
        // resolution.
        renderer.getFramebuffer(resolution.x, resolution.y);

        const std::string filename = type + ".pfm";

        std::cout << "Rendering with " << type << " ..." << std::endl;

        scheduler.start(renderer);
        for (unsigned i = 0; i < scene.batchModeSpp; ++i) {
          std::cout << "\r" << i << " / " << scene.batchModeSpp << " spp"
                    << std::flush;
          scheduler.renderFrame(renderer);
        }
        scheduler.stop(renderer);
        std::cout << std::endl;

        const auto &framebuffer =
            renderer.getFramebuffer(resolution.x, resolution.y);
        const auto &fb = framebuffer.getFrontBuffer();
        std::cout << "Writing " << filename << " ..." << std::endl;
        rkcommon::utility::writePFM(
            filename, fb.getWidth(), fb.getHeight(), fb.getRgba());
      }
    }

  }  // namespace examples
}  // namespace openvkl

