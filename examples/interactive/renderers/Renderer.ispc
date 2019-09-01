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

export uniform int Renderer_pixelsPerJob()
{
  return programCount;
}

export void Renderer_setCamera(void *uniform _self,
                               const uniform vec3f &camPos,
                               const uniform vec3f &dir_00,
                               const uniform vec3f &dir_du,
                               const uniform vec3f &dir_dv)
{
  Renderer *uniform self = (Renderer * uniform) _self;

  self->camPos = camPos;
  self->dir_00 = dir_00;
  self->dir_du = dir_du;
  self->dir_dv = dir_dv;
}

export void Renderer_setFrameBuffer(void *uniform _self,
                                    vec3f *uniform framebuffer,
                                    float *uniform accum_r,
                                    float *uniform accum_g,
                                    float *uniform accum_b)
{
  Renderer *uniform self = (Renderer * uniform) _self;

  self->framebuffer = framebuffer;

  self->accum_r = accum_r;
  self->accum_g = accum_g;
  self->accum_b = accum_b;
}

export void Renderer_setFrameID(void *uniform _self, uniform int frameID)
{
  Renderer *uniform self = (Renderer * uniform) _self;

  self->frameID = frameID;
}

export void Renderer_renderPixel(void *uniform _self,
                                 void *uniform _volume,
                                 const uniform box3f &volumeBounds,
                                 void *uniform _mask,
                                 const uniform vec2i &fbDims,
                                 const uniform int frameID,
                                 const uniform float accumScale,
                                 const uniform int _taskID)
{
  Renderer *uniform self = (Renderer * uniform) _self;
  VKLVolume volume       = (VKLVolume)_volume;
  VKLSamplesMask mask    = (VKLSamplesMask)_mask;

  const int pixelID = _taskID * programCount + programIndex;

  vec2i pixel = make_vec2i(pixelID % fbDims.x, pixelID / fbDims.x);

  if (pixel.x >= fbDims.x)
    return;

  vec2f screen = make_vec2f(pixel.x * rcp((float)fbDims.x),
                            pixel.y * rcp((float)fbDims.y));

  Ray ray = Renderer_computeRay(self, screen);

  vec3f color = self->renderPixel(
      self, volume, volumeBounds, mask, ray, pixel, frameID, fbDims.x);

  float ar = self->accum_r[pixelID];
  float ag = self->accum_g[pixelID];
  float ab = self->accum_b[pixelID];

  ar += color.x;
  ag += color.y;
  ab += color.z;

  self->framebuffer[pixelID] = make_vec3f(ar, ag, ab) * accumScale;

  self->accum_r[pixelID] = ar;
  self->accum_g[pixelID] = ag;
  self->accum_b[pixelID] = ab;
}

export void Renderer_freeRenderer(void *uniform self)
{
  delete self;
}
