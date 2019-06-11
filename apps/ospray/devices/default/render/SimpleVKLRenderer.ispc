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

#include "ospray/SDK/common/Model.ih"
#include "ospray/SDK/common/Ray.ih"
#include "ospray/SDK/render/Renderer.ih"

#include <openvkl/openvkl.isph>

struct SimpleVKLRenderer
{
  uniform Renderer super;
  uniform VKLVolume volume;
  TransferFunction *uniform transferFunction;
};

inline void integrate(uniform SimpleVKLRenderer *uniform self,
                      varying vec3f &color,
                      varying float &alpha,
                      const uniform int accumID,
                      const Ray &ray,
                      const int32 pixel_x,
                      const int32 pixel_y)
{
  color = make_vec3f(self->super.bgColor);
  alpha = self->super.bgColor.w;

  const uniform vkl_box3f boundingBox = vklGetBoundingBox(self->volume);

  float tBox0, tBox1;
  const box3f *uniform bb = (const uniform struct box3f *uniform) & boundingBox;
  intersectBox(ray, *bb, tBox0, tBox1);

  if (tBox0 > tBox1)
    return;

  uniform int nSamples = 32;

  const varying float dt = (tBox1 - tBox0) / nSamples;

  for (uniform int i = 0; i < nSamples; i++) {
    float t = tBox0 + i * dt;

    vec3f c      = ray.org + t * ray.dir;
    float sample = vklComputeSampleV(self->volume, (varying vkl_vec3f *)&c);

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

void SimpleVKLRenderer_renderSample(uniform Renderer *uniform _self,
                                 void *uniform perFrameData,
                                 varying ScreenSample &sample)
{
  uniform SimpleVKLRenderer *uniform self =
      (uniform SimpleVKLRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform SimpleVKLRenderer_create(void *uniform cppE)
{
  uniform SimpleVKLRenderer *uniform self = uniform new uniform SimpleVKLRenderer;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = SimpleVKLRenderer_renderSample;
  return self;
}

export void SimpleVKLRenderer_set(void *uniform _self,
                               uniform VKLVolume volume,
                               void *uniform transferFunction)
{
  uniform SimpleVKLRenderer *uniform self =
      (uniform SimpleVKLRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
}
