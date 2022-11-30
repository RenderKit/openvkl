// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

using namespace openvkl;

using openvkl::testing::TestingVdbTorusVolume;
using openvkl::testing::WaveletVdbVolumeFloat;
using openvkl::testing::XYZVdbVolumeFloat;

#if OPENVKL_DEVICE_CPU_VDB
TEST_CASE("VDB volume leaf validation", "[validation]")
{
  initializeOpenVKL();
  VKLVolume volume = vklNewVolume(getOpenVKLDevice(), "vdb");

  const uint32_t level   = vklVdbNumLevels() - 1;
  const VKLFormat format = VKL_FORMAT_DENSE_ZYX;
  const vec3i origin     = vec3i(0, 0, 0);

  std::vector<float> voxels(vklVdbLevelNumVoxels(level));

  SECTION("Invalid level")
  {
    VKLData data =
        vklNewData(getOpenVKLDevice(), voxels.size(), VKL_FLOAT, voxels.data());
    uint32_t invLevel = vklVdbNumLevels();
    VKLData levelData = vklNewData(getOpenVKLDevice(), 1, VKL_UINT, &invLevel);
    vklSetData2(volume, "node.level", levelData);
    vklRelease(levelData);
    VKLData originData = vklNewData(getOpenVKLDevice(), 1, VKL_VEC3I, &origin);
    vklSetData2(volume, "node.origin", originData);
    vklRelease(originData);
    VKLData formatData = vklNewData(getOpenVKLDevice(), 1, VKL_UINT, &format);
    vklSetData2(volume, "node.format", formatData);
    vklRelease(formatData);
    VKLData dataData = vklNewData(getOpenVKLDevice(), 1, VKL_DATA, &data);
    vklSetData2(volume, "node.data", dataData);
    vklRelease(dataData);
    vklRelease(data);

    vklCommit2(volume);
    REQUIRE(vklDeviceGetLastErrorCode(getOpenVKLDevice()) == 1);
    REQUIRE(std::string(vklDeviceGetLastErrorMsg(getOpenVKLDevice())) ==
            "invalid node level 4 for this vdb configuration");
  }

  SECTION("Constant data too small")
  {
    VKLData data = vklNewData(getOpenVKLDevice(), 1, VKL_FLOAT, voxels.data());
    VKLData levelData = vklNewData(getOpenVKLDevice(), 1, VKL_UINT, &level);
    vklSetData2(volume, "node.level", levelData);
    vklRelease(levelData);
    VKLData originData = vklNewData(getOpenVKLDevice(), 1, VKL_VEC3I, &origin);
    vklSetData2(volume, "node.origin", originData);
    vklRelease(originData);
    VKLData formatData = vklNewData(getOpenVKLDevice(), 1, VKL_UINT, &format);
    vklSetData2(volume, "node.format", formatData);
    vklRelease(formatData);
    VKLData dataData = vklNewData(getOpenVKLDevice(), 1, VKL_DATA, &data);
    vklSetData2(volume, "node.data", dataData);
    vklRelease(dataData);
    vklRelease(data);

    vklCommit2(volume);
    REQUIRE(vklDeviceGetLastErrorCode(getOpenVKLDevice()) == 1);
    REQUIRE(std::string(vklDeviceGetLastErrorMsg(getOpenVKLDevice()))
                .find("Node data too small") != std::string::npos);
  }

  vklRelease2(volume);

  shutdownOpenVKL();
}

TEST_CASE("VDB volume value range", "[value_range]")
{
  initializeOpenVKL();

  for (const auto &repackNodes : {true, false}) {
    std::stringstream sectionName;
    sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

    DYNAMIC_SECTION(sectionName.str())
    {
      // half

      SECTION("WaveletVdbVolumeHalf")
      {
        WaveletVdbVolumeHalf *volume = nullptr;
        range1f valueRange;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeHalf(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));
        REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
        REQUIRE(valueRange.upper >= valueRange.lower);
        REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
                Approx(6.f).epsilon(0.001f));
        REQUIRE_NOTHROW(delete volume);
      }

      // float

      SECTION("WaveletVdbVolumeFloat")
      {
        WaveletVdbVolumeFloat *volume = nullptr;
        range1f valueRange;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeFloat(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));
        REQUIRE_NOTHROW(valueRange = volume->getComputedValueRange());
        REQUIRE(valueRange.upper >= valueRange.lower);
        REQUIRE(std::fabs((valueRange.upper - valueRange.lower)) ==
                Approx(6.f).epsilon(0.001f));
        REQUIRE_NOTHROW(delete volume);
      }
    }
  }

  shutdownOpenVKL();
}

