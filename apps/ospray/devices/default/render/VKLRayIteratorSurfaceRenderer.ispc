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
#include "ospray/SDK/transferFunction/LinearTransferFunction.ih"

#include <openvkl/openvkl.isph>

struct VKLRayIteratorSurfaceRenderer
{
  uniform Renderer super;
  uniform VKLVolume volume;
  TransferFunction *uniform transferFunction;
  uniform int numIsovalues;
  float *uniform isovalues;
};

inline void integrate(uniform VKLRayIteratorSurfaceRenderer *uniform self,
                      varying vec3f &color,
                      varying float &alpha,
                      const uniform int accumID,
                      const Ray &ray,
                      const int32 pixel_x,
                      const int32 pixel_y)
{
  color = make_vec3f(self->super.bgColor);
  alpha = self->super.bgColor.w;

  // create volume ray iterator
  vkl_range1f tRange;
  tRange.lower = ray.t0;
  tRange.upper = ray.t;

  VKLSamplesMask samplesMask = vklNewSamplesMask(self->volume);

  // add in isovalues of interest
  vklSamplesMaskSetValues(samplesMask, self->numIsovalues, self->isovalues);

  vklCommit((VKLObject)samplesMask);

  uniform VKLRayIterator rayIterator;
  vklInitRayIteratorV(&rayIterator,
                      self->volume,
                      (varying vkl_vec3f *)&ray.org,
                      (varying vkl_vec3f *)&ray.dir,
                      &tRange,
                      samplesMask);

  // the current surface hit
  VKLHit hit;

  while (vklIterateSurfaceV(&rayIterator, &hit) && alpha < 0.99f) {
    const vec3f surfaceColor = self->transferFunction->getColorForValue(
        self->transferFunction, hit.sample);
    const float surfaceOpacity = 0.5f;

    color = color + (1.f - alpha) * surfaceColor;
    alpha = alpha + (1.f - alpha) * surfaceOpacity;
  }

  vklRelease((VKLObject)samplesMask);
}

void VKLRayIteratorSurfaceRenderer_renderSample(Renderer *uniform _self,
                                                FrameBuffer *uniform fb,
                                                World *uniform world,
                                                void *uniform perFrameData,
                                                varying ScreenSample &sample)
{
  uniform VKLRayIteratorSurfaceRenderer *uniform self =
      (uniform VKLRayIteratorSurfaceRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform VKLRayIteratorSurfaceRenderer_create(void *uniform cppE)
{
  uniform VKLRayIteratorSurfaceRenderer *uniform self =
      uniform new uniform VKLRayIteratorSurfaceRenderer;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = VKLRayIteratorSurfaceRenderer_renderSample;
  return self;
}

export void VKLRayIteratorSurfaceRenderer_set(void *uniform _self,
                                              uniform VKLVolume volume,
                                              void *uniform transferFunction,
                                              uniform int numIsovalues,
                                              float *uniform isovalues)
{
  uniform VKLRayIteratorSurfaceRenderer *uniform self =
      (uniform VKLRayIteratorSurfaceRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->numIsovalues     = numIsovalues;
  self->isovalues        = isovalues;
}
