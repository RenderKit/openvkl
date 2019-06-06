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
#include "ospray/SDK/transferFunction/LinearTransferFunction.ih"

#include <volley/volley.isph>

struct VolleyRayIteratorSurfaceRenderer
{
  uniform Renderer super;
  uniform VLYVolume volume;
  TransferFunction *uniform transferFunction;
  uniform int numIsovalues;
  float *uniform isovalues;
};

inline void integrate(uniform VolleyRayIteratorSurfaceRenderer *uniform self,
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
  vly_range1f tRange;
  tRange.lower = ray.t0;
  tRange.upper = ray.t;

  VLYSamplesMask samplesMask = vlyNewSamplesMask(self->volume);

  // add in isovalues of interest
  vlySamplesMaskSetValues(samplesMask, self->numIsovalues, self->isovalues);

  vlyCommit((VLYObject)samplesMask);

  uniform VLYRayIterator rayIterator = vlyNewRayIteratorV(self->volume,
                                                  (varying vly_vec3f *)&ray.org,
                                                  (varying vly_vec3f *)&ray.dir,
                                                  &tRange,
                                                  samplesMask);

  // the current surface hit
  VLYSurfaceHit surfaceHit;

  while (vlyIterateSurfaceV(&rayIterator, &surfaceHit) && alpha < 0.99f) {
    const vec3f surfaceColor = self->transferFunction->getColorForValue(
        self->transferFunction, surfaceHit.sample);
    const float surfaceOpacity = 0.5f;

    color = color + (1.f - alpha) * surfaceColor;
    alpha = alpha + (1.f - alpha) * surfaceOpacity;
  }

  vlyRelease((VLYObject)samplesMask);
}

void VolleyRayIteratorSurfaceRenderer_renderSample(uniform Renderer *uniform
                                                       _self,
                                                   void *uniform perFrameData,
                                                   varying ScreenSample &sample)
{
  uniform VolleyRayIteratorSurfaceRenderer *uniform self =
      (uniform VolleyRayIteratorSurfaceRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform VolleyRayIteratorSurfaceRenderer_create(void *uniform cppE)
{
  uniform VolleyRayIteratorSurfaceRenderer *uniform self =
      uniform new uniform VolleyRayIteratorSurfaceRenderer;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = VolleyRayIteratorSurfaceRenderer_renderSample;
  return self;
}

export void VolleyRayIteratorSurfaceRenderer_set(void *uniform _self,
                                                 uniform VLYVolume volume,
                                                 void *uniform transferFunction,
                                                 uniform int numIsovalues,
                                                 float *uniform isovalues)
{
  uniform VolleyRayIteratorSurfaceRenderer *uniform self =
      (uniform VolleyRayIteratorSurfaceRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->numIsovalues     = numIsovalues;
  self->isovalues        = isovalues;
}