TEST_CASE("VDB volume sampling", "[volume_sampling]")
{
  initializeOpenVKL();

  for (const auto &repackNodes : {true, false}) {
    std::stringstream sectionName;
    sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

    DYNAMIC_SECTION(sectionName.str())
    {
      // half

      SECTION("WaveletVdbVolumeHalf nearest")
      {
        WaveletVdbVolumeHalf *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeHalf(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_NEAREST);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_NEAREST);
        vklCommit2(vklSampler);
        const vec3i step(2);
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
        vklRelease2(vklSampler);
      }

      SECTION("WaveletVdbVolumeHalf trilinear")
      {
        WaveletVdbVolumeHalf *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeHalf(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRILINEAR);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRILINEAR);
        vklCommit2(vklSampler);
        const vec3i step(2);
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
        vklRelease2(vklSampler);
      }

      SECTION("WaveletVdbVolumeHalf tricubic")
      {
        WaveletVdbVolumeHalf *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeHalf(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRICUBIC);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRICUBIC);
        vklCommit2(vklSampler);
        const vec3i step(2);

        // tricubic support span; ignore coordinates here since they will
        // interpolate with background
        const int lowerSpan = 1;
        const int upperSpan = 2;

        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step;

          if (coordinate_in_boundary_span(offsetWithStep,
                                          volume->getDimensions(),
                                          lowerSpan,
                                          upperSpan)) {
            continue;
          }

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
        vklRelease2(vklSampler);
      }

      // float

      SECTION("WaveletVdbVolumeFloat nearest")
      {
        WaveletVdbVolumeFloat *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeFloat(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_NEAREST);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_NEAREST);
        vklCommit2(vklSampler);
        const vec3i step(2);
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
        vklRelease2(vklSampler);
      }

      SECTION("WaveletVdbVolumeFloat trilinear")
      {
        WaveletVdbVolumeFloat *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeFloat(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRILINEAR);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRILINEAR);
        vklCommit2(vklSampler);
        const vec3i step(2);
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
        vklRelease2(vklSampler);
      }

      SECTION("WaveletVdbVolumeFloat tricubic")
      {
        WaveletVdbVolumeFloat *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeFloat(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRICUBIC);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRICUBIC);
        vklCommit2(vklSampler);
        const vec3i step(2);

        // tricubic support span; ignore coordinates here since they will
        // interpolate with background
        const int lowerSpan = 1;
        const int upperSpan = 2;

        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step;

          if (coordinate_in_boundary_span(offsetWithStep,
                                          volume->getDimensions(),
                                          lowerSpan,
                                          upperSpan)) {
            continue;
          }

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
        vklRelease2(vklSampler);
      }
    }
  }

  shutdownOpenVKL();
}

TEST_CASE("VDB volume interval iterator", "[volume_sampling]")
{
  initializeOpenVKL();

  WaveletVdbVolumeFloat *volume = nullptr;
  range1f valueRange;
  REQUIRE_NOTHROW(volume = new WaveletVdbVolumeFloat(
                      getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f)));

  VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklSetInt2(vklSampler, "filter", VKL_FILTER_TRILINEAR);
  vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRILINEAR);
  vklCommit2(vklSampler);
  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(vklSampler);
  vklCommit2(intervalContext);
  std::vector<char> buffer(vklGetIntervalIteratorSize(&intervalContext));
  VKLIntervalIterator iterator;
  VKLInterval interval;
  vkl_vec3f origin{0, 0, -5.f};
  vkl_vec3f direction{0, 0, 1.f};
  vkl_range1f tRange{0.f, 1000.f};
  const float time = 0.f;
  REQUIRE_NOTHROW(
      iterator = vklInitIntervalIterator(
          &intervalContext, &origin, &direction, &tRange, time, buffer.data()));
  REQUIRE_NOTHROW(vklIterateInterval(iterator, &interval));
  REQUIRE_NOTHROW(vklRelease2(intervalContext));
  REQUIRE_NOTHROW(vklRelease2(vklSampler));
  REQUIRE_NOTHROW(delete volume);

  shutdownOpenVKL();
}

