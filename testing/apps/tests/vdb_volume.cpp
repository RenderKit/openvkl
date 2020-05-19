// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace openvkl;

static void init_driver()
{
  vklLoadModule("ispc_driver");
  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

using openvkl::testing::WaveletVdbVolume;
using openvkl::testing::XYZVdbVolume;

TEST_CASE("VDB volume value range", "[value_range]")
{
  init_driver();

  SECTION("WaveletVdbVolume nearest")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_NEAREST));
    REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
    REQUIRE(valueRange.upper >= valueRange.lower);
    REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
            Approx(6.f).epsilon(0.001f));
    REQUIRE_NOTHROW(delete volume);
  }

  SECTION("WaveletVdbVolume trilinear")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));
    REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
    REQUIRE(valueRange.upper >= valueRange.lower);
    REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
            Approx(6.f).epsilon(0.001f));
    REQUIRE_NOTHROW(delete volume);
  }
}

TEST_CASE("VDB volume sampling", "[volume_sampling]")
{
  init_driver();

  SECTION("WaveletVdbVolume nearest")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_NEAREST));

    VKLVolume vklVolume = volume->getVKLVolume();
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit(vklSampler);
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;

      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      const float proceduralValue =
          volume->computeProceduralValue(objectCoordinates);

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      test_scalar_and_vector_sampling(
          vklSampler, objectCoordinates, proceduralValue, 1e-4f);
    }

    REQUIRE_NOTHROW(delete volume);
    vklRelease(vklSampler);
  }

  SECTION("WaveletVdbVolume trilinear")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

    VKLVolume vklVolume = volume->getVKLVolume();
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit(vklSampler);
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;

      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      const float proceduralValue =
          volume->computeProceduralValue(objectCoordinates);

      INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                       << offsetWithStep.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      test_scalar_and_vector_sampling(
          vklSampler, objectCoordinates, proceduralValue, 1e-4f);
    }

    REQUIRE_NOTHROW(delete volume);
    vklRelease(vklSampler);
  }
}

TEST_CASE("VDB volume interval iterator", "[volume_sampling]")
{
  init_driver();

  WaveletVdbVolume *volume = nullptr;
  range1f valueRange;
  REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                      128, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

  VKLVolume vklVolume = volume->getVKLVolume();
  VKLIntervalIterator iterator;
  VKLInterval interval;
  vkl_vec3f origin{0, 0, -5.f};
  vkl_vec3f direction{0, 0, 1.f};
  vkl_range1f tRange{0.f, 1000.f};
  REQUIRE_NOTHROW(vklInitIntervalIterator(
      &iterator, vklVolume, &origin, &direction, &tRange, nullptr));
  REQUIRE_NOTHROW(vklIterateInterval(&iterator, &interval));
  REQUIRE_NOTHROW(delete volume);
}

TEST_CASE("VDB volume gradients", "[volume_gradients]")
{
  init_driver();

  SECTION("WaveletVdbVolume nearest")
  {
    WaveletVdbVolume *volume = nullptr;
    range1f valueRange;
    REQUIRE_NOTHROW(volume = new WaveletVdbVolume(
                        128, vec3f(0.f), vec3f(1.f), VKL_FILTER_NEAREST));

    VKLVolume vklVolume = volume->getVKLVolume();
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit(vklSampler);
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      const auto offsetWithStep = offset * step;
      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      const vkl_vec3f vklGradient =
          vklComputeGradient(vklSampler, (const vkl_vec3f *)&objectCoordinates);
      const vec3f gradient = (const vec3f &)vklGradient;

      REQUIRE(gradient.x == 0.f);
      REQUIRE(gradient.y == 0.f);
      REQUIRE(gradient.z == 0.f);
    }

    REQUIRE_NOTHROW(delete volume);
    vklRelease(vklSampler);
  }

  SECTION("XYZVdbVolume trilinear")
  {
    XYZVdbVolume *volume = nullptr;
    range1f valueRange;
    const int dim = 128;
    REQUIRE_NOTHROW(volume = new XYZVdbVolume(
                        dim, vec3f(0.f), vec3f(1.f), VKL_FILTER_TRILINEAR));

    VKLVolume vklVolume = volume->getVKLVolume();
    VKLSampler vklSampler = vklNewSampler(vklVolume);
    vklCommit(vklSampler);
    const vec3i step(1);
    multidim_index_sequence<3> mis(volume->getDimensions() / step);
    for (const auto &offset : mis) {
      if (offset.x+1 >= volume->getDimensions().x
       || offset.y+1 >= volume->getDimensions().y
       || offset.z+1 >= volume->getDimensions().z)
      {
        continue;
      }

      const auto offsetWithStep = offset * step;
      const vec3f objectCoordinates =
          volume->transformLocalToObjectCoordinates(offsetWithStep);

      INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
      INFO("objectCoordinates = " << objectCoordinates.x << " "
                                  << objectCoordinates.y << " "
                                  << objectCoordinates.z);

      const vkl_vec3f vklGradient =
          vklComputeGradient(vklSampler, (const vkl_vec3f *)&objectCoordinates);
      const vec3f gradient = (const vec3f &)vklGradient;

      // compare to analytical gradient
      const vec3f proceduralGradient =
          volume->computeProceduralGradient(objectCoordinates);

      static constexpr float tolerance = 0.1f;
      REQUIRE(gradient.x == Approx(proceduralGradient.x).margin(tolerance));
      REQUIRE(gradient.y == Approx(proceduralGradient.y).margin(tolerance));
      REQUIRE(gradient.z == Approx(proceduralGradient.z).margin(tolerance));
    }

    REQUIRE_NOTHROW(delete volume);
    vklRelease(vklSampler);
  }
}
