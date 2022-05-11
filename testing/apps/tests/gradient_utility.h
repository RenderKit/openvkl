// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

using namespace openvkl::testing;

inline void test_scalar_and_vector_gradients(
    VKLSampler sampler,
    const vec3f &objectCoordinates,
    const vec3f gradientTruth,
    const float gradientTolerance,
    const unsigned int attributeIndex = 0,
    const float time                  = 0.f)
{
  vkl_vec3f scalarGradientValue = vklComputeGradient(
      sampler, (const vkl_vec3f *)&objectCoordinates, attributeIndex, time);

  REQUIRE(scalarGradientValue.x ==
          Approx(gradientTruth.x).margin(gradientTolerance));
  REQUIRE(scalarGradientValue.y ==
          Approx(gradientTruth.y).margin(gradientTolerance));
  REQUIRE(scalarGradientValue.z ==
          Approx(gradientTruth.z).margin(gradientTolerance));

  // since vklComputeGradient() can have a specialized implementation separate
  // from vector gradients, check the vector APIs as well. we only need to
  // check for consistency with the scalar API result, as that has already
  // been validated.

  // first lane active only
  std::vector<int> valid(16, 0);
  valid[0] = 1;

  std::vector<vec3f> objectCoordinatesVector;
  objectCoordinatesVector.push_back(objectCoordinates);

  AlignedVector<float> objectCoordinatesSOA;

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 4);
  float times_4[4]     = {time};
  vkl_vvec3f4 gradients_4;
  vklComputeGradient4(valid.data(),
                      sampler,
                      (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                      &gradients_4,
                      attributeIndex,
                      time == 0.f ? nullptr : times_4);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
  float times_8[8]     = {time};
  vkl_vvec3f8 gradients_8;
  vklComputeGradient8(valid.data(),
                      sampler,
                      (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                      &gradients_8,
                      attributeIndex,
                      time == 0.f ? nullptr : times_8);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
  vkl_vvec3f16 gradients_16;
  float times_16[16] = {time};
  vklComputeGradient16(valid.data(),
                       sampler,
                       (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                       &gradients_16,
                       attributeIndex,
                       time == 0.f ? nullptr : times_16);

  REQUIRE(scalarGradientValue.x == gradients_4.x[0]);
  REQUIRE(scalarGradientValue.y == gradients_4.y[0]);
  REQUIRE(scalarGradientValue.z == gradients_4.z[0]);

  REQUIRE(scalarGradientValue.x == gradients_8.x[0]);
  REQUIRE(scalarGradientValue.y == gradients_8.y[0]);
  REQUIRE(scalarGradientValue.z == gradients_8.z[0]);

  REQUIRE(scalarGradientValue.x == gradients_16.x[0]);
  REQUIRE(scalarGradientValue.y == gradients_16.y[0]);
  REQUIRE(scalarGradientValue.z == gradients_16.z[0]);
}

// applicable to procedural structured and VDB volumes
template <typename VOLUME_TYPE>
inline void gradients_on_vertices_vs_procedural_values_multi(
    std::shared_ptr<VOLUME_TYPE> v,
    vec3i step              = vec3i(1),
    float gradientTolerance = 0.1f)
{
  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  constexpr int filterRadius = 2;
  multidim_index_sequence<3> mis((v->getDimensions() - 2 * filterRadius) /
                                 step);

  for (unsigned int attributeIndex = 0; attributeIndex < v->getNumAttributes();
       attributeIndex++) {
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step + filterRadius;
      if (offsetWithStep.x + filterRadius >= v->getDimensions().x ||
          offsetWithStep.y + filterRadius >= v->getDimensions().y ||
          offsetWithStep.z + filterRadius >= v->getDimensions().z) {
        continue;
      }

      vec3f objectCoordinates =
          v->transformLocalToObjectCoordinates(offsetWithStep);

      const vec3f proceduralGradient =
          v->computeProceduralGradient(objectCoordinates, attributeIndex);

      INFO("attributeIndex = " << attributeIndex);
      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      // larger tolerance since gradients are not exact (e.g. computed via
      // finite differences)
      test_scalar_and_vector_gradients(vklSampler,
                                       objectCoordinates,
                                       proceduralGradient,
                                       gradientTolerance,
                                       attributeIndex);
    }
  }

  vklRelease(vklSampler);
}

inline void test_stream_gradients(std::shared_ptr<TestingVolume> v,
                                  const unsigned int attributeIndex = 0,
                                  const float time                  = 0.f)
{
  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  SECTION("randomized stream gradients")
  {
    vkl_box3f bbox = vklGetBoundingBox(vklVolume);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
    std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

    const int maxN = 1024;

    for (int N = 1; N < maxN; N++) {
      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<float> times(N, time);
      std::vector<vkl_vec3f> gradients(N);

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeGradientN(vklSampler,
                          N,
                          objectCoordinates.data(),
                          gradients.data(),
                          attributeIndex,
                          time == 0.f ? nullptr : times.data());

      for (int i = 0; i < N; i++) {
        vkl_vec3f gradientTruth = vklComputeGradient(
            vklSampler, &objectCoordinates[i], attributeIndex, time);

        INFO("gradient = " << i + 1 << " / " << N);
        INFO("gradientTruth = " << gradientTruth.x << ", "
                           << gradientTruth.y << ", " << gradientTruth.z);
        INFO("gradients[i] = " << gradients[i].x << ", "
                           << gradients[i].y << ", " << gradients[i].z);

        // gradients may be NaN if out of bounds of the grid (e.g. for gradients
        // in the bounding box of a spherical volume but outside the grid)
#ifdef __ARM_NEON
        static constexpr float tolerance = 0.1f;

        REQUIRE(
            (((gradientTruth.x == Approx(gradients[i].x).margin(tolerance)) ||
              (std::isnan(gradientTruth.x) && std::isnan(gradients[i].x))) &&
             ((gradientTruth.y == Approx(gradients[i].y).margin(tolerance)) ||
              (std::isnan(gradientTruth.y) && std::isnan(gradients[i].y))) &&
             ((gradientTruth.z == Approx(gradients[i].z).margin(tolerance)) ||
              (std::isnan(gradientTruth.z) && std::isnan(gradients[i].z)))));
#else
        REQUIRE(
            (((gradientTruth.x == gradients[i].x) ||
              (std::isnan(gradientTruth.x) && std::isnan(gradients[i].x))) &&
             ((gradientTruth.y == gradients[i].y) ||
              (std::isnan(gradientTruth.y) && std::isnan(gradients[i].y))) &&
             ((gradientTruth.z == gradients[i].z) ||
              (std::isnan(gradientTruth.z) && std::isnan(gradients[i].z)))));
#endif
      }
    }
  }
  vklRelease(vklSampler);
}
