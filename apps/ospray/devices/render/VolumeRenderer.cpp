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

#include "VolumeRenderer.h"
// ospcommon
#include <ospray/ospcommon/tasking/parallel_for.h>

namespace ospray {
  namespace scalar_volley_device {

    void VolumeRenderer::commit()
    {
      Renderer::commit();
    }

    void VolumeRenderer::renderFrame(FrameBuffer &frameBuffer)
    {
      if (!currentModel->volumes.empty())
        volume = currentModel->volumes.front().get();

      Renderer::renderFrame(frameBuffer);
    }

    void VolumeRenderer::renderTile(Tile &tile)
    {
#if 0
      renderTileNormal(tile);
#else
      renderTileWide(tile);
#endif
    }

    void VolumeRenderer::renderTileNormal(Tile &tile)
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

    void VolumeRenderer::renderTileWide(Tile &tile)
    {
      auto &transferFunction = volume->getTransferFunction();

      struct TileSample
      {
        size_t index;
        Ray ray;

        TileSample() = default;
        TileSample(const size_t &index, const Ray &ray) : index(index), ray(ray)
        {
        }
      };

      // buffer for all _active_ tile samples
      std::vector<TileSample> tileSampleBuffer;

      // pre-allocate; we'll have at most a full tile's worth of samples
      tileSampleBuffer.reserve(tile.size.x * tile.size.y);

      // generate initial tile samples from camera
      for (int y = 0; y < tile.size.y; y++) {
        for (int x = 0; x < tile.size.x; x++) {
          // camera sample in [0-1] screen space
          CameraSample cameraSample{vec2f{
              (tile.origin.x + x) * rcp(float(currentFrameBuffer->size().x)),
              (tile.origin.y + y) * rcp(float(currentFrameBuffer->size().y))}};

          // ray from camera sample
          Ray ray;
          currentCamera->getRay(cameraSample, ray);

          // if the ray intersects the volume; add it to the tile sample buffer
          if (volume->intersect(ray)) {
            tileSampleBuffer.emplace_back(tile.indexOf(vec2i{x, y}), ray);
          }
        }
      }

      // initialize tile to background color
      tile.clear(backgroundColor);

      // as we iterate, we'll generate new requested tile samples as we advance
      // through the volume
      std::vector<TileSample> newTileSampleBuffer;
      newTileSampleBuffer.reserve(tile.size.x * tile.size.y);

      // pre-allocate storage for sample coordinates
      std::vector<vec3f> sampleCoordinates;
      sampleCoordinates.reserve(tile.size.x * tile.size.y);

      // as long as we have active tile samples...
      while (tileSampleBuffer.size()) {
        // generate sample coordinates
        sampleCoordinates.resize(0);

        std::for_each(
            tileSampleBuffer.begin(),
            tileSampleBuffer.end(),
            [&](const TileSample &tileSample) {
              sampleCoordinates.emplace_back(
                  tileSample.ray.org + tileSample.ray.t0 * tileSample.ray.dir);
            });

        // compute all volume samples
        std::vector<float> samples = volume->computeSamples(sampleCoordinates);

        // apply transfer function and accumulate contributions
        for (size_t s = 0; s < samples.size(); s++) {
          vec4f sampleColor = transferFunction.getColorAndOpacity(samples[s]);

          // accumulate contribution
          const float clampedOpacity =
              clamp(sampleColor.w / volume->getSamplingRate());

          sampleColor *= clampedOpacity;
          sampleColor.w = clampedOpacity;

          const size_t colorBufferIndex = tileSampleBuffer[s].index;

          tile.colorBuffer[colorBufferIndex] +=
              (1.f - tile.colorBuffer[colorBufferIndex].w) * sampleColor;
        }

        // generate tile samples for next iteration
        newTileSampleBuffer.resize(0);

        std::for_each(tileSampleBuffer.begin(),
                      tileSampleBuffer.end(),
                      [&](TileSample &tileSample) {
                        // early termination
                        if (tile.colorBuffer[tileSample.index].w >= 0.99f)
                          return;

                        // advance the ray for the next sample
                        volume->advance(tileSample.ray);

                        // add tile sample if the ray still intersects the
                        // volume
                        if (tileSample.ray.t0 <= tileSample.ray.t) {
                          newTileSampleBuffer.emplace_back(tileSample);
                        }
                      });

        // set new requested tile samples
        tileSampleBuffer = newTileSampleBuffer;
      }
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
