// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "VKLWindow.h"
// std
#include <iostream>
#include <stdexcept>
// rkcommon
#include "rkcommon/utility/SaveImage.h"

#include "renderers/DensityPathTracer.h"
#include "renderers/HitIterator.h"
#include "renderers/IntervalIteratorDebug.h"
#include "renderers/RayMarchIterator.h"

namespace openvkl {
  namespace examples {

    VKLWindow::VKLWindow(const vec2i &windowSize,
                         const Scene& scene,
                         std::string rendererType)
        : windowSize(windowSize), scene(scene)
    {
      assert(scene.volume);
      const auto volumeBounds = vklGetBoundingBox(scene.volume);
      arcballCamera = std::unique_ptr<ArcballCamera>(
          new ArcballCamera((const box3f &)volumeBounds, windowSize));

      renderer_density_pathtracer =
          std::unique_ptr<Renderer>(new DensityPathTracer());
      renderer_density_pathtracer->commit();
      renderer_hit_iterator =
          std::unique_ptr<Renderer>(new HitIterator());
      renderer_hit_iterator->commit();
      renderer_ray_march_iterator =
          std::unique_ptr<Renderer>(new RayMarchIterator());
      renderer_ray_march_iterator->commit();
      renderer_interval_iterator_debug =
          std::unique_ptr<Renderer>(new IntervalIteratorDebug());
      renderer_interval_iterator_debug->commit();

      setActiveRenderer(rendererType);
      reshape(this->windowSize);
    }

    void VKLWindow::render()
    {
      if (scene.volume)
      {
        if (useISPC)
          renderer->renderFrame_ispc(scene);
        else
          renderer->renderFrame(scene);
      }
    }

    Renderer &VKLWindow::currentRenderer()
    {
      return *renderer;
    }

    void VKLWindow::resetAccumulation()
    {
      renderer->resetAccumulation();
    }

    void VKLWindow::resetCamera()
    {
      if (scene.volume)
      {
        const auto volumeBounds = vklGetBoundingBox(scene.volume);
        arcballCamera->resetCamera((const box3f &)volumeBounds);
        updateCamera();
      }
    }

    void VKLWindow::setUseISPC(bool enabled)
    {
      useISPC = enabled;
    }

    void VKLWindow::savePPM(const std::string &filename)
    {
      const auto &fb = renderer->frameBuffer();
      utility::writePFM(filename, windowSize.x, windowSize.y, fb.data());
    }

    void VKLWindow::setActiveRenderer(const std::string &rendererType)
    {
      if (rendererType == "density_pathtracer")
        renderer = renderer_density_pathtracer.get();
      else if (rendererType == "hit_iterator")
        renderer = renderer_hit_iterator.get();
      else if (rendererType == "ray_march_iterator")
        renderer = renderer_ray_march_iterator.get();
      else if (rendererType == "interval_iterator_debug")
        renderer = renderer_interval_iterator_debug.get();
      else
        throw std::runtime_error("VKLWindow: unknown renderer type");

      updateCamera();
    }

    void VKLWindow::reshape(const vec2i &newWindowSize)
    {
      windowSize = newWindowSize;

      renderer_density_pathtracer->setFrameSize(windowSize);
      renderer_hit_iterator->setFrameSize(windowSize);
      renderer_ray_march_iterator->setFrameSize(windowSize);
      renderer_interval_iterator_debug->setFrameSize(windowSize);

      // update camera
      arcballCamera->updateWindowSize(windowSize);
      updateCamera();
    }

    void VKLWindow::setRenderPixelRange(const region2i &pixelRange)
    {
      if (area(pixelRange) <= 0 || anyLessThan(pixelRange.lower, vec2i(0)) ||
          anyLessThan(pixelRange.upper, vec2i(0)) ||
          anyLessThan(windowSize, pixelRange.lower) ||
          anyLessThan(windowSize, pixelRange.upper)) {
        throw std::runtime_error("VKLWindow: bad pixelRange");
      }
      renderer_density_pathtracer->setPixelRange(pixelRange);
      renderer_hit_iterator->setPixelRange(pixelRange);
      renderer_ray_march_iterator->setPixelRange(pixelRange);
      renderer_interval_iterator_debug->setPixelRange(pixelRange);
    }

    void VKLWindow::updateCamera()
    {
      resetAccumulation();

      renderer->setCamera(arcballCamera->eyePos(),
                          arcballCamera->lookDir(),
                          arcballCamera->upDir(),
                          windowSize.x / float(windowSize.y));
    }

  }  // namespace examples
}  // namespace openvkl
