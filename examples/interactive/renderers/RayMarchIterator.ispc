// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "Renderer.ih"

struct RayMarchIterator
{
  Renderer super;
  float samplingRate;
};

vec3f RayMarchIterator_renderPixel(uniform Renderer *uniform _self,
                                   Ray &ray,
                                   const vec2i &pixel,
                                   const uniform int &frameID,
                                   const uniform int &fbWidth)
{
  RayMarchIterator *uniform self = (RayMarchIterator * uniform) _self;

  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // create volume iterator
  vkl_range1f tRange;
  tRange.lower = ray.tnear;
  tRange.upper = ray.tfar;

  uniform VKLIntervalIterator iterator;
  vklInitIntervalIteratorV(&iterator,
                           self->super.volume,
                           (varying vkl_vec3f *)&ray.org,
                           (varying vkl_vec3f *)&ray.dir,
                           &tRange,
                           self->super.samplesMask);

  // the current ray interval
  VKLInterval interval;

  while (vklIterateIntervalV(&iterator, &interval) && alpha < 0.99f) {
    const float nominalSamplingDt = interval.nominalDeltaT / self->samplingRate;

    // initial sub interval, based on our renderer-defined sampling rate
    // and the volume's nominal dt
    box1f subInterval = make_box1f(
        interval.tRange.lower,
        min(interval.tRange.lower + nominalSamplingDt, interval.tRange.upper));

    // integrate as long as we have valid sub intervals and are not
    // fully opaque
    while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
      const float t  = 0.5f * (subInterval.lower + subInterval.upper);
      const float dt = subInterval.upper - subInterval.lower;

      // get volume sample
      vec3f c = ray.org + t * ray.dir;
      float sample =
          vklComputeSampleV(self->super.volume, (varying vkl_vec3f *)&c);

      // map through transfer function
      vec4f sampleColorAndOpacity =
          Renderer_sampleTransferFunction(&self->super, sample);

      // accumulate contribution
      const float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

      sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

      color = color + (1.f - alpha) * make_vec3f(sampleColorAndOpacity);
      alpha = alpha + (1.f - alpha) * clampedOpacity;

      // compute next sub interval
      subInterval.lower = subInterval.upper;
      subInterval.upper =
          min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
    }
  }

  return color;
}

export void *uniform RayMarchIterator_create(void *uniform _volume)
{
  RayMarchIterator *uniform self = uniform new RayMarchIterator;

  VKLVolume volume = (VKLVolume)_volume;
  Renderer_constructor(&self->super, volume);

  self->super.renderPixel = RayMarchIterator_renderPixel;
  self->samplingRate      = 1.f;

  return self;
}

export void *uniform RayMarchIterator_set(void *uniform _self,
                                          const uniform float samplingRate)
{
  RayMarchIterator *uniform self = (RayMarchIterator * uniform) _self;

  self->samplingRate = samplingRate;
}