TEST_CASE("VDB volume gradients", "[volume_gradients]")
{
  initializeOpenVKL();

  for (const auto &repackNodes : {true, false}) {
    std::stringstream sectionName;
    sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

    DYNAMIC_SECTION(sectionName.str())
    {
      // half

      SECTION("WaveletVdbVolumeHalf nearest")
      {
        WaveletVdbVolumeHalf *volume = nullptr;
        // use a smaller grid spacing to avoid overflow / precision issues for
        // half
        REQUIRE_NOTHROW(volume = new WaveletVdbVolumeHalf(getOpenVKLDevice(),
                                                          128,
                                                          vec3f(0.f),
                                                          vec3f(0.01f),
                                                          repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_NEAREST);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_NEAREST);
        vklCommit2(vklSampler);
        const vec3i step(2);
        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step;
          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
          const vec3f gradient = (const vec3f &)vklGradient;

          REQUIRE(gradient.x == 0.f);
          REQUIRE(gradient.y == 0.f);
          REQUIRE(gradient.z == 0.f);
        }

        REQUIRE_NOTHROW(delete volume);
        vklRelease2(vklSampler);
      }

      SECTION("XYZVdbVolumeHalf trilinear")
      {
        XYZVdbVolumeHalf *volume = nullptr;
        const int dim            = 128;
        // use a smaller grid spacing to avoid overflow / precision issues for
        // half
        REQUIRE_NOTHROW(volume = new XYZVdbVolumeHalf(getOpenVKLDevice(),
                                                      dim,
                                                      vec3f(0.f),
                                                      vec3f(0.01f),
                                                      repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRILINEAR);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRILINEAR);
        vklCommit2(vklSampler);
        const vec3i step(2);
        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          if (offset.x + 1 >= volume->getDimensions().x ||
              offset.y + 1 >= volume->getDimensions().y ||
              offset.z + 1 >= volume->getDimensions().z) {
            continue;
          }

          const auto offsetWithStep = offset * step;
          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
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
        vklRelease2(vklSampler);
      }

      SECTION("XYZVdbVolumeHalf tricubic")
      {
        XYZVdbVolumeHalf *volume = nullptr;
        const int dim            = 128;
        // use a smaller grid spacing to avoid overflow / precision issues for
        // half
        REQUIRE_NOTHROW(volume = new XYZVdbVolumeHalf(getOpenVKLDevice(),
                                                      dim,
                                                      vec3f(0.f),
                                                      vec3f(0.01f),
                                                      repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRICUBIC);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRICUBIC);
        vklCommit2(vklSampler);
        const vec3i step(2);

        // Gradient will be different around the border due to central
        // differencing, so we discard the outer layer of voxels.
        constexpr int filterRadius = 2;
        multidim_index_sequence<3> mis(
            (volume->getDimensions() - 2 * filterRadius) / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step + filterRadius;
          if (offsetWithStep.x + filterRadius >= volume->getDimensions().x ||
              offsetWithStep.y + filterRadius >= volume->getDimensions().y ||
              offsetWithStep.z + filterRadius >= volume->getDimensions().z) {
            continue;
          }

          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
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
        vklRelease2(vklSampler);
      }

      // float

      SECTION("WaveletVdbVolumeFloat")
      {
        WaveletVdbVolumeFloat *volume = nullptr;
        REQUIRE_NOTHROW(
            volume = new WaveletVdbVolumeFloat(
                getOpenVKLDevice(), 128, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_NEAREST);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_NEAREST);
        vklCommit2(vklSampler);
        const vec3i step(2);
        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step;
          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
          const vec3f gradient = (const vec3f &)vklGradient;

          REQUIRE(gradient.x == 0.f);
          REQUIRE(gradient.y == 0.f);
          REQUIRE(gradient.z == 0.f);
        }

        REQUIRE_NOTHROW(delete volume);
        vklRelease2(vklSampler);
      }

      SECTION("XYZVdbVolumeFloat trilinear")
      {
        XYZVdbVolumeFloat *volume = nullptr;
        const int dim             = 128;
        REQUIRE_NOTHROW(
            volume = new XYZVdbVolumeFloat(
                getOpenVKLDevice(), dim, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRILINEAR);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRILINEAR);
        vklCommit2(vklSampler);
        const vec3i step(2);
        multidim_index_sequence<3> mis(volume->getDimensions() / step);
        for (const auto &offset : mis) {
          if (offset.x + 1 >= volume->getDimensions().x ||
              offset.y + 1 >= volume->getDimensions().y ||
              offset.z + 1 >= volume->getDimensions().z) {
            continue;
          }

          const auto offsetWithStep = offset * step;
          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
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
        vklRelease2(vklSampler);
      }

      SECTION("XYZVdbVolumeFloat tricubic")
      {
        XYZVdbVolumeFloat *volume = nullptr;
        const int dim             = 128;
        REQUIRE_NOTHROW(
            volume = new XYZVdbVolumeFloat(
                getOpenVKLDevice(), dim, vec3f(0.f), vec3f(1.f), repackNodes));

        VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
        VKLSampler vklSampler = vklNewSampler(vklVolume);
        vklSetInt2(vklSampler, "filter", VKL_FILTER_TRICUBIC);
        vklSetInt2(vklSampler, "gradientFilter", VKL_FILTER_TRICUBIC);
        vklCommit2(vklSampler);
        const vec3i step(2);

        // Gradient will be different around the border due to central
        // differencing, so we discard the outer layer of voxels.
        constexpr int filterRadius = 2;
        multidim_index_sequence<3> mis(
            (volume->getDimensions() - 2 * filterRadius) / step);
        for (const auto &offset : mis) {
          const auto offsetWithStep = offset * step + filterRadius;
          if (offsetWithStep.x + filterRadius >= volume->getDimensions().x ||
              offsetWithStep.y + filterRadius >= volume->getDimensions().y ||
              offsetWithStep.z + filterRadius >= volume->getDimensions().z) {
            continue;
          }

          const vec3f objectCoordinates =
              volume->transformLocalToObjectCoordinates(offsetWithStep);

          INFO("offset = " << offset.x << " " << offset.y << " " << offset.z);
          INFO("objectCoordinates = " << objectCoordinates.x << " "
                                      << objectCoordinates.y << " "
                                      << objectCoordinates.z);

          const vkl_vec3f vklGradient = vklComputeGradient(
              &vklSampler, (const vkl_vec3f *)&objectCoordinates);
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
        vklRelease2(vklSampler);
      }
    }
  }

  shutdownOpenVKL();
}

TEST_CASE("VDB volume strides", "[volume_strides]")
{
  initializeOpenVKL();

  std::vector<VKLDataCreationFlags> dataCreationFlags{VKL_DATA_DEFAULT,
                                                      VKL_DATA_SHARED_BUFFER};

  std::vector<float> strideFactors{0.f, 1.f, 1.5f, 2.f};

  for (const auto &repackNodes : {true, false}) {
    for (const auto &dcf : dataCreationFlags) {
      for (const auto &strideFactor : strideFactors) {
        std::stringstream sectionName;
        sectionName << (repackNodes ? "repackNodes=true, "
                                    : "repackNodes=false, ");
        sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                                : "VKL_DATA_SHARED_BUFFER");
        sectionName << ", stride factor: " << strideFactor;

        // can't have duplicate section names at the same level
        DYNAMIC_SECTION(sectionName.str())
        {
          WaveletVdbVolumeFloat *volume = nullptr;
          REQUIRE_NOTHROW(
              volume = new WaveletVdbVolumeFloat(getOpenVKLDevice(),
                                                 128,
                                                 vec3f(0.f),
                                                 vec3f(1.f),
                                                 repackNodes,
                                                 TemporalConfig(),
                                                 1,
                                                 dcf,
                                                 strideFactor * sizeof(float)));

          VKLVolume vklVolume   = volume->getVKLVolume(getOpenVKLDevice());
          VKLSampler vklSampler = vklNewSampler(vklVolume);
          vklCommit2(vklSampler);
          const vec3i step(2);
          multidim_index_sequence<3> mis(volume->getDimensions() / step);
          for (const auto &offset : mis) {
            const auto offsetWithStep = offset * step;

            const vec3f objectCoordinates =
                volume->transformLocalToObjectCoordinates(offsetWithStep);

            const float proceduralValue =
                volume->computeProceduralValue(objectCoordinates);

            INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y
                             << " " << offsetWithStep.z);
            INFO("objectCoordinates = " << objectCoordinates.x << " "
                                        << objectCoordinates.y << " "
                                        << objectCoordinates.z);

            test_scalar_and_vector_sampling(
                vklSampler, objectCoordinates, proceduralValue, 1e-4f);
          }

          REQUIRE_NOTHROW(delete volume);
          vklRelease2(vklSampler);
        }
      }
    }
  }

  shutdownOpenVKL();
}

