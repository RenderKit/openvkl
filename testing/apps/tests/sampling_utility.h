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
    const unsigned int attributeIndex = 0,
    const float time                  = 0.f)
{
  float scalarSampledValue = vklComputeSample(
      sampler, (const vkl_vec3f *)&objectCoordinates, attributeIndex, time);

  // std::cout << "attr index: " << attributeIndex << std::endl;
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
  float times_4[4]     = {time};
  vklComputeSample4(valid.data(),
                    sampler,
                    (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                    samples_4,
                    attributeIndex,
                    times_4);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
  float samples_8[8]   = {0.f};
  float times_8[8]     = {time};
  vklComputeSample8(valid.data(),
                    sampler,
                    (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                    samples_8,
                    attributeIndex,
                    times_8);

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
  float samples_16[16] = {0.f};
  float times_16[16]   = {time};
  vklComputeSample16(valid.data(),
                     sampler,
                     (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                     samples_16,
                     attributeIndex,
                     times_16);

  REQUIRE(scalarSampledValue == Approx(samples_4[0]).margin(1.e-8));
  REQUIRE(scalarSampledValue == Approx(samples_8[0]).margin(1.e-8));
  REQUIRE(scalarSampledValue == Approx(samples_16[0]).margin(1.e-8));

  // std::cout << "**************************************** end test_scalar_and_vector_sampling\n";
}

inline void test_scalar_and_vector_sampling_multi(
    VKLSampler sampler,
    const vec3f &objectCoordinates,
    const std::vector<float> &sampleTruths,
    const float sampleTolerance,
    const std::vector<unsigned int> &attributeIndices,
    const float time = 0.f)
{
  std::vector<float> scalarSampledValues(attributeIndices.size());

  vklComputeSampleM(sampler,
                    (const vkl_vec3f *)&objectCoordinates,
                    scalarSampledValues.data(),
                    attributeIndices.size(),
                    attributeIndices.data(),
                    (const float *)&time);

  for (unsigned int a = 0; a < attributeIndices.size(); a++) {
    REQUIRE(scalarSampledValues[a] ==
            Approx(sampleTruths[a]).margin(sampleTolerance));
  }

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
  std::vector<float> samples_4(4 * attributeIndices.size(), 0.f);
  std::vector<float> times_4(4, time);
  vklComputeSampleM4(valid.data(),
                     sampler,
                     (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                     samples_4.data(),
                     attributeIndices.size(),
                     attributeIndices.data(),
                     times_4.data());

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
  std::vector<float> samples_8(8 * attributeIndices.size(), 0.f);
  std::vector<float> times_8(8, time);
  vklComputeSampleM8(valid.data(),
                     sampler,
                     (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                     samples_8.data(),
                     attributeIndices.size(),
                     attributeIndices.data(),
                     times_8.data());

  objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
  std::vector<float> samples_16(16 * attributeIndices.size(), 0.f);
  std::vector<float> times_16(16, time);
  vklComputeSampleM16(valid.data(),
                      sampler,
                      (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                      samples_16.data(),
                      attributeIndices.size(),
                      attributeIndices.data(),
                      times_16.data());

  for (unsigned int a = 0; a < attributeIndices.size(); a++) {
    REQUIRE(scalarSampledValues[a] == Approx(samples_4[a * 4 + 0]).margin(1.e-8));
    REQUIRE(scalarSampledValues[a] == Approx(samples_8[a * 8 + 0]).margin(1.e-8));
    REQUIRE(scalarSampledValues[a] == Approx(samples_16[a * 16 + 0]).margin(1.e-8));
  }
}

inline void test_stream_sampling(std::shared_ptr<TestingVolume> v,
                                 const unsigned int attributeIndex = 0,
                                 const float time                  = 0.f)
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
      std::vector<float> times(N, time);
      std::vector<float> samples(N);

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeSampleN(vklSampler,
                        N,
                        objectCoordinates.data(),
                        samples.data(),
                        attributeIndex,
                        times.data());

      for (int i = 0; i < N; i++) {
        float sampleTruth = vklComputeSample(
            vklSampler, &objectCoordinates[i], attributeIndex, time);

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

inline void test_stream_sampling_multi(
    std::shared_ptr<TestingVolume> v,
    const std::vector<unsigned int> &attributeIndices,
    const float time = 0.f)
{
  VKLVolume vklVolume   = v->getVKLVolume();
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  std::stringstream sectionName;
  sectionName << "randomized stream sampling, multi-attribute";

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
      std::vector<float> times(N, time);
      std::vector<float> samples(N * attributeIndices.size());

      for (auto &oc : objectCoordinates) {
        oc = vkl_vec3f{distX(eng), distY(eng), distZ(eng)};
      }

      vklComputeSampleMN(vklSampler,
                         N,
                         objectCoordinates.data(),
                         samples.data(),
                         attributeIndices.size(),
                         attributeIndices.data(),
                         times.data());

      for (unsigned int a = 0; a < attributeIndices.size(); a++) {
        for (int i = 0; i < N; i++) {
          float sampleTruth = vklComputeSample(
              vklSampler, &objectCoordinates[i], attributeIndices[a], times[i]);

          INFO("sample = " << i + 1 << " / " << N
                           << ", attributeIndex = " << a);

          // samples may be NaN if out of bounds of the grid (e.g. for samples
          // in the bounding box of a spherical volume but outside the grid)
          const size_t sampleIndex = i * attributeIndices.size() + a;

          REQUIRE(
              ((sampleTruth == samples[sampleIndex]) ||
               (std::isnan(sampleTruth) && std::isnan(samples[sampleIndex]))));
        }
      }
    }
  }

  vklRelease(vklSampler);
}
