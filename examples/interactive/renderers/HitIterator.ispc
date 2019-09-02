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

struct HitIterator
{
  Renderer super;

  float value1;
  float value2;
  float value3;
};

vec3f HitIterator_renderPixel(uniform Renderer *uniform,
                              uniform VKLVolume volume,
                              const uniform box3f &,
                              uniform VKLSamplesMask samplesMask,
                              Ray &ray,
                              const vec2i &,
                              const uniform int &,
                              const uniform int &)
{
  vec3f color = make_vec3f(0.f);
  float alpha = 0.f;

  // create volume iterator
  vkl_range1f tRange;
  tRange.lower = ray.tnear;
  tRange.upper = ray.tfar;

  uniform VKLHitIterator iterator;
  vklInitHitIteratorV(&iterator,
                      volume,
                      (varying vkl_vec3f *)&ray.org,
                      (varying vkl_vec3f *)&ray.dir,
                      &tRange,
                      samplesMask);

  // the current surface hit
  VKLHit hit;

  int numHits = 0;

  while (vklIterateHitV(&iterator, &hit) && alpha < 0.99f && numHits < 3) {
    vec3f surfaceColor;

    if (numHits < 1)
      surfaceColor = make_vec3f(1.f, 0.f, 0.f);
    else if (numHits < 2)
      surfaceColor = make_vec3f(0.f, 1.f, 0.f);
    else
      surfaceColor = make_vec3f(0.f, 0.f, 1.f);

    color = color + (1.f - alpha) * surfaceColor;
    alpha = alpha + (1.f - alpha) * 0.25f;

    numHits++;
  }

  return color;
}

export void *uniform HitIterator_create()
{
  HitIterator *uniform self = uniform new HitIterator;
  self->super.renderPixel   = HitIterator_renderPixel;
  return self;
}
