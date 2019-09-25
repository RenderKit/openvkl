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

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const box3f &worldBounds, const vec2i &windowSize)
    : zoomSpeed(1),
      invWindowSize(vec2f(1.0) / vec2f(windowSize)),
      centerTranslation(one),
      translation(one),
      rotation(one)
{
  resetCamera(worldBounds);
}

void ArcballCamera::rotate(const vec2f &from, const vec2f &to)
{
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= zoomSpeed;
  translation = AffineSpace3f::translate(vec3f(0, 0, amount)) * translation;
  updateCamera();
}

void ArcballCamera::pan(const vec2f &delta)
{
  const vec3f t =
      vec3f(-delta.x * invWindowSize.x, delta.y * invWindowSize.y, 0);
  const vec3f worldt = translation.p.z * xfmVector(invCamera, t);
  centerTranslation  = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

vec3f ArcballCamera::eyePos() const
{
  return xfmPoint(invCamera, vec3f(0, 0, 1));
}

vec3f ArcballCamera::center() const
{
  return -centerTranslation.p;
}

vec3f ArcballCamera::lookDir() const
{
  return xfmVector(invCamera, vec3f(0, 0, 1));
}

vec3f ArcballCamera::upDir() const
{
  return xfmVector(invCamera, vec3f(0, 1, 0));
}

void ArcballCamera::resetCamera(const box3f &worldBounds)
{
  vec3f diag = worldBounds.size();
  zoomSpeed  = max(length(diag) / 150.0, 0.001);
  diag       = max(diag, vec3f(0.3f * length(diag)));

  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  translation       = AffineSpace3f::translate(vec3f(0, 0, length(diag)));
  rotation          = one;

  updateCamera();
}

void ArcballCamera::updateCamera()
{
  const AffineSpace3f rot    = LinearSpace3f(rotation);
  const AffineSpace3f camera = translation * rot * centerTranslation;
  invCamera                  = rcp(camera);
}

void ArcballCamera::updateWindowSize(const vec2i &windowSize)
{
  invWindowSize = vec2f(1) / vec2f(windowSize);
}

quaternionf ArcballCamera::screenToArcball(const vec2f &p)
{
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f) {
    return quaternionf(0, p.x, p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const vec2f unitDir = normalize(p);
    return quaternionf(0, unitDir.x, unitDir.y, 0);
  }
}