void vdb_special_case_interval_iterator(
    VKLIntervalIteratorContext intervalContext,
    const vkl_vec3f &rayOrigin,
    const vkl_vec3f &rayDirection)
{
  std::vector<char> buffer(vklGetIntervalIteratorSize(&intervalContext));

  const vkl_range1f rayTRange = {0.f, inf};
  const float time            = 0.f;

  VKLIntervalIterator intervalIterator =
      vklInitIntervalIterator(&intervalContext,
                              &rayOrigin,
                              &rayDirection,
                              &rayTRange,
                              time,
                              buffer.data());

  int numIntervalsFound = 0;
  VKLInterval prevInterval;

  while (true) {
    VKLInterval interval;
    int result = vklIterateInterval(intervalIterator, &interval);
    if (!result)
      break;

    INFO("tRange = " << interval.tRange.lower << " " << interval.tRange.upper
                     << "\nvalueRange = " << interval.valueRange.lower << " "
                     << interval.valueRange.upper
                     << "\nnominalDeltaT = " << interval.nominalDeltaT);

    REQUIRE(interval.tRange.lower >= 0.f);
    REQUIRE(interval.tRange.upper >= 0.f);
    REQUIRE(interval.tRange.upper > interval.tRange.lower);

    if (numIntervalsFound > 0) {
      REQUIRE(interval.tRange.lower == prevInterval.tRange.upper);
    }

    numIntervalsFound++;
    prevInterval = interval;
  }

  REQUIRE(numIntervalsFound > 0);
}

