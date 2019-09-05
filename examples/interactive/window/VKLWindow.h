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

#pragma once

#include "ArcballCamera.h"
#include "renderers/Renderer.h"
// ospcommon
#include "ospcommon/math/box.h"
// std
#include <string>

namespace openvkl {
  namespace examples {

    struct TransferFunction
    {
      TransferFunction() = default;

      TransferFunction(const range1f &valueRange,
                       const std::vector<vec4f> &colorsAndOpacities)
          : valueRange(valueRange), colorsAndOpacities(colorsAndOpacities)
      {
      }

      range1f valueRange{-1.f, 1.f};
      std::vector<vec4f> colorsAndOpacities{
          {0.f, 0.f, 1.f, 0.f}, {0.f, 1.f, 0.f, 0.5f}, {1.f, 0.f, 0.f, 1.f}};
    };

    class VKLWindow
    {
     public:
      VKLWindow(const vec2i &windowSize,
                const box3f &volumeBounds,
                VKLVolume volume,
                const range1f &voxelRange,
                VKLSamplesMask mask,
                std::string rendererType);

      virtual ~VKLWindow();

      void render();

      Renderer &currentRenderer();

      void resetAccumulation();

      void resetCamera();

      void setUseISPC(bool enabled);

      void setTransferFunction(const TransferFunction &transferFunction);

      void setIsovalues(int numValues, const float *values);

      void savePPM(const std::string &filename);

      void setActiveRenderer(const std::string &rendererType);

     protected:
      virtual void reshape(const vec2i &newWindowSize);

      void updateCamera();

      void updateTransferFunction();

      bool useISPC{true};

      vec2i windowSize;
      box3f volumeBounds;
      VKLVolume volume{nullptr};

      range1f voxelRange;

      VKLSamplesMask samplesMask{nullptr};

      Renderer *renderer;

      std::unique_ptr<Renderer> renderer_hit_iterator;
      std::unique_ptr<Renderer> renderer_ray_marcher;
      std::unique_ptr<Renderer> renderer_pathtracer;

      ArcballCamera arcballCamera;

      TransferFunction transferFunction;
    };

  }  // namespace examples
}  // namespace openvkl