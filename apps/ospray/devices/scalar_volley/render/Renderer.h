// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "../camera/Camera.h"
#include "../common/Model.h"
#include "../fb/FrameBuffer.h"

namespace ospray {
  namespace scalar_volley_device {

    struct Renderer : public ManagedObject
    {
      Renderer() = default;

      virtual void commit() override;

      virtual void renderFrame(FrameBuffer &frameBuffer);

      // this should be overridden by actual renderer implementations
      virtual void renderTile(Tile &tile);

      static vec3f computeLighting(const vec3f &rayDirection,
                                   const vec3f &normal,
                                   const vec3f &color);

      vec4f backgroundColor{0.f};

     protected:
      Camera *currentCamera{nullptr};
      Model *currentModel{nullptr};
      FrameBuffer *currentFrameBuffer{nullptr};
    };

  }  // namespace scalar_volley_device
}  // namespace ospray
