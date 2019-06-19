// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "ospray/SDK/common/Ray.ih"
#include "ospray/SDK/common/World.ih"
#include "ospray/SDK/render/Renderer.ih"

struct SimpleNativeVolumeRenderer
{
  uniform Renderer super;
  Volume *uniform volume;
  TransferFunction *uniform transferFunction;
};

inline void integrate(uniform SimpleNativeVolumeRenderer *uniform self,
                      varying vec3f &color,
                      varying float &alpha,
                      const uniform int accumID,
                      const Ray &ray,
                      const int32 pixel_x,
                      const int32 pixel_y)
{
  color = make_vec3f(self->super.bgColor);
  alpha = self->super.bgColor.w;

  float tBox0, tBox1;
  intersectBox(ray, self->volume->boundingBox, tBox0, tBox1);

  if (tBox0 > tBox1)
    return;

  uniform int nSamples = 32;

  const varying float dt = (tBox1 - tBox0) / nSamples;

  for (uniform int i = 0; i < nSamples; i++) {
    float t = tBox0 + i * dt;

    vec3f c      = ray.org + t * ray.dir;
    float sample = self->volume->sample(self->volume, c);

    vec3f sampleColor = self->transferFunction->getColorForValue(
        self->transferFunction, sample);

    const float sampleOpacity = self->transferFunction->getOpacityForValue(
        self->transferFunction, sample);

    const float clampedOpacity = clamp(sampleOpacity * dt);

    sampleColor = sampleColor * clampedOpacity;

    color = color + (1.f - alpha) * make_vec3f(sampleColor);
    alpha = alpha + (1.f - alpha) * clampedOpacity;
  }
}

void SimpleNativeVolumeRenderer_renderSample(Renderer *uniform _self,
                                             FrameBuffer *uniform fb,
                                             World *uniform world,
                                             void *uniform perFrameData,
                                             varying ScreenSample &sample)
{
  uniform SimpleNativeVolumeRenderer *uniform self =
      (uniform SimpleNativeVolumeRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform SimpleNativeVolumeRenderer_create(void *uniform cppE)
{
  uniform SimpleNativeVolumeRenderer *uniform self =
      uniform new uniform SimpleNativeVolumeRenderer;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = SimpleNativeVolumeRenderer_renderSample;
  return self;
}

export void SimpleNativeVolumeRenderer_set(void *uniform _self,
                                           void *uniform volume,
                                           void *uniform transferFunction)
{
  uniform SimpleNativeVolumeRenderer *uniform self =
      (uniform SimpleNativeVolumeRenderer * uniform) _self;
  self->volume           = (Volume * uniform) volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
}
