// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

    class VKLWindow
    {
     public:
      VKLWindow(const vec2i &windowSize,
                VKLVolume volume,
                std::string rendererType);

      virtual ~VKLWindow() = default;

      void render();

      Renderer &currentRenderer();

      void resetAccumulation();

      void resetCamera();

      void setUseISPC(bool enabled);

      void setTransferFunction(const TransferFunction &transferFunction);

      void setIsovalues(const std::vector<float> &isovalues);

      void savePPM(const std::string &filename);

      void setActiveRenderer(const std::string &rendererType);

     protected:
      virtual void reshape(const vec2i &newWindowSize);

      void updateCamera();

      bool useISPC{true};

      vec2i windowSize;
      VKLVolume volume{nullptr};

      Renderer *renderer;

      std::unique_ptr<Renderer> renderer_density_pathtracer;
      std::unique_ptr<Renderer> renderer_hit_iterator;
      std::unique_ptr<Renderer> renderer_ray_march_iterator;

      std::unique_ptr<ArcballCamera> arcballCamera;

      TransferFunction transferFunction;

      std::vector<float> isovalues{-1.f, 0.f, 1.f};
    };

  }  // namespace examples
}  // namespace openvkl
