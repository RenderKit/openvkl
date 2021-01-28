// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.isph>

export void demo_ispc(void *uniform _volume, void *uniform _sampler)
{
  uniform VKLVolume volume   = (uniform VKLVolume)_volume;
  uniform VKLSampler sampler = (uniform VKLSampler)_sampler;

  // bounding box
  uniform vkl_box3f bbox = vklGetBoundingBox(volume);
  print("bounding box:\n");
  print("\tlower: % % %\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  print("\tupper: % % %\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // coordinates for sampling and gradients
  vkl_vec3f objectCoordinates;
  objectCoordinates.x = programIndex * 3 + 0;
  objectCoordinates.y = programIndex * 3 + 1;
  objectCoordinates.z = programIndex * 3 + 2;

  print("objectCoordinates:\n\tx: %\n\ty: %\n\tz: %\n\n",
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

  // value selector setup (note the commit at the end)
  uniform vkl_range1f ranges[2] = {{10, 20}, {50, 75}};
  uniform int num_ranges        = 2;
  uniform float values[2]       = {32, 96};
  uniform int num_values        = 2;
  VKLValueSelector selector     = vklNewValueSelector(volume);
  vklValueSelectorSetRanges(selector, num_ranges, ranges);
  vklValueSelectorSetValues(selector, num_values, values);
  vklCommit((VKLObject)selector);

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

  // time value for hit iteration
  float time = 0.f;

  // interval iteration
  uniform unsigned int8 bufferInterval[VKL_MAX_INTERVAL_ITERATOR_SIZE];
  VKLIntervalIterator intervalIterator = vklInitIntervalIteratorV(
      sampler, &rayOrigin, &rayDirection, &rayTRange, selector, bufferInterval);

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
  uniform unsigned int8 bufferHit[VKL_MAX_HIT_ITERATOR_SIZE];
  VKLHitIterator hitIterator = vklInitHitIteratorV(sampler,
                                                   &rayOrigin,
                                                   &rayDirection,
                                                   &rayTRange,
                                                   &time,
                                                   selector,
                                                   bufferHit);

  print("hit iterator for values % %\n", values[0], values[1]);

  VKLHit hit;
  while (vklIterateHitV(hitIterator, &hit)) {
    print(
        "\tt: %\n\tsample %\n\tepsilon %\n\n", hit.t, hit.sample, hit.epsilon);
  }

  vklRelease((VKLObject)selector);
}
