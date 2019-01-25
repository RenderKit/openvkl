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

#include "VolumeRendererVolleyIntegration.h"
#include "../volume/VolleyVolumeWrapper.h"

// ospcommon
#include <ospray/ospcommon/tasking/parallel_for.h>

namespace ospray {
  namespace scalar_volley_device {

    struct RayUserData
    {
      const VolleyVolumeWrapper *volleyVolumeWrapper;

      struct PixelData
      {
        size_t index;
        vec4f color;
      };

      std::vector<PixelData> pixelData;

      RayUserData(const VolleyVolumeWrapper *volleyVolumeWrapper,
                  const Tile &tile,
                  const vec4f &backgroundColor)
          : volleyVolumeWrapper(volleyVolumeWrapper)
      {
        pixelData.resize(tile.size.x * tile.size.y);

        for (int y = 0; y < tile.size.y; y++) {
          for (int x = 0; x < tile.size.x; x++) {
            const size_t index = tile.indexOf(vec2i{x, y});

            pixelData[index].index = index;
            pixelData[index].color = backgroundColor;
          }
        }
      }
    };

    void integrationStepFunction(size_t numValues,
                                 const vly_vec3f *worldCoordinates,
                                 const vly_vec3f *directions,
                                 const float *samples,
                                 const vly_vec3f *gradients,
                                 void *_rayUserData,
                                 bool *earlyTerminationMask)
    {
      RayUserData *rayUserData = (RayUserData *)_rayUserData;

      for (size_t i = 0; i < numValues; i++) {
        if (earlyTerminationMask[i]) {
          continue;
        }

        // apply transfer function
        vec4f sampleColor =
            rayUserData->volleyVolumeWrapper->getTransferFunction()
                .getColorAndOpacity(samples[i]);

        // compute lighting if we have gradient information; reference:
        // http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch39.html
        if (gradients) {
          const vec3f direction =
              *reinterpret_cast<const vec3f *>(&directions[i]);

          const vec3f gradient =
              safe_normalize((*reinterpret_cast<const vec3f *>(&gradients[i])));

          // light parameters
          const float Il = 1.f;
          const vec3f lightDirection{-1.f, -1.f, -1.f};
          const vec3f Kd{sampleColor.x, sampleColor.y, sampleColor.z};
          const vec3f Ka = 0.1f * Kd;
          const vec3f Ks{0.5f, 0.5f, 0.5f};
          const float Ns = 10.f;

          // two-sided lighting
          float cosNL = fabs(dot(lightDirection, gradient));

          const vec3f H     = normalize(lightDirection - direction);
          const float cosNH = dot(H, gradient);

          const vec3f litColor = Ka + Il * (Kd * cosNL + Ks * powf(cosNH, Ns));

          sampleColor =
              vec4f(litColor.x, litColor.y, litColor.z, sampleColor.w);
        }

        // accumulate contribution
        const float clampedOpacity =
            clamp(sampleColor.w /
                  rayUserData->volleyVolumeWrapper->getSamplingRate());

        sampleColor *= clampedOpacity;
        sampleColor.w = clampedOpacity;

        rayUserData->pixelData[i].color +=
            (1.f - rayUserData->pixelData[i].color.w) * sampleColor;

        // early termination
        if (rayUserData->pixelData[i].color.w >= 0.99f) {
          earlyTerminationMask[i] = true;
        }
      }
    }

    void VolumeRendererVolleyIntegration::renderTile(Tile &tile)
    {
      VolleyVolumeWrapper *volleyVolumeWrapper =
          dynamic_cast<VolleyVolumeWrapper *>(volume);

      if (!volleyVolumeWrapper) {
        throw std::runtime_error(
            "only Volley-based volumes supported in this renderer");
      }

      VLYIntegrator vlyIntegrator = volleyVolumeWrapper->getVLYIntegrator();
      VLYVolume vlyVolume         = volleyVolumeWrapper->getVLYVolume();

      // generate initial ray user data state that will be sent to Volley
      RayUserData rayUserData(volleyVolumeWrapper, tile, backgroundColor);

      // generate initial rays to be sent to Volley
      VolleyRays volleyRays = getCameraRays(tile);

      // intersect Volley volume, which will return ray t ranges
      vlyIntersectVolume(vlyVolume,
                         volleyRays.numRays,
                         (const vly_vec3f *)volleyRays.origins.data(),
                         (const vly_vec3f *)volleyRays.directions.data(),
                         (vly_range1f *)volleyRays.ranges.data());

      // integrate over the Volley volume over the full ray t ranges
      vlyIntegrateVolume(vlyIntegrator,
                         vlyVolume,
                         volleyRays.numRays,
                         (const vly_vec3f *)volleyRays.origins.data(),
                         (const vly_vec3f *)volleyRays.directions.data(),
                         (const vly_range1f *)volleyRays.ranges.data(),
                         &rayUserData,
                         &integrationStepFunction);

      // save final integrated color values to tile
      std::for_each(rayUserData.pixelData.begin(),
                    rayUserData.pixelData.end(),
                    [&](RayUserData::PixelData &pixelData) {
                      tile.colorBuffer[pixelData.index] = pixelData.color;
                    });
    }

    VolleyRays VolumeRendererVolleyIntegration::getCameraRays(const Tile &tile)
    {
      VolleyRays volleyRays(tile.size.x * tile.size.y);

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

          const size_t index = tile.indexOf(vec2i{x, y});

          volleyRays.origins[index]    = ray.org;
          volleyRays.directions[index] = ray.dir;
          volleyRays.ranges[index].x   = 0.f;
          volleyRays.ranges[index].y   = ospcommon::inf;
        }
      }

      return volleyRays;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
