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

#include "../volume/VolleyVolumeWrapper.h"
#include "VolumeRendererDefault.h"

// ospcommon
#include <ospray/ospcommon/tasking/parallel_for.h>

namespace ospray {
  namespace scalar_volley_device {

    void VolumeRendererDefault::renderTile(Tile &tile)
    {
      auto &transferFunction = volume->getTransferFunction();

      tasking::parallel_for(tile.size.y, [&](int y) {
        for (int x = 0; x < tile.size.x; x++) {
          // generate camera sample in [0-1] screen space
          CameraSample cameraSample{vec2f{
              (tile.origin.x + x) * rcp(float(currentFrameBuffer->size().x)),
              (tile.origin.y + y) * rcp(float(currentFrameBuffer->size().y))}};

          // generate ray from camera sample
          Ray ray;
          currentCamera->getRay(cameraSample, ray);

          // the integrated volume value
          vec4f color{0.f, 0.f, 0.f, 0.f};

          // intersect ray with volume
          if (!volume || !volume->intersect(ray)) {
            tile.colorBuffer[tile.indexOf(vec2i{x, y})] = vec4f{0.f};
            continue;
          }

          while (ray.t0 <= ray.t) {
            // get volume sample
            const float sample =
                volume->computeSample(ray.org + ray.t0 * ray.dir);

            // apply transfer function
            vec4f sampleColor = transferFunction.getColorAndOpacity(sample);

            // accumulate contribution
            const float clampedOpacity =
                clamp(sampleColor.w / volume->getSamplingRate());

            sampleColor *= clampedOpacity;
            sampleColor.w = clampedOpacity;

            color += (1.f - color.w) * sampleColor;

            // early termination
            if (color.w >= 0.99f)
              break;

            // advance the ray for the next sample
            volume->advance(ray);
          }

          tile.colorBuffer[tile.indexOf(vec2i{x, y})] = color;
        }
      });
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
