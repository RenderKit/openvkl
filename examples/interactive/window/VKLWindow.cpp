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

#include "renderers/DensityOnlyPathTracer.h"
#include "renderers/IntersectBounds.h"

namespace openvkl {
  namespace examples {

    VKLWindow::VKLWindow(const vec2i &windowSize,
                         const box3f &volumeBounds,
                         VKLVolume v,
                         VKLSamplesMask m,
                         std::string rendererType)
        : windowSize(windowSize),
          volumeBounds(volumeBounds),
          volume(v),
          mask(m),
          arcballCamera(volumeBounds, windowSize)
    {
      // TODO: create a real renderer based on 'rendererType'
      // renderer = std::unique_ptr<Renderer>(new IntersectBounds);
      renderer = std::unique_ptr<Renderer>(new DensityOnlyPathTracer);

      renderer->commit();

      reshape(this->windowSize);
    }

    void VKLWindow::render()
    {
      renderer->renderFrame(volume, mask);
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
      arcballCamera.resetCamera(volumeBounds);
      updateCamera();
    }

    void VKLWindow::updateCamera()
    {
      resetAccumulation();

      renderer->setCamera(arcballCamera.eyePos(),
                          arcballCamera.lookDir(),
                          arcballCamera.upDir(),
                          windowSize.x / float(windowSize.y));
    }

    void VKLWindow::savePPM(const std::string &filename)
    {
      const auto &fb = renderer->frameBuffer();
      utility::writePFM(filename, windowSize.x, windowSize.y, fb.data());
    }

    void VKLWindow::reshape(const vec2i &newWindowSize)
    {
      windowSize = newWindowSize;

      renderer->setFrameSize(windowSize);

      // update camera
      arcballCamera.updateWindowSize(windowSize);
      updateCamera();
    }

  }  // namespace examples
}  // namespace openvkl