// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"

using namespace openvkl::testing;

inline void test_scalar_and_vector_sampling(
    VKLSampler sampler,
    const vec3f &objectCoordinates,
    const float sampleTruth,
    const float sampleTolerance,
    const unsigned int attributeIndex = 0)
{
  float scalarSampledValue = vklComputeSample(
      sampler, (const vkl_vec3f *)&objectCoordinates, attributeIndex);

  REQUIRE(scalarSampledValue == Approx(sampleTruth).margin(sampleTolerance));

  // since vklComputeSample() can have a specialized implementation separate
  // from vector sampling, check the vector APIs as well. we only need to
  // check for consistency with the scalar API result, as that has already
  // been validated.

  // first lane active only
  std::vector<int> valid(16, 0);
  valid[0] = 1;

  std::vector<vec3f> objectCoordinatesVector;
  objectCoordinatesVector.push_back(objectCoordinates);

  AlignedVector<float> objectCoordinatesSOA;

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 4);
  float samples_4[4]   = {0.f};
  vklComputeSample4(valid.data(),
                    sampler,
                    (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                    samples_4,
                    attributeIndex);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
  float samples_8[8]   = {0.f};
  vklComputeSample8(valid.data(),
                    sampler,
                    (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                    samples_8,
                    attributeIndex);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
  float samples_16[16] = {0.f};
  vklComputeSample16(valid.data(),
                     sampler,
                     (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                     samples_16,
                     attributeIndex);

  REQUIRE(scalarSampledValue == samples_4[0]);
  REQUIRE(scalarSampledValue == samples_8[0]);
  REQUIRE(scalarSampledValue == samples_16[0]);
}

inline void test_stream_sampling(std::shared_ptr<TestingVolume> v,
                                 const unsigned int attributeIndex = 0)
{
  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  std::stringstream sectionName;
  sectionName << "randomized stream sampling, attribute " << attributeIndex;

  SECTION(sectionName.str())
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
      std::vector<float> samples(N);

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeSampleN(vklSampler,
                        N,
                        objectCoordinates.data(),
                        samples.data(),
                        attributeIndex);

      for (int i = 0; i < N; i++) {
        float sampleTruth =
            vklComputeSample(vklSampler, &objectCoordinates[i], attributeIndex);

        INFO("sample = " << i + 1 << " / " << N);

        // samples may be NaN if out of bounds of the grid (e.g. for samples in
        // the bounding box of a spherical volume but outside the grid)
        REQUIRE(((sampleTruth == samples[i]) ||
                 (std::isnan(sampleTruth) && std::isnan(samples[i]))));
      }
    }
  }

  vklRelease(vklSampler);
}
