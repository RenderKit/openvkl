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

#include "common/Model.ih"
#include "common/Ray.ih"
#include "render/Renderer.ih"

#include <volley/volley.isph>

struct SimpleVolleyRenderer
{
  uniform Renderer super;
  uniform VLYVolume volume;
  TransferFunction *uniform transferFunction;
};

inline void integrate(uniform SimpleVolleyRenderer *uniform self,
                      varying vec3f &color,
                      varying float &alpha,
                      const uniform int accumID,
                      const Ray &ray,
                      const int32 pixel_x,
                      const int32 pixel_y)
{
  color = make_vec3f(self->super.bgColor);
  alpha = self->super.bgColor.w;

  const uniform vly_box3f boundingBox = vlyGetBoundingBox(self->volume);

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
    float sample = vlyComputeSampleV(self->volume, (varying vly_vec3f *)&c);

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

void SimpleVolleyRenderer_renderSample(uniform Renderer *uniform _self,
                                 void *uniform perFrameData,
                                 varying ScreenSample &sample)
{
  uniform SimpleVolleyRenderer *uniform self =
      (uniform SimpleVolleyRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform SimpleVolleyRenderer_create(void *uniform cppE)
{
  uniform SimpleVolleyRenderer *uniform self = uniform new uniform SimpleVolleyRenderer;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = SimpleVolleyRenderer_renderSample;
  return self;
}

export void SimpleVolleyRenderer_set(void *uniform _self,
                               uniform VLYVolume volume,
                               void *uniform transferFunction)
{
  uniform SimpleVolleyRenderer *uniform self =
      (uniform SimpleVolleyRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
}
