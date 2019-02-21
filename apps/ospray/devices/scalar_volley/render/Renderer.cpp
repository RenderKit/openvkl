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

#include "Renderer.h"
#include <ospray/ospcommon/tasking/parallel_foreach.h>

namespace ospray {
  namespace scalar_volley_device {

    void Renderer::commit()
    {
      auto camera   = getParamObject<Camera>("camera");
      currentCamera = camera.get();

      auto model   = getParamObject<Model>("model");
      currentModel = model.get();
    }

    void Renderer::renderFrame(FrameBuffer &frameBuffer)
    {
      currentFrameBuffer = &frameBuffer;
      currentFrameBuffer->beginFrame();

      auto &tiles = currentFrameBuffer->getTiles();

      tasking::parallel_foreach(
          tiles.begin(), tiles.end(), [&](Tile &t) { renderTile(t); });

      currentFrameBuffer->endFrame();
    }

    void Renderer::renderTile(Tile &tile)
    {
      for (int y = 0; y < tile.size.y; y++) {
        for (int x = 0; x < tile.size.x; x++) {
          size_t index = tile.indexOf(vec2i(x, y));

          // we'll render from black -> white based on array index
          float gray = float(index) / float(tile.size.x * tile.size.y);
          tile.colorBuffer[index] = vec4f(gray, gray, gray, 1.f);
        }
      }
    }

    vec3f Renderer::computeLighting(const vec3f &rayDirection,
                                    const vec3f &normal,
                                    const vec3f &color)
    {
      // light parameters
      const float Il = 1.f;
      const vec3f lightDirection{-1.f, -1.f, -1.f};
      const vec3f Kd = color;
      const vec3f Ka = 0.1f * Kd;
      const vec3f Ks{0.5f, 0.5f, 0.5f};
      const float Ns = 10.f;

      // two-sided lighting
      float cosNL = fabs(dot(lightDirection, normal));

      const vec3f H     = normalize(lightDirection - rayDirection);
      const float cosNH = dot(H, normal);

      return Ka + Il * (Kd * cosNL + Ks * powf(cosNH, Ns));
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
