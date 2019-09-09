// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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
                         VKLVolume volume,
                         std::string rendererType)
        : windowSize(windowSize), volume(volume)
    {
      const auto volumeBounds = vklGetBoundingBox(volume);

      arcballCamera = std::unique_ptr<ArcballCamera>(
          new ArcballCamera((const box3f &)volumeBounds, windowSize));

      renderer_density_pathtracer =
          std::unique_ptr<Renderer>(new DensityPathTracer(volume));
      renderer_hit_iterator =
          std::unique_ptr<Renderer>(new HitIterator(volume));
      renderer_ray_march_iterator =
          std::unique_ptr<Renderer>(new RayMarchIterator(volume));

      setActiveRenderer(rendererType);

      renderer_density_pathtracer->commit();
      renderer_hit_iterator->commit();
      renderer_ray_march_iterator->commit();

      reshape(this->windowSize);
    }

    void VKLWindow::render()
    {
      if (useISPC)
        renderer->renderFrame_ispc();
      else
        renderer->renderFrame();
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
      const auto volumeBounds = vklGetBoundingBox(volume);
      arcballCamera->resetCamera((const box3f &)volumeBounds);
      updateCamera();
    }

    void VKLWindow::setUseISPC(bool enabled)
    {
      useISPC = enabled;
    }

    void VKLWindow::setTransferFunction(
        const TransferFunction &transferFunction)
    {
      this->transferFunction = transferFunction;
      renderer->setTransferFunction(transferFunction.valueRange,
                                    transferFunction.colorsAndOpacities);
    }

    void VKLWindow::setIsovalues(const std::vector<float> &isovalues)
    {
      this->isovalues = isovalues;
      renderer->setIsovalues(isovalues);
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

      renderer->setTransferFunction(transferFunction.valueRange,
                                    transferFunction.colorsAndOpacities);

      renderer->setIsovalues(isovalues);
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