TEST_CASE("VDB volume special cases", "[interval_iterators]")
{
  initializeOpenVKL();

  SECTION("torus interval iteration")
  {
    TestingVdbTorusVolume *volume = nullptr;
    REQUIRE_NOTHROW(volume = new TestingVdbTorusVolume());

    VKLVolume vklVolume = volume->getVKLVolume(getOpenVKLDevice());

    VKLSampler sampler = vklNewSampler(vklVolume);
    vklCommit2(sampler);

    VKLIntervalIteratorContext intervalContext =
        vklNewIntervalIteratorContext(sampler);
    vklCommit2(intervalContext);

    // failure case found from OSPRay
    {
      // intbits() representation of ray
      const uint32_t rayOrigin[] = {
          1112900070, 1116163650, 1103628776};  // 53.3769 67.6528 25.0048
      const uint32_t rayDirection[] = {
          1081551625, 1098411576, 2984533223};  // 3.862 15.5269 -6.64624e-09

      vdb_special_case_interval_iterator(intervalContext,
                                         *(vkl_vec3f *)&rayOrigin,
                                         *(vkl_vec3f *)&rayDirection);

      // additional cases based on the above testing updated divide_safe()
      // implementation

      // from rkCommon: smallest positive normal number 2^-126=0x1p-126 (needs a
      // C++17 compiler)
      static const float rkcommon_flt_min = 1.17549435e-38;

      const vkl_vec3f rayDirectionVec3f = *(vkl_vec3f *)&rayDirection;

      std::vector<float> dirZs{0.f,
                               rkcommon_flt_min,
                               1.1f * rkcommon_flt_min,
                               -0.f,
                               -rkcommon_flt_min,
                               -1.1f * rkcommon_flt_min};

      for (const auto &dirZ : dirZs) {
        vkl_vec3f dir = rayDirectionVec3f;
        dir.z         = dirZ;

        vdb_special_case_interval_iterator(
            intervalContext, *(vkl_vec3f *)&rayOrigin, dir);
      }
    }

    vklRelease2(intervalContext);
    vklRelease2(sampler);

    REQUIRE_NOTHROW(delete volume);
  }

  shutdownOpenVKL();
}
#endif
