// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
                                 const uniform Scene* uniform scene,
                                 const uniform vec2i &fbDims,
                                 const uniform vec2i &rangeDims,
                                 const uniform vec2i &pixelOffset,
                                 const uniform int frameID,
                                 const uniform float accumScale,
                                 const uniform int _taskID)
{
  Renderer *uniform self = (Renderer * uniform) _self;

  const int pixelID = _taskID * programCount + programIndex;

  vec2i pixel = make_vec2i(pixelOffset.x + (pixelID % rangeDims.x),
                           pixelOffset.y + (pixelID / rangeDims.x));

  if (pixel.x >= fbDims.x)
    return;

  const vec2f screen = make_vec2f(pixel.x * rcp((uniform float)fbDims.x),
                                  pixel.y * rcp((uniform float)fbDims.y));

  Ray ray = Renderer_computeRay(self, screen);
  const vec3f color = self->renderPixel(self, scene, ray, pixel, frameID, fbDims.x);

  const int fbIndex = pixel.x + fbDims.x * pixel.y;
  float ar = self->accum_r[fbIndex];
  float ag = self->accum_g[fbIndex];
  float ab = self->accum_b[fbIndex];

  ar += color.x;
  ag += color.y;
  ab += color.z;

  self->accum_r[fbIndex] = ar;
  self->accum_g[fbIndex] = ag;
  self->accum_b[fbIndex] = ab;

  self->framebuffer[fbIndex] = make_vec3f(ar, ag, ab) * accumScale;

  // linear to sRGB color space conversion
  self->framebuffer[fbIndex] =
      make_vec3f(pow(self->framebuffer[fbIndex].x, 1.f / 2.2f),
                 pow(self->framebuffer[fbIndex].y, 1.f / 2.2f),
                 pow(self->framebuffer[fbIndex].z, 1.f / 2.2f));
}

export void Renderer_freeRenderer(void *uniform self)
{
  delete self;
}
