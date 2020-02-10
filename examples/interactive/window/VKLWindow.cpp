// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "VKLWindow.h"
// std
#include <iostream>
#include <stdexcept>
// ospcommon
#include "ospcommon/utility/SaveImage.h"

#include "renderers/DensityPathTracer.h"
#include "renderers/HitIterator.h"
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

      // update camera
      arcballCamera->updateWindowSize(windowSize);
      updateCamera();
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
