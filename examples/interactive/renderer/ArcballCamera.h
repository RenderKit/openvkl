// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/math/AffineSpace.h>
#include <rkcommon/math/Quaternion.h>
#include <rkcommon/math/vec.h>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    /*
     * The camera model is a pinhole camera with this configuration:
     *
     *          /
     *         /
     * S\     /
     * S \   /
     * |  \ /
     * O   A ------- <-d-> ----- C
     * |  / \
     * S /   \
     * S/     \
     *         \
     *          \
     *
     * |<f>|
     *
     * The camera origin is at O, in the center of the sensor.
     * f is the focal length, and the camera rotates around the
     * center at C, which is at a distance d+f from the sensor.
     *
     * The sensor has a set width, but based on the available screen
     * space may have an arbitrary aspect ratio.
     *
     * The sensor is parametrized in terms of its width, such that
     * (0,0) is the center of the sensor, and it ranges horizontally
     * from -0.5 to 0.5.
     *
     * To transform a point (sx,sy) in sensor coordinates to world coordinates,
     * first multiply by the sensor width. Then, dolly, rotate, and move the
     * center:
     *
     * pw = Mc * Mr * Md * sensorWidth * (sx,sy,0)
     *
     * Local camera directions are defined by rays through the aperture:
     *
     * d(sx,sy) = (0,0,-f) - (sensorWidth * (sx, sy, 0))
     *
     */
    struct ArcballCamera
    {
      ArcballCamera() {}

      // Set parameters to fit the given box on screen.
      void fitToScreen(const box3f &box);

      // Rotate around the center. Parameters in pixels.
      void rotate(const vec2f &pfrom, const vec2f &pto, const vec2i &res);

      // Dolly the camera towards / away from the center.
      void dolly(const vec2f &pfrom, const vec2f &pto, const vec2i &res);

      // Move the center in a plane perpendicular to the sensor.
      // Parameters in pixels.
      void pan(const vec2f &pfrom, const vec2f &pto, const vec2i &res);

      void createRay(const vec2f &p,
                     const vec2i &res,
                     vec3f &o,
                     vec3f &d) const;

      vec3f getEyePos() const
      {
        return xfmPoint(getCameraToWorld(), vec3f(0, 0, 0));
      }

      vec3f getCenterPos() const
      {
        return -center;
      }

      vec3f getLookDir() const
      {
        return xfmVector(getCameraToWorld(), vec3f(0, 0, 1));
      }

      vec3f getUpDir() const
      {
        return xfmVector(getCameraToWorld(), vec3f(0, 1, 0));
      }

      float getSensorWidth() const
      {
        return sensorWidth;
      }

      float getFocalLength() const
      {
        return focalLength;
      }

      AffineSpace3f getCameraToWorld() const;

     private:
      // Sensor space is in units of sensor width (so we divide by res.x)
      // with the center of the screen (0,0).
      vec2f pixelToSensor(const vec2f &p, const vec2i &res) const;
      vec3f sensorToCamera(const vec2f &s) const;
      vec3f cameraToCenter(const vec3f &pCamera) const;
      quaternionf sensorToArcball(const vec2f &p) const;

     private:
      float focalLength{0.0028f};
      float sensorWidth{0.0022f};
      float dollyDistance{1.f};
      vec3f center{0.f, 0.f, 0.f};
      quaternionf rotation{one};
    };

  }  // namespace examples
}  // namespace openvkl
