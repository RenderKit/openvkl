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

struct IntersectBounds
{
  Renderer super;
};

vec3f IntersectBounds_renderPixel(uniform Renderer *uniform self,
                                  uniform VKLVolume volume,
                                  const uniform box3f &volumeBounds,
                                  uniform VKLSamplesMask,
                                  Ray &ray,
                                  const vec2i &pixel,
                                  const uniform int &frameID,
                                  const uniform int &fbWidth)
{
  float tBox0, tBox1;
  const box3f *uniform bb =
      (const uniform struct box3f *uniform) & volumeBounds;
  intersectBox(ray, *bb, tBox0, tBox1);

  if (tBox0 > tBox1)
    return make_vec3f(0.f);
  else
    return make_vec3f(0.f, 0.f, 1.f);
}

export void *uniform IntersectBounds_create()
{
  IntersectBounds *uniform self = uniform new IntersectBounds;
  self->super.renderPixel       = IntersectBounds_renderPixel;
  return self;
}