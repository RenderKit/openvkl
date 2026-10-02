// Copyright 2026 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "wrappers.h"

using namespace openvkl::testing;

// only the grid changes on recommit; sampler parameters (e.g. filter) are not
// updated by a volume recommit, thus are kept unchanged

static const VKLFilter filters[] = {
    VKL_FILTER_NEAREST, VKL_FILTER_LINEAR, VKL_FILTER_CUBIC};

static VKLSampler newSampler(VKLVolume volume, VKLFilter filter)
{
  VKLSampler sampler = vklNewSampler(volume);
  vklSetInt(sampler, "filter", filter);
  vklCommit(sampler);
  return sampler;
}

// sampler created before the recommit must match a fresh sampler
static void requireSamplesOfFreshSampler(VKLVolume volume,
                                         VKLSampler sampler,
                                         VKLFilter filter,
                                         unsigned int attributeIndex = 0)
{
  VKLSampler freshSampler = newSampler(volume, filter);

  const vkl_box3f bbox = vklGetBoundingBox(volume);
  std::mt19937 eng(0);
  std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
  std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
  std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

  size_t numValid = 0;
  for (size_t i = 0; i < 64; i++) {
    const vkl_vec3f oc{distX(eng), distY(eng), distZ(eng)};
    const float expected =
        vklComputeSampleWrapper(&freshSampler, &oc, attributeIndex, 0.f);
    const float sample =
        vklComputeSampleWrapper(&sampler, &oc, attributeIndex, 0.f);

    INFO("filter = " << filter << ", attributeIndex = " << attributeIndex);
    INFO("objectCoordinates = " << oc.x << " " << oc.y << " " << oc.z);
    if (std::isnan(expected)) {
      REQUIRE(std::isnan(sample));
    } else {
      REQUIRE(sample == expected);
      numValid++;
    }
  }
  REQUIRE(numValid > 0);

  vklRelease(freshSampler);
}

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR && !defined(OPENVKL_TESTING_GPU)
static VKLData newVoxelData(const TestingStructuredVolume &v)
{
  std::vector<unsigned char> voxels;
  std::vector<float> time;
  std::vector<uint32_t> tuvIndex;
  v.generateVoxels(voxels, time, tuvIndex);
  return vklNewData(getOpenVKLDevice(),
                    v.getDimensions().long_product(),
                    v.getVoxelType(),
                    voxels.data());
}
#endif

#if OPENVKL_DEVICE_CPU_VDB || defined(OPENVKL_TESTING_GPU)
TEST_CASE("VDB volume recommit with existing sampler", "[volume_sampling]")
{
  initializeOpenVKL();

  SECTION("sparse")
  {
    for (VKLFilter filter : filters) {
      WaveletVdbVolumeFloat v(
          getOpenVKLDevice(), vec3i(64), vec3f(0.f), vec3f(1.f));
      VKLVolume volume   = v.getVKLVolume(getOpenVKLDevice());
      VKLSampler sampler = newSampler(volume, filter);

      // transform is part of the grid
      const AffineSpace3f indexToObject =
          AffineSpace3f::translate(vec3f(1.f, 2.f, 3.f)) *
          AffineSpace3f::scale(vec3f(0.5f, 1.f, 2.f));
      vklSetParam(volume, "indexToObject", VKL_AFFINE3F, &indexToObject);
      vklCommit(volume);

      requireSamplesOfFreshSampler(volume, sampler, filter);

      vklRelease(sampler);
    }
  }

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR && !defined(OPENVKL_TESTING_GPU)
  // structuredRegular is implemented as dense VDB on CPU
  SECTION("dense")
  {
    for (VKLFilter filter : filters) {
      WaveletStructuredRegularVolumeFloat v(
          vec3i(8, 9, 10), vec3f(0.f), vec3f(1.f));
      VKLVolume volume   = v.getVKLVolume(getOpenVKLDevice());
      VKLSampler sampler = newSampler(volume, filter);

      // other dimensions, voxel type and number of attributes
      const vec3i dimensions(12, 10, 9);
      const VKLData attributes[] = {
          newVoxelData(WaveletStructuredRegularVolumeUChar(
              dimensions, vec3f(0.f), vec3f(1.f))),
          newVoxelData(XYZStructuredRegularVolumeFloat(
              dimensions, vec3f(0.f), vec3f(1.f)))};
      VKLData data = vklNewData(getOpenVKLDevice(), 2, VKL_DATA, attributes);
      vklSetVec3i(
          volume, "dimensions", dimensions.x, dimensions.y, dimensions.z);
      vklSetData(volume, "data", data);
      vklRelease(data);
      for (VKLData d : attributes)
        vklRelease(d);
      vklCommit(volume);

      requireSamplesOfFreshSampler(volume, sampler, filter, 0);
      requireSamplesOfFreshSampler(volume, sampler, filter, 1);

      vklRelease(sampler);
    }
  }
#endif

  shutdownOpenVKL();
}
#endif
