// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.isph>
#include <openvkl/device/openvkl.isph>

export void demo_ispc(void *uniform _volume,
                      void *uniform _sampler,
                      void *uniform _intervalContext,
                      void *uniform _hitContext,
                      void *uniform _ranges,
                      void *uniform _values)
{
  VKLVolume *uniform volume   = (VKLVolume * uniform) _volume;
  VKLSampler *uniform sampler = (VKLSampler * uniform) _sampler;
  VKLIntervalIteratorContext *uniform intervalContext =
      (VKLIntervalIteratorContext * uniform) _intervalContext;
  VKLHitIteratorContext *uniform hitContext =
      (VKLHitIteratorContext * uniform) _hitContext;
  vkl_range1f *uniform ranges = (vkl_range1f * uniform) _ranges;
  float *uniform values       = (float *uniform)_values;

  // bounding box
  uniform vkl_box3f bbox = vklGetBoundingBox(*volume);
  print("bounding box:\n");
  print("\tlower: % % %\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  print("\tupper: % % %\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // number of attributes
  uniform unsigned int numAttributes = vklGetNumAttributes(*volume);
  print("num attributes = %\n\n", numAttributes);

  // value range for all attributes
  for (uniform unsigned int i = 0; i < numAttributes; i++) {
    uniform vkl_range1f valueRange = vklGetValueRange(*volume, i);
    print("value range (attribute %) = (% %)\n",
          i,
          valueRange.lower,
          valueRange.upper);
  }

  // coordinates for sampling and gradients
  vkl_vec3f objectCoordinates;
  objectCoordinates.x = programIndex * 3 + 0;
  objectCoordinates.y = programIndex * 3 + 1;
  objectCoordinates.z = programIndex * 3 + 2;

  print("\nobjectCoordinates:\n\tx: %\n\ty: %\n\tz: %\n\n",
        objectCoordinates.x,
        objectCoordinates.y,
        objectCoordinates.z);

  // sampling and gradients (first attribute)
  float sample   = vklComputeSampleV(sampler, &objectCoordinates);
  vkl_vec3f grad = vklComputeGradientV(sampler, &objectCoordinates);

  print("sampling and gradient computation (first attribute)\n");
  print("\tsamples: %\n\n", sample);
  print(
      "\tgradient:\n\t\tx: %\n\t\ty: %\n\t\tz: %\n\n", grad.x, grad.y, grad.z);

  // sampling (multiple attributes)
  uniform unsigned int M                  = 3;
  uniform unsigned int attributeIndices[] = {0, 1, 2};
  float samples[3];
  vklComputeSampleMV(sampler, &objectCoordinates, samples, M, attributeIndices);

  print("sampling (multiple attributes)\n");
  for (uniform unsigned int i = 0; i < M; i++)
    print("\tsamples[%]: %\n", i, samples[i]);
  print("\n");

  // ray definition for iterators
  vkl_vec3f rayOrigin = {0, 1, 1};
  vkl_vec3f rayDirection;
  rayDirection.x = 1;
  rayDirection.y = programIndex / 16.f;
  rayDirection.z = 0;

  // direction doesn't need to be normalized, doing it here for demo purposes
  float length =
      sqrt(rayDirection.x * rayDirection.x + rayDirection.y * rayDirection.y +
           rayDirection.z * rayDirection.z);
  rayDirection.x /= length;
  rayDirection.y /= length;
  rayDirection.z /= length;

  vkl_range1f rayTRange = {0, 200};
  print("rayOrigin:\n\tx: %\n\ty: %\n\tz: %\n",
        rayOrigin.x,
        rayOrigin.y,
        rayOrigin.z);
  print("rayDirection:\n\tx: %\n\ty: %\n\tz: %\n",
        rayDirection.x,
        rayDirection.y,
        rayDirection.z);
  print("rayTRange:\n\tlower: %\n\tupper: %\n\n",
        rayTRange.lower,
        rayTRange.upper);

  // time value for iteration
  float time = 0.f;

  // interval iteration
  void *uniform bufferInterval =
      alloca(vklGetIntervalIteratorSizeV(intervalContext));
  VKLIntervalIterator intervalIterator =
      vklInitIntervalIteratorV(intervalContext,
                               &rayOrigin,
                               &rayDirection,
                               &rayTRange,
                               &time,
                               bufferInterval);

  print("interval iterator for value ranges {% %} {% %}\n",
        ranges[0].lower,
        ranges[0].upper,
        ranges[1].lower,
        ranges[1].upper);

  VKLInterval interval;
  while (vklIterateIntervalV(intervalIterator, &interval)) {
    print("\ttRange:\n\t\tlower: %\n\t\tupper: %\n",
          interval.tRange.lower,
          interval.tRange.upper);
    print("\tvalueRange:\n\t\tlower: %\n\t\tupper: %\n",
          interval.valueRange.lower,
          interval.valueRange.upper);
    print("\tnominalDeltaT:\n\t\t%\n\n", interval.nominalDeltaT);
  }

  // hit iteration
  void *uniform bufferHit    = alloca(vklGetHitIteratorSizeV(hitContext));
  VKLHitIterator hitIterator = vklInitHitIteratorV(
      hitContext, &rayOrigin, &rayDirection, &rayTRange, &time, bufferHit);

  print("hit iterator for values % %\n", values[0], values[1]);

  VKLHit hit;
  while (vklIterateHitV(hitIterator, &hit)) {
    print(
        "\tt: %\n\tsample %\n\tepsilon %\n\n", hit.t, hit.sample, hit.epsilon);
  }
}
