// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cmath>
#include "../../external/catch.hpp"
#include "aos_soa_conversion.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"

#include "wrappers.h"

using namespace openvkl::testing;

// Returns true if the provided coordinate is near the boundary of a box with
// given dimensions. A separate lower span and upper span defines the size of
// the region near the lower and upper boundary of the box, respectively.
inline bool coordinate_in_boundary_span(const vec3i &coord,
                                        const vec3i &dimensions,
                                        int lowerSpan,
                                        int upperSpan)
{
  if (reduce_min(coord) < lowerSpan) {
    return true;
  }

  if (reduce_min(dimensions - 1 - coord) < upperSpan) {
    return true;
  }

  return false;
}

inline void test_scalar_and_vector_sampling(
    VKLSampler sampler,
    const vec3f &objectCoordinates,
    const float sampleTruth,
    const float sampleTolerance,
    const unsigned int attributeIndex = 0,
    const float time                  = 0.f)
{
  float scalarSampledValue = vklComputeSampleWrapper(
      &sampler, (const vkl_vec3f *)&objectCoordinates, attributeIndex, time);

  REQUIRE(scalarSampledValue == Approx(sampleTruth).margin(sampleTolerance));

  // since vklComputeSample() can have a specialized implementation separate
  // from vector sampling, check the vector APIs as well. we only need to
  // check for consistency with the scalar API result, as that has already
  // been validated.

// OpenVKL API functions used in below part of this function is now available
// only for CPU device.
#ifdef OPENVKL_TESTING_CPU
  // permutations of valid mask we will test (first 4 elements only)
  const std::vector<std::vector<int>> valids = {{1, 0, 0, 1}};

  for (const auto &v : valids) {
    std::vector<int> valid(16, 0);
    std::vector<vec3f> objectCoordinatesVector(v.size());
    std::vector<float> times(16, 0.f);

    for (int i = 0; i < v.size(); i++) {
      valid[i] = v[i];

      if (valid[i]) {
        objectCoordinatesVector[i] = objectCoordinates;
        times[i]                   = time;
      }
    }

    AlignedVector<float> objectCoordinatesSOA;

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 4);
    float samples_4[4]   = {0.f};
    vklComputeSample4(valid.data(),
                      &sampler,
                      (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                      samples_4,
                      attributeIndex,
                      time == 0.f ? nullptr : times.data());

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
    float samples_8[8]   = {0.f};
    vklComputeSample8(valid.data(),
                      &sampler,
                      (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                      samples_8,
                      attributeIndex,
                      time == 0.f ? nullptr : times.data());

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
    float samples_16[16] = {0.f};
    vklComputeSample16(valid.data(),
                       &sampler,
                       (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                       samples_16,
                       attributeIndex,
                       time == 0.f ? nullptr : times.data());

    for (int i = 0; i < valid.size(); i++) {
      if (valid[i] != 0) {
#ifdef __ARM_NEON
        static constexpr float tolerance = 1e-3f;

        REQUIRE(scalarSampledValue == Approx(samples_4[i]).margin(tolerance));
        REQUIRE(scalarSampledValue == Approx(samples_8[i]).margin(tolerance));
        REQUIRE(scalarSampledValue == Approx(samples_16[i]).margin(tolerance));
#else
        REQUIRE(scalarSampledValue == Approx(samples_4[i]));
        REQUIRE(scalarSampledValue == Approx(samples_8[i]));
        REQUIRE(scalarSampledValue == Approx(samples_16[i]));
#endif
      }
    }
  }
#endif
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

  vklComputeSampleMWrapper(&sampler,
                           (const vkl_vec3f *)&objectCoordinates,
                           scalarSampledValues.data(),
                           attributeIndices.size(),
                           attributeIndices.data(),
                           time);

  for (unsigned int a = 0; a < attributeIndices.size(); a++) {
    REQUIRE(scalarSampledValues[a] ==
            Approx(sampleTruths[a]).margin(sampleTolerance));
  }

// OpenVKL API functions used in below part of this function is now available
// only for CPU device.
#ifdef OPENVKL_TESTING_CPU
  // since vklComputeSample() can have a specialized implementation separate
  // from vector sampling, check the vector APIs as well. we only need to
  // check for consistency with the scalar API result, as that has already
  // been validated.

  // permutations of valid mask we will test (first 4 elements only)
  const std::vector<std::vector<int>> valids = {{1, 0, 0, 1}};

  for (const auto &v : valids) {
    std::vector<int> valid(16, 0);
    std::vector<vec3f> objectCoordinatesVector(v.size());
    std::vector<float> times(16, 0.f);

    for (int i = 0; i < v.size(); i++) {
      valid[i] = v[i];

      if (valid[i]) {
        objectCoordinatesVector[i] = objectCoordinates;
        times[i]                   = time;
      }
    }

    AlignedVector<float> objectCoordinatesSOA;

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 4);
    std::vector<float> samples_4(4 * attributeIndices.size(), 0.f);
    vklComputeSampleM4(valid.data(),
                       &sampler,
                       (const vkl_vvec3f4 *)objectCoordinatesSOA.data(),
                       samples_4.data(),
                       attributeIndices.size(),
                       attributeIndices.data(),
                       time == 0.f ? nullptr : times.data());

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 8);
    std::vector<float> samples_8(8 * attributeIndices.size(), 0.f);
    vklComputeSampleM8(valid.data(),
                       &sampler,
                       (const vkl_vvec3f8 *)objectCoordinatesSOA.data(),
                       samples_8.data(),
                       attributeIndices.size(),
                       attributeIndices.data(),
                       time == 0.f ? nullptr : times.data());

    objectCoordinatesSOA = AOStoSOA_vec3f(objectCoordinatesVector, 16);
    std::vector<float> samples_16(16 * attributeIndices.size(), 0.f);
    vklComputeSampleM16(valid.data(),
                        &sampler,
                        (const vkl_vvec3f16 *)objectCoordinatesSOA.data(),
                        samples_16.data(),
                        attributeIndices.size(),
                        attributeIndices.data(),
                        time == 0.f ? nullptr : times.data());

    for (unsigned int a = 0; a < attributeIndices.size(); a++) {
      for (int i = 0; i < valid.size(); i++) {
        if (valid[i] != 0) {
          REQUIRE(scalarSampledValues[a] == Approx(samples_4[a * 4 + i]));
          REQUIRE(scalarSampledValues[a] == Approx(samples_8[a * 8 + i]));
          REQUIRE(scalarSampledValues[a] == Approx(samples_16[a * 16 + i]));
        }
      }
    }
  }
#endif
}

