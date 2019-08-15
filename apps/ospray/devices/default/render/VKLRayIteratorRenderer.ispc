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
#include "samples_mask_util.ih"

#include <openvkl/openvkl.isph>

struct VKLRayIteratorRenderer
{
  uniform Renderer super;
  uniform VKLVolume volume;
  TransferFunction *uniform transferFunction;
  uniform float samplingRate;
  uniform int numIsovalues;
  float *uniform isovalues;
};

inline void integrateVolumeInterval(uniform VKLRayIteratorRenderer *uniform
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
    float sample = vklComputeSampleV(self->volume, (varying vkl_vec3f *)&c);

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

inline void integrate(uniform VKLRayIteratorRenderer *uniform self,
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

  VKLSamplesMask samplesMask =
      getSamplesMaskForTransferFunction(self->volume, self->transferFunction);

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

  // the current ray interval
  VKLInterval interval;
  bool activeInterval = vklIterateIntervalV(&rayIterator, &interval);

  // the current surface hit
  VKLHit hit;
  bool activeSurface = vklIterateSurfaceV(&rayIterator, &hit);

  while ((activeInterval || activeSurface) && alpha < 0.99f) {
    float firstHit = min(activeInterval ? interval.tRange.lower : inf,
                         activeSurface ? hit.t : inf);

    if (activeInterval && firstHit == interval.tRange.lower) {
      // volume interval
      const float nominalSamplingDt =
          interval.nominalDeltaT / self->samplingRate;

      box1f subInterval =
          make_box1f(interval.tRange.lower,
                     min(interval.tRange.upper, activeSurface ? hit.t : inf));

      integrateVolumeInterval(
          self, ray, subInterval, nominalSamplingDt, color, alpha);

      if (subInterval.upper == interval.tRange.upper) {
        activeInterval = vklIterateIntervalV(&rayIterator, &interval);
      } else {
        interval.tRange.lower = subInterval.upper + 1e-6f;
      }

    } else if (activeSurface && firstHit == hit.t) {
      // surface hit
      const vec3f surfaceColor = self->transferFunction->getColorForValue(
          self->transferFunction, hit.sample);
      const float surfaceOpacity = 0.5f;

      color = color + (1.f - alpha) * surfaceColor;
      alpha = alpha + (1.f - alpha) * surfaceOpacity;

      activeSurface = vklIterateSurfaceV(&rayIterator, &hit);
    }
  }

  vklRelease((VKLObject)samplesMask);
}

void VKLRayIteratorRenderer_renderSample(Renderer *uniform _self,
                                         FrameBuffer *uniform fb,
                                         World *uniform world,
                                         void *uniform perFrameData,
                                         varying ScreenSample &sample)
{
  uniform VKLRayIteratorRenderer *uniform self =
      (uniform VKLRayIteratorRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform VKLRayIteratorRenderer_create(void *uniform cppE)
{
  uniform VKLRayIteratorRenderer *uniform self =
      uniform new uniform VKLRayIteratorRenderer;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = VKLRayIteratorRenderer_renderSample;
  return self;
}

export void VKLRayIteratorRenderer_set(void *uniform _self,
                                       uniform VKLVolume volume,
                                       void *uniform transferFunction,
                                       uniform float samplingRate,
                                       uniform int numIsovalues,
                                       float *uniform isovalues)
{
  uniform VKLRayIteratorRenderer *uniform self =
      (uniform VKLRayIteratorRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->samplingRate     = samplingRate;
  self->numIsovalues     = numIsovalues;
  self->isovalues        = isovalues;
}
