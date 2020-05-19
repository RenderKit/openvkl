// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/AffineSpace.h"

using namespace rkcommon::math;

class ArcballCamera
{
 public:
  ArcballCamera(const box3f &worldBounds, const vec2i &windowSize);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const vec2f &from, const vec2f &to);
  void zoom(float amount);
  void pan(const vec2f &delta);

  vec3f eyePos() const;
  vec3f center() const;
  vec3f lookDir() const;
  vec3f upDir() const;

  void resetCamera(const box3f &worldBounds);

  void updateWindowSize(const vec2i &windowSize);

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  quaternionf screenToArcball(const vec2f &p);

  float zoomSpeed;
  vec2f invWindowSize;
  AffineSpace3f centerTranslation, translation, invCamera;
  quaternionf rotation;
};