// applicable to procedural structured and VDB volumes
template <typename VOLUME_TYPE>
inline void sampling_on_vertices_vs_procedural_values_multi(
    std::shared_ptr<VOLUME_TYPE> v,
    vec3i step       = vec3i(1),
    int lowerSpan    = 0,
    int upperSpan    = 0,
    VKLFilter filter = VKL_FILTER_TRILINEAR)
{
  const float sampleTolerance = 1e-4f;

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklSetInt2(vklSampler, "filter", filter);
  vklSetInt2(vklSampler, "gradientFilter", filter);
  vklCommit2(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    if (coordinate_in_boundary_span(
            offsetWithStep, v->getDimensions(), lowerSpan, upperSpan)) {
      continue;
    }

    const vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    std::vector<float> proceduralValues;

    for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
      proceduralValues.push_back(
          v->computeProceduralValue(objectCoordinates, a));
    }

    for (unsigned int a = 0; a < v->getNumAttributes(); a++) {
      test_scalar_and_vector_sampling(vklSampler,
                                      objectCoordinates,
                                      proceduralValues[a],
                                      sampleTolerance,
                                      a);
    }

    std::vector<unsigned int> attributeIndices(v->getNumAttributes());
    std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

    test_scalar_and_vector_sampling_multi(vklSampler,
                                          objectCoordinates,
                                          proceduralValues,
                                          sampleTolerance,
                                          attributeIndices);
  }

  vklRelease2(vklSampler);
}

// OpenVKL API functions used in this function is now available only for CPU
// device.
#ifdef OPENVKL_TESTING_CPU
inline void test_stream_sampling(std::shared_ptr<TestingVolume> v,
                                 const unsigned int attributeIndex = 0,
                                 const float time                  = 0.f)
{
  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

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

      vklComputeSampleN(&vklSampler,
                        N,
                        objectCoordinates.data(),
                        samples.data(),
                        attributeIndex,
                        time == 0.f ? nullptr : times.data());

      for (int i = 0; i < N; i++) {
        float sampleTruth = vklComputeSample(
            &vklSampler, &objectCoordinates[i], attributeIndex, time);

        INFO("sample = " << i + 1 << " / " << N);

        // samples may be NaN if out of bounds of the grid (e.g. for samples in
        // the bounding box of a spherical volume but outside the grid)
#ifdef __ARM_NEON
        static constexpr float tolerance = 1e-3f;
#else
        static constexpr float tolerance = 1e-5f;
#endif

        REQUIRE(((sampleTruth == Approx(samples[i]).margin(tolerance)) ||
                 (std::isnan(sampleTruth) && std::isnan(samples[i]))));
      }
    }
  }

  vklRelease2(vklSampler);
}
#endif

// OpenVKL API functions used in this function is now available only for CPU
// device.
#ifdef OPENVKL_TESTING_CPU
inline void test_stream_sampling_multi(
    std::shared_ptr<TestingVolume> v,
    const std::vector<unsigned int> &attributeIndices,
    const float time = 0.f)
{
  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

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

      vklComputeSampleMN(&vklSampler,
                         N,
                         objectCoordinates.data(),
                         samples.data(),
                         attributeIndices.size(),
                         attributeIndices.data(),
                         time == 0.f ? nullptr : times.data());

      for (unsigned int a = 0; a < attributeIndices.size(); a++) {
        for (int i = 0; i < N; i++) {
          float sampleTruth = vklComputeSample(&vklSampler,
                                               &objectCoordinates[i],
                                               attributeIndices[a],
                                               times[i]);

          INFO("sample = " << i + 1 << " / " << N
                           << ", attributeIndex = " << a);

          // samples may be NaN if out of bounds of the grid (e.g. for samples
          // in the bounding box of a spherical volume but outside the grid)
          const size_t sampleIndex = i * attributeIndices.size() + a;

#ifdef __ARM_NEON
          static constexpr float tolerance = 1e-3f;
#else
          static constexpr float tolerance = 1e-5f;
#endif

          REQUIRE((
              (sampleTruth == Approx(samples[sampleIndex]).margin(tolerance)) ||
              (std::isnan(sampleTruth) && std::isnan(samples[sampleIndex]))));
        }
      }
    }
  }

  vklRelease2(vklSampler);
}
#endif
