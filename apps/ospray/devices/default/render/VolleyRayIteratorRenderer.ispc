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
#include "samples_mask_util.ih"
#include "transferFunction/LinearTransferFunction.ih"

#include <volley/volley.isph>

struct VolleyRayIteratorRenderer
{
  uniform Renderer super;
  uniform VLYVolume volume;
  TransferFunction *uniform transferFunction;
  uniform float samplingRate;
  uniform int numIsovalues;
  float *uniform isovalues;
};

inline void integrateVolumeInterval(uniform VolleyRayIteratorRenderer *uniform
                                        self,
                                    const Ray &ray,
                                    box1f tRange,
                                    float nominalSamplingDt,
                                    vec3f &color,
                                    float &alpha)
{
  // initial sub interval, based on our renderer-defined sampling rate
  // and the volume's nominal dt
  box1f subInterval = make_box1f(
      tRange.lower, min(tRange.lower + nominalSamplingDt, tRange.upper));

  // integrate as long as we have valid sub intervals and are not
  // fully opaque
  while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
    const float t  = 0.5f * (subInterval.lower + subInterval.upper);
    const float dt = subInterval.upper - subInterval.lower;

    // get volume sample
    vec3f c      = ray.org + t * ray.dir;
    float sample = vlyComputeSampleV(self->volume, (varying vly_vec3f *)&c);

    // apply transfer function
    vec3f sampleColor = self->transferFunction->getColorForValue(
        self->transferFunction, sample);

    const float sampleOpacity = self->transferFunction->getOpacityForValue(
        self->transferFunction, sample);

    // accumulate contribution
    const float clampedOpacity = clamp(sampleOpacity * dt);

    sampleColor = sampleColor * clampedOpacity;

    color = color + (1.f - alpha) * sampleColor;
    alpha = alpha + (1.f - alpha) * clampedOpacity;

    // compute next sub interval
    subInterval.lower = subInterval.upper;
    subInterval.upper =
        min(subInterval.lower + nominalSamplingDt, tRange.upper);
  }
}

inline void integrate(uniform VolleyRayIteratorRenderer *uniform self,
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

  VLYSamplesMask samplesMask =
      getSamplesMaskForTransferFunction(self->volume, self->transferFunction);

  // add in isovalues of interest
  vlySamplesMaskSetValues(samplesMask, self->numIsovalues, self->isovalues);

  vlyCommit((VLYObject)samplesMask);

  VLYRayIterator rayIterator = vlyNewRayIteratorV(self->volume,
                                                  (varying vly_vec3f *)&ray.org,
                                                  (varying vly_vec3f *)&ray.dir,
                                                  &tRange,
                                                  samplesMask);

  // the current ray interval
  VLYRayInterval rayInterval;
  bool activeInterval = vlyIterateIntervalV(rayIterator, &rayInterval);

  // the current surface hit
  VLYSurfaceHit surfaceHit;
  bool activeSurface = vlyIterateSurfaceV(rayIterator, &surfaceHit);

  while ((activeInterval || activeSurface) && alpha < 0.99f) {
    float firstHit = min(activeInterval ? rayInterval.tRange.lower : inf,
                         activeSurface ? surfaceHit.t : inf);

    if (activeInterval && firstHit == rayInterval.tRange.lower) {
      // volume interval
      const float nominalSamplingDt =
          rayInterval.nominalDeltaT / self->samplingRate;

      box1f subInterval = make_box1f(
          rayInterval.tRange.lower,
          min(rayInterval.tRange.upper, activeSurface ? surfaceHit.t : inf));

      integrateVolumeInterval(
          self, ray, subInterval, nominalSamplingDt, color, alpha);

      if (subInterval.upper == rayInterval.tRange.upper) {
        activeInterval = vlyIterateIntervalV(rayIterator, &rayInterval);
      } else {
        rayInterval.tRange.lower = subInterval.upper + 1e-6f;
      }

    } else if (activeSurface && firstHit == surfaceHit.t) {
      // surface hit
      const vec3f surfaceColor = self->transferFunction->getColorForValue(
          self->transferFunction, surfaceHit.sample);
      const float surfaceOpacity = 0.5f;

      color = color + (1.f - alpha) * surfaceColor;
      alpha = alpha + (1.f - alpha) * surfaceOpacity;

      activeSurface = vlyIterateSurfaceV(rayIterator, &surfaceHit);
    }
  }

  vlyRelease((VLYObject)samplesMask);
  vlyRelease((VLYObject)rayIterator);
}

void VolleyRayIteratorRenderer_renderSample(uniform Renderer *uniform _self,
                                            void *uniform perFrameData,
                                            varying ScreenSample &sample)
{
  uniform VolleyRayIteratorRenderer *uniform self =
      (uniform VolleyRayIteratorRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform VolleyRayIteratorRenderer_create(void *uniform cppE)
{
  uniform VolleyRayIteratorRenderer *uniform self =
      uniform new uniform VolleyRayIteratorRenderer;
  Renderer_Constructor(&self->super, cppE, NULL, NULL, 1);
  self->super.renderSample = VolleyRayIteratorRenderer_renderSample;
  return self;
}

export void VolleyRayIteratorRenderer_set(void *uniform _self,
                                          uniform VLYVolume volume,
                                          void *uniform transferFunction,
                                          uniform float samplingRate,
                                          uniform int numIsovalues,
                                          float *uniform isovalues)
{
  uniform VolleyRayIteratorRenderer *uniform self =
      (uniform VolleyRayIteratorRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->samplingRate     = samplingRate;
  self->numIsovalues     = numIsovalues;
  self->isovalues        = isovalues;
}
