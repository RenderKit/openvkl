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
                         const box3f &volumeBounds,
                         VKLVolume v,
                         const range1f &vRange,
                         VKLSamplesMask m,
                         std::string rendererType)
        : windowSize(windowSize),
          volumeBounds(volumeBounds),
          volume(v),
          voxelRange(vRange),
          samplesMask(m),
          arcballCamera(volumeBounds, windowSize)
    {
      renderer_hit_iterator = std::unique_ptr<Renderer>(new HitIterator);
      renderer_ray_marcher  = std::unique_ptr<Renderer>(new RayMarchIterator);
      renderer_pathtracer   = std::unique_ptr<Renderer>(new DensityPathTracer);

      setActiveRenderer(rendererType);

      renderer_hit_iterator->setParam<range1f>("voxelRange", voxelRange);
      renderer_ray_marcher->setParam<range1f>("voxelRange", voxelRange);
      renderer_pathtracer->setParam<range1f>("voxelRange", voxelRange);

      renderer_hit_iterator->commit();
      renderer_ray_marcher->commit();
      renderer_pathtracer->commit();

      samplesMask = vklNewSamplesMask(volume);
      vklSamplesMaskSetRanges(samplesMask, 1, (const vkl_range1f *)&voxelRange);
      vklCommit((VKLObject)samplesMask);

      reshape(this->windowSize);
    }

    VKLWindow::~VKLWindow()
    {
      if (samplesMask) {
        vklRelease((VKLObject)samplesMask);
        samplesMask = nullptr;
      }
    }

    void VKLWindow::render()
    {
      if (useISPC)
        renderer->renderFrame_ispc(volume, samplesMask);
      else
        renderer->renderFrame(volume, samplesMask);
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

    void VKLWindow::updateTransferFunction()
    {
      renderer->setTransferFunction(transferFunction.valueRange,
                                    transferFunction.colorsAndOpacities);
    }

    void VKLWindow::setUseISPC(bool enabled)
    {
      useISPC = enabled;
    }

    void VKLWindow::setTransferFunction(
        const TransferFunction &transferFunction)
    {
      this->transferFunction = transferFunction;

      updateTransferFunction();
    }

    void VKLWindow::setIsovalues(int numValues, const float *values)
    {
      if (samplesMask) {
        vklRelease((VKLObject)samplesMask);
        samplesMask = nullptr;
      }

      if (numValues > 0) {
        samplesMask = vklNewSamplesMask(volume);
        vklSamplesMaskSetValues(samplesMask, numValues, values);
        vklSamplesMaskSetRanges(
            samplesMask, 1, (const vkl_range1f *)&voxelRange);
        vklCommit((VKLObject)samplesMask);
      }

      renderer->setParam<const float *>("isovalues", values);
      renderer->commit();
    }

    void VKLWindow::savePPM(const std::string &filename)
    {
      const auto &fb = renderer->frameBuffer();
      utility::writePFM(filename, windowSize.x, windowSize.y, fb.data());
    }

    void VKLWindow::setActiveRenderer(const std::string &rendererType)
    {
      if (rendererType == "hit_iterator")
        renderer = renderer_hit_iterator.get();
      else if (rendererType == "ray_march_iterator")
        renderer = renderer_ray_marcher.get();
      else
        renderer = renderer_pathtracer.get();

      updateCamera();
      updateTransferFunction();
    }

    void VKLWindow::reshape(const vec2i &newWindowSize)
    {
      windowSize = newWindowSize;

      renderer_hit_iterator->setFrameSize(windowSize);
      renderer_ray_marcher->setFrameSize(windowSize);
      renderer_pathtracer->setFrameSize(windowSize);

      // update camera
      arcballCamera.updateWindowSize(windowSize);
      updateCamera();
    }

  }  // namespace examples
}  // namespace openvkl