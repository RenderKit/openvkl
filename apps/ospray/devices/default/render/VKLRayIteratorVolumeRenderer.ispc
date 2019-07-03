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

struct VKLRayIteratorVolumeRenderer
{
  uniform Renderer super;
  uniform VKLVolume volume;
  TransferFunction *uniform transferFunction;
  uniform float samplingRate;
};

inline void integrate(uniform VKLRayIteratorVolumeRenderer *uniform self,
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

  uniform VKLRayIterator rayIterator;
  vklInitRayIteratorV(&rayIterator,
                      self->volume,
                      (varying vkl_vec3f *)&ray.org,
                      (varying vkl_vec3f *)&ray.dir,
                      &tRange,
                      samplesMask);

  // the current ray interval
  VKLRayInterval rayInterval;

  while (vklIterateIntervalV(&rayIterator, &rayInterval) && alpha < 0.99f) {
    const float nominalSamplingDt =
        rayInterval.nominalDeltaT / self->samplingRate;

    // initial sub interval, based on our renderer-defined sampling rate
    // and the volume's nominal dt
    box1f subInterval =
        make_box1f(rayInterval.tRange.lower,
                   min(rayInterval.tRange.lower + nominalSamplingDt,
                       rayInterval.tRange.upper));

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
          min(subInterval.lower + nominalSamplingDt, rayInterval.tRange.upper);
    }
  }

  vklRelease((VKLObject)samplesMask);
}

void VKLRayIteratorVolumeRenderer_renderSample(Renderer *uniform _self,
                                               FrameBuffer *uniform fb,
                                               World *uniform world,
                                               void *uniform perFrameData,
                                               varying ScreenSample &sample)
{
  uniform VKLRayIteratorVolumeRenderer *uniform self =
      (uniform VKLRayIteratorVolumeRenderer * uniform) _self;

  const uniform int accumID = reduce_max(sample.sampleID.z);

  integrate(self,
            sample.rgb,
            sample.alpha,
            accumID,
            sample.ray,
            sample.sampleID.x,
            sample.sampleID.y);
}

export void *uniform VKLRayIteratorVolumeRenderer_create(void *uniform cppE)
{
  uniform VKLRayIteratorVolumeRenderer *uniform self =
      uniform new uniform VKLRayIteratorVolumeRenderer;
  Renderer_Constructor(&self->super, cppE, 1);
  self->super.renderSample = VKLRayIteratorVolumeRenderer_renderSample;
  return self;
}

export void VKLRayIteratorVolumeRenderer_set(void *uniform _self,
                                             uniform VKLVolume volume,
                                             void *uniform transferFunction,
                                             uniform float samplingRate)
{
  uniform VKLRayIteratorVolumeRenderer *uniform self =
      (uniform VKLRayIteratorVolumeRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->samplingRate     = samplingRate;
}
