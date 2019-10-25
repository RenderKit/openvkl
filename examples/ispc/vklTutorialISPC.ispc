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

#include <openvkl/openvkl.isph>

export void demo_ispc(void *uniform _volume)
{
  uniform VKLVolume volume = (uniform VKLVolume)_volume;

  // bounding box
  uniform vkl_box3f bbox = vklGetBoundingBox(volume);
  print("bounding box:\n");
  print("\tlower: % % %\n", bbox.lower.x, bbox.lower.y, bbox.lower.z);
  print("\tupper: % % %\n\n", bbox.upper.x, bbox.upper.y, bbox.upper.z);

  // sampling and gradients
  vkl_vec3f objectCoordinates;
  objectCoordinates.x = programIndex * 3 + 0;
  objectCoordinates.y = programIndex * 3 + 1;
  objectCoordinates.z = programIndex * 3 + 2;

  float sample   = vklComputeSampleV(volume, &objectCoordinates);
  vkl_vec3f grad = vklComputeGradientV(volume, &objectCoordinates);

  print("objectCoordinates:\n\tx: %\n\ty: %\n\tz: %\n\n",
        objectCoordinates.x,
        objectCoordinates.y,
        objectCoordinates.z);
  print("samples:\n\t%\n\n", sample);
  print("gradient:\n\tx: %\n\ty: %\n\tz: %\n\n", grad.x, grad.y, grad.z);

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
  vkl_vec3f rayOrigin = {0, 0, 0};
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

  // interval iteration
  uniform VKLIntervalIterator intervalIterator;
  vklInitIntervalIteratorV(&intervalIterator,
                           volume,
                           &rayOrigin,
                           &rayDirection,
                           &rayTRange,
                           selector);

  print("interval iterator for value ranges {% %} {% %}\n",
        ranges[0].lower,
        ranges[0].upper,
        ranges[1].lower,
        ranges[1].upper);

  VKLInterval interval;
  while (vklIterateIntervalV(&intervalIterator, &interval)) {
    print("\ttRange:\n\t\tlower: %\n\t\tupper: %\n",
          interval.tRange.lower,
          interval.tRange.upper);
    print("\tvalueRange:\n\t\tlower: %\n\t\tupper: %\n",
          interval.valueRange.lower,
          interval.valueRange.upper);
    print("\tnominalDeltaT:\n\t\t%\n\n", interval.nominalDeltaT);
  }

  // hit iteration
  uniform VKLHitIterator hitIterator;
  vklInitHitIteratorV(
      &hitIterator, volume, &rayOrigin, &rayDirection, &rayTRange, selector);

  print("hit iterator for values % %\n", values[0], values[1]);

  VKLHit hit;
  while (vklIterateHitV(&hitIterator, &hit)) {
    print("\tt: %\n\tsample %\n\n", hit.t, hit.sample);
  }

  vklRelease((VKLObject)selector);
}
