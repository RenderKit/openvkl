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
#include "transferFunction/LinearTransferFunction.ih"

#include <volley/volley_driver.isph>
#include <volley/volley_iterator.isph>
#include <volley/volley_volume.isph>

struct VolleyRayIteratorRenderer
{
  uniform Renderer super;
  uniform VLYVolume volume;
  TransferFunction *uniform transferFunction;
  uniform int maxNumIntervals;
};

// in the future, sample masks will likely be computed and stored directly with
// the volume and transfer function
inline VLYSamplesMask getSamplesMaskForTransferFunction(
    const uniform VLYVolume volume,
    const TransferFunction *uniform transferFunction)
{
  // assume linear transfer function is always used
  const LinearTransferFunction *uniform ltf =
      (const LinearTransferFunction *uniform)transferFunction;

  VLYSamplesMask samplesMask = vlyNewSamplesMask(volume);

  // allocate maximum number of needed ranges
  vly_range1f *uniform valueRanges =
      uniform new uniform vly_range1f[ltf->opacityValueCount - 1 + 2];

  uniform int numActiveRanges = 0;

  // -inf to min value
  if (ltf->opacityValues[0] > 0.f) {
    valueRanges[numActiveRanges].lower = -inf;
    valueRanges[numActiveRanges].upper = ltf->super.valueRange.x;

    numActiveRanges = numActiveRanges + 1;
  }

  // max value to inf
  if (ltf->opacityValues[ltf->opacityValueCount - 1] > 0.f) {
    valueRanges[numActiveRanges].lower = ltf->super.valueRange.y;
    valueRanges[numActiveRanges].upper = inf;

    numActiveRanges = numActiveRanges + 1;
  }

  // inner value ranges
  for (uniform int i = 0; i < ltf->opacityValueCount - 1; i++) {
    if (ltf->opacityValues[i] > 0.f || ltf->opacityValues[i + 1] > 0.f) {
      uniform float lowerValue =
          ltf->super.valueRange.x +
          (float)i / ((float)ltf->opacityValueCount - 1.f) *
              (ltf->super.valueRange.y - ltf->super.valueRange.x);

      uniform float upperValue =
          ltf->super.valueRange.x +
          (float)(i + 1) / ((float)ltf->opacityValueCount - 1.f) *
              (ltf->super.valueRange.y - ltf->super.valueRange.x);

      valueRanges[numActiveRanges].lower = lowerValue;
      valueRanges[numActiveRanges].upper = upperValue;

      numActiveRanges = numActiveRanges + 1;
    }
  }

  vlySamplesMaskSetRanges(samplesMask, numActiveRanges, valueRanges);

  vlyCommit((VLYObject)samplesMask);

  delete[] valueRanges;

  return samplesMask;
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

  VLYRayIterator rayIterator = vlyNewRayIteratorV(self->volume,
                                                  (varying vly_vec3f *)&ray.org,
                                                  (varying vly_vec3f *)&ray.dir,
                                                  &tRange,
                                                  samplesMask);

  // the current ray interval
  VLYRayInterval rayInterval;

  // TODO
  uniform float samplingRate = 1.f;

  uniform int currentInterval = 0;

  while (vlyIterateIntervalV(rayIterator, &rayInterval) && alpha < 0.99f) {
    currentInterval = currentInterval + 1;

    const float nominalSamplingDt = rayInterval.nominalDeltaT / samplingRate;

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
          min(subInterval.lower + nominalSamplingDt, rayInterval.tRange.upper);
    }

    if (currentInterval >= self->maxNumIntervals) {
      break;
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
                                          uniform int maxNumIntervals)
{
  uniform VolleyRayIteratorRenderer *uniform self =
      (uniform VolleyRayIteratorRenderer * uniform) _self;
  self->volume           = volume;
  self->transferFunction = (TransferFunction * uniform) transferFunction;
  self->maxNumIntervals  = maxNumIntervals;
}
