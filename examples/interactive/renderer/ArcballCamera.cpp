// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"

namespace openvkl {
  namespace examples {

    void ArcballCamera::fitToScreen(const box3f &box)
    {
      focalLength = 0.0028f;
      sensorWidth = 0.0022f;
      center = 0.5f * (box.lower + box.upper);
      rotation = quaternionf(0.f, 0.1f * M_PI, 0.f) *
                 quaternionf(0.25f * M_PI, 0.f, 0.f);

      // The factor of 1.5 is empirical, but it seems to work well!
      const float diag = length(box.upper-box.lower);
      dollyDistance = 1.5f * diag;
    }

    void ArcballCamera::rotate(const vec2f &pfrom,
                               const vec2f &pto,
                               const vec2i &res)
    {
      const vec2f sfrom       = pixelToSensor(pfrom, res);
      const vec2f sto         = pixelToSensor(pto, res);
      const quaternionf qfrom = sensorToArcball(sfrom);
      const quaternionf qto   = sensorToArcball(sto);
      const quaternionf rot   = qto * qfrom;
      rotation                = rot * rotation;
    }

    // Move towards the center, or away from it. Parameters in pixels.
    void ArcballCamera::dolly(const vec2f &pfrom,
                              const vec2f &pto,
                              const vec2i &res)
    {
      const vec2f sfrom = pixelToSensor(pfrom, res);
      const vec2f sto   = pixelToSensor(pto, res);

      // The minimum distance of the sensor to center is always
      // focalLength; we only modify dollyDistance here.
      const float delta = dot(sto - sfrom, vec2f(1.f,1.f));
      float speed       = 0.5f * (dollyDistance + focalLength);
      if (delta > 0) {
        speed = std::max(speed, 1.f);
      }

      dollyDistance =
          std::max<float>(0.1f, dollyDistance - speed * delta);
    }

    void ArcballCamera::pan(const vec2f &pfrom,
                            const vec2f &pto,
                            const vec2i &res)
    {
      const vec3f cfrom = sensorToCamera(pixelToSensor(pfrom, res));
      const vec3f cto   = sensorToCamera(pixelToSensor(pto, res));
      const vec3f delta = cameraToCenter(cto) - cameraToCenter(cfrom);
      center            = delta + center;
    }

    void ArcballCamera::createRay(const vec2f &p,
                                  const vec2i &res,
                                  vec3f &o,
                                  vec3f &d) const
    {
      const vec2f pSensor = pixelToSensor(p, res);
      const vec3f pCamera = sensorToCamera(pSensor);
      const vec3f pAperture(0.f, 0.f, -focalLength);
      const vec3f dCamera     = pAperture - pCamera;
      const AffineSpace3f ctw = getCameraToWorld();
      o                       = xfmPoint(ctw, pCamera);
      d                       = normalize(xfmVector(ctw, dCamera));
    }

    vec2f ArcballCamera::pixelToSensor(const vec2f &p, const vec2i &res) const
    {
      // Note: units of sensor width!
      const float irx = 1.f / res.x;
      return vec2f((p.x - 0.5f * res.x) * irx, -(p.y - 0.5f * res.y) * irx);
    }

    vec3f ArcballCamera::sensorToCamera(const vec2f &s) const
    {
      return vec3f(sensorWidth * s.x, sensorWidth * s.y, 0.f);
    }

    /*
     * We use this method to enable panning. In particular, we project
     * sensor positions to a plane defined by the center position and
     * parallel to the sensor.
     * This way we obtain an offset, in world coordinates, from a
     * screen space delta.
     */
    vec3f ArcballCamera::cameraToCenter(const vec3f &pCamera) const
    {
      const float proj = -dollyDistance / focalLength;
      const vec3f pCenter(
          proj * pCamera.x, proj * pCamera.y, dollyDistance + focalLength);
      return rcp(LinearSpace3f(rotation)) * pCenter;
    }

    quaternionf ArcballCamera::sensorToArcball(const vec2f &p) const
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

    // NOTE: If this becomes a performance problem, cache the matrices.
    AffineSpace3f ArcballCamera::getCameraToWorld() const
    {
      return AffineSpace3f::translate(center) *
             AffineSpace3f(rcp(LinearSpace3f(rotation))) *
             AffineSpace3f::translate(
                 vec3f(0.f, 0.f, dollyDistance + focalLength));
    }

  }  // namespace examples
}  // namespace openvkl
