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

#include "Renderer.h"
// std
#include <algorithm>
// ospcommon
#include "ospcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace examples {

    void Renderer::commit()
    {
      spp = getParam<int>("spp", 1);
    }

    void Renderer::setCamera(const vec3f &pos,
                             const vec3f &dir,
                             const vec3f &up,
                             float aspect,
                             float fovy)
    {
      camPos = pos;

      dir_du = normalize(cross(dir, up));
      dir_dv = cross(dir_du, dir);

      float imgPlane_size_y = 2.f * tanf(fovy / 2.f * M_PI / 180.);
      float imgPlane_size_x = imgPlane_size_y * aspect;

      dir_du *= imgPlane_size_x;
      dir_dv *= imgPlane_size_y;

      dir_00 = dir - .5f * dir_du - .5f * dir_dv;
    }

    void Renderer::setFrameSize(const vec2i &dims)
    {
      pixelIndices = index_sequence_2D(dims);

      const auto numPixels = pixelIndices.total_indices();
      framebuffer.resize(numPixels);
      accumulation.resize(numPixels);
    }

    vec2i Renderer::frameSize() const
    {
      return pixelIndices.dimensions();
    }

    void Renderer::resetAccumulation()
    {
      std::fill(accumulation.begin(), accumulation.end(), vec3f(0));
      frameID = 0;
    }

    const FrameBuffer &Renderer::frameBuffer() const
    {
      return framebuffer;
    }

    void Renderer::renderFrame(VKLVolume volume, VKLSamplesMask mask)
    {
      auto fbDims = pixelIndices.dimensions();

      const auto vkl_bounds    = vklGetBoundingBox(volume);
      const box3f volumeBounds = (const box3f &)vkl_bounds;

      for (int i = 0; i < spp; ++i) {
        float accumScale = 1.f / frameID;

        tasking::parallel_for(pixelIndices.total_indices(), [&](size_t i) {
          auto pixel = pixelIndices.reshape(i);

          vec2f screen(pixel.x * rcp(float(fbDims.x)),
                       pixel.y * rcp(float(fbDims.y)));

          Ray ray = computeRay(screen);

          vec3f color = renderPixel(volume,
                                    volumeBounds,
                                    mask,
                                    ray,
                                    vec4i(pixel.x, pixel.y, frameID, fbDims.x));

          auto index = pixelIndices.flatten(pixel);
          accumulation[index] += color;
          framebuffer[index] = accumulation[index] * accumScale;
        });

        frameID++;
      }
    }

  }  // namespace examples
}  // namespace openvkl
