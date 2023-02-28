// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <cmath>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/array3D/for_each.h"
#include "rkcommon/utility/getEnvVar.h"
#include "sampling_utility.h"

#if defined(_MSC_VER)
#include <malloc.h>  // _malloca
#endif

inline static void requireEqualsHelper(const float v1, const float v2)
{
  if (std::isnan(v1)) {
    REQUIRE((std::isnan(v1) && std::isnan(v2)));
  } else {
#ifdef __OPTIMIZE__
    // with certain optimizations, for example -ffp-contract=fast, we can see
    // some numerical differences due to differing code paths and use of fma,
    // etc.; so allow a tolerance here for those cases
    REQUIRE(v1 == Approx(v2).margin(1e-3f));
#else
    REQUIRE(v1 == v2);
#endif
  }
}

#ifdef OPENVKL_UTILITY_VDB_OPENVDB_ENABLED
#include <openvkl/utility/vdb/OpenVdbGrid.h>

static VKLVolume vdbToStructuredRegular(openvdb::FloatGrid::Ptr grid)
{
  auto _dims             = grid->evalActiveVoxelDim();
  const vec3i dimensions = vec3i(_dims.x() - 1, _dims.y() - 1, _dims.z() - 1);

  auto _bbox = grid->evalActiveVoxelBoundingBox();
  const vec3i indexOrigin =
      vec3i(_bbox.min().x(), _bbox.min().y(), _bbox.min().z());

  std::vector<float> voxels;
  voxels.reserve(rkcommon::array3D::longProduct(dimensions));

  auto accessor = grid->getAccessor();

  for (size_t k = 0; k < dimensions.z; k++) {
    for (size_t j = 0; j < dimensions.y; j++) {
      for (size_t i = 0; i < dimensions.x; i++) {
        openvdb::Coord xyz(
            indexOrigin.x + i, indexOrigin.y + j, indexOrigin.z + k);
        float value = accessor.getValue(xyz);
        voxels.push_back(value);
      }
    }
  }

  if (rkcommon::array3D::longProduct(dimensions) != voxels.size()) {
    throw std::runtime_error(
        "inconsistent number of voxels in VKLVolume conversion");
  }

  VKLVolume vklVolumeStructured =
      vklNewVolume(getOpenVKLDevice(), "structuredRegular");

  vklSetBool(vklVolumeStructured, "cellCentered", true);

  vklSetFloat(vklVolumeStructured, "background", 0.f);

  vklSetVec3i(vklVolumeStructured,
              "dimensions",
              dimensions.x,
              dimensions.y,
              dimensions.z);

  vklSetVec3i(vklVolumeStructured,
              "indexOrigin",
              indexOrigin.x,
              indexOrigin.y,
              indexOrigin.z);

  const auto &indexToObject = grid->transform().baseMap();
  if (!indexToObject->isLinear())
    throw std::runtime_error(
        "OpenVKL only supports linearly transformed volumes");

  const auto &ri2o = indexToObject->getAffineMap()->getMat4();
  const auto *i2o  = ri2o.asPointer();
  AffineSpace3f openvdbIndexToObject;
  openvdbIndexToObject.l = LinearSpace3f(vec3f(i2o[0], i2o[1], i2o[2]),
                                         vec3f(i2o[4], i2o[5], i2o[6]),
                                         vec3f(i2o[8], i2o[9], i2o[10]));
  openvdbIndexToObject.p = vec3f(i2o[12], i2o[13], i2o[14]);

  vklSetParam(vklVolumeStructured,
              "indexToObject",
              VKL_AFFINE3F,
              &openvdbIndexToObject);

  VKLData data =
      vklNewData(getOpenVKLDevice(), voxels.size(), VKL_FLOAT, voxels.data());
  vklSetData(vklVolumeStructured, "data", data);
  vklRelease(data);

  vklCommit(vklVolumeStructured);

  return vklVolumeStructured;
}
#endif

static void bounding_box_equivalence(VKLVolume vklVolume1, VKLVolume vklVolume2)
{
  vkl_box3f bb1 = vklGetBoundingBox(vklVolume1);
  vkl_box3f bb2 = vklGetBoundingBox(vklVolume2);

  REQUIRE(bb1.lower.x == bb2.lower.x);
  REQUIRE(bb1.lower.y == bb2.lower.y);
  REQUIRE(bb1.lower.z == bb2.lower.z);

  REQUIRE(bb1.upper.x == bb2.upper.x);
  REQUIRE(bb1.upper.y == bb2.upper.y);
  REQUIRE(bb1.upper.z == bb2.upper.z);
}

static void sampling_gradient_equivalence(VKLVolume vklVolume1,
                                          VKLVolume vklVolume2)
{
  // we'll test under different filter modes
  const std::vector<VKLFilter> filters = {
      VKL_FILTER_NEAREST, VKL_FILTER_TRILINEAR, VKL_FILTER_TRICUBIC};

  for (const auto &filter : filters) {
    VKLSampler sampler1 = vklNewSampler(vklVolume1);
    vklSetInt(sampler1, "filter", filter);
    vklCommit(sampler1);

    VKLSampler sampler2 = vklNewSampler(vklVolume2);
    vklSetInt(sampler2, "filter", filter);
    vklCommit(sampler2);

    // assume bounding boxes match
    vkl_box3f bbox = vklGetBoundingBox(vklVolume1);

    std::random_device rd;
    std::mt19937 eng(rd());

    std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
    std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);
    std::uniform_real_distribution<float> distZ(bbox.lower.z, bbox.upper.z);

    const size_t N = 1024;

    for (size_t i = 0; i < N; i++) {
      vkl_vec3f oc{distX(eng), distY(eng), distZ(eng)};

      const float s1 = vklComputeSample(sampler1, &oc);
      const float s2 = vklComputeSample(sampler2, &oc);

      requireEqualsHelper(s1, s2);

      const vkl_vec3f g1 = vklComputeGradient(sampler1, &oc);
      const vkl_vec3f g2 = vklComputeGradient(sampler2, &oc);

      requireEqualsHelper(g1.x, g2.x);
      requireEqualsHelper(g1.y, g2.y);
      requireEqualsHelper(g1.z, g2.z);
    }

    vklRelease(sampler1);
    vklRelease(sampler2);
  }
}

static void iterator_equivalence(VKLVolume vklVolume1,
                                 VKLVolume vklVolume2,
                                 bool matchIntervalsExactly = true)
{
  vkl_range1f range1 = vklGetValueRange(vklVolume1);
  vkl_range1f range2 = vklGetValueRange(vklVolume2);
  REQUIRE(range1.lower == range2.lower);
  REQUIRE(range1.upper == range2.upper);

  std::vector<vkl_range1f> ranges;
  ranges.push_back(
      vkl_range1f{range1.lower + 0.4f * (range1.upper - range1.lower),
                  range1.lower + 0.6f * (range1.upper - range1.lower)});
  VKLData rangesData =
      vklNewData(getOpenVKLDevice(), ranges.size(), VKL_BOX1F, ranges.data());

  VKLSampler sampler1 = vklNewSampler(vklVolume1);
  vklCommit(sampler1);

  VKLSampler sampler2 = vklNewSampler(vklVolume2);
  vklCommit(sampler2);

  VKLIntervalIteratorContext context1 = vklNewIntervalIteratorContext(sampler1);
  vklSetData(context1, "valueRanges", rangesData);
  vklCommit(context1);

  VKLIntervalIteratorContext context2 = vklNewIntervalIteratorContext(sampler2);
  vklSetData(context2, "valueRanges", rangesData);
  vklCommit(context2);

  // assume bounding boxes match
  vkl_box3f bbox   = vklGetBoundingBox(vklVolume1);
  vkl_vec3f center = {0.5f * (bbox.lower.x + bbox.upper.x),
                      0.5f * (bbox.lower.y + bbox.upper.y),
                      0.5f * (bbox.lower.z + bbox.upper.z)};

  vkl_vec3f rayOrigin = {
      bbox.lower.x - 1.f, bbox.lower.y - 1.f, bbox.lower.z - 1.f};
  vkl_vec3f rayDirection = {
      center.x - rayOrigin.x, center.y - rayOrigin.y, center.z - rayOrigin.z};
  vkl_range1f rayTRange = {0.f, inf};

  // Note: buffer will cease to exist at the end of this scope.
#if defined(_MSC_VER)
  // MSVC does not support variable length arrays, but provides a
  // safer version of alloca.
  char *buffer1 =
      static_cast<char *>(_malloca(vklGetIntervalIteratorSize(context1)));
  char *buffer2 =
      static_cast<char *>(_malloca(vklGetIntervalIteratorSize(context2)));
#else
  char *buffer1 =
      static_cast<char *>(alloca(vklGetIntervalIteratorSize(context1)));
  char *buffer2 =
      static_cast<char *>(alloca(vklGetIntervalIteratorSize(context2)));
#endif

  size_t numIterations = 0;

  VKLIntervalIterator iterator1 = vklInitIntervalIterator(
      context1, &rayOrigin, &rayDirection, &rayTRange, 0.f, buffer1);

  VKLIntervalIterator iterator2 = vklInitIntervalIterator(
      context2, &rayOrigin, &rayDirection, &rayTRange, 0.f, buffer2);

  while (true) {
    VKLInterval interval1, interval2;

    int result1 = vklIterateInterval(iterator1, &interval1);
    int result2 = vklIterateInterval(iterator2, &interval2);

    REQUIRE(result1 == result2);

    if (!result1 || !result2) {
      break;
    }

    numIterations++;

    if (matchIntervalsExactly) {
      requireEqualsHelper(interval1.tRange.lower, interval2.tRange.lower);
      requireEqualsHelper(interval1.tRange.upper, interval2.tRange.upper);

      requireEqualsHelper(interval1.valueRange.lower,
                          interval2.valueRange.lower);
      requireEqualsHelper(interval1.valueRange.upper,
                          interval2.valueRange.upper);

      requireEqualsHelper(interval1.nominalDeltaT, interval2.nominalDeltaT);
    } else {
      // if we're not matching intervals exactly, we'll only make sure we get at
      // least one interval for each iterator / volume
      break;
    }
  }

  REQUIRE(numIterations > 0);

  vklRelease(rangesData);
  vklRelease(sampler1);
  vklRelease(sampler2);
  vklRelease(context1);
  vklRelease(context2);
}

static void test_volume_equivalence(VKLVolume vklVolume1, VKLVolume vklVolume2)
{
  bounding_box_equivalence(vklVolume1, vklVolume2);
  sampling_gradient_equivalence(vklVolume1, vklVolume2);
}

static void test_volume_equivalence_rotation(VKLVolume vklVolume1,
                                             VKLVolume vklVolume2)
{
  // also verify equivalence under rotation
  const std::vector<vec3f> axes{
      {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}, {1.f, 1.f, 1.f}};

  const std::vector<float> anglesDeg{
      0.f, 30.f, 45.f, 60.f, 90.f, 120.f, 135.f, 150.f, 180.f};

  const vec3f gridOrigin(-1.f, -2.f, -3.f);
  const vec3f gridSpacing(1.f, 2.f, 3.f);
  const AffineSpace3f original =
      AffineSpace3f::translate(gridOrigin) * AffineSpace3f::scale(gridSpacing);

  for (const auto &axis : axes) {
    for (const auto &angleDeg : anglesDeg) {
      const AffineSpace3f rot =
          original * AffineSpace3f::rotate(axis, angleDeg * M_PI / 180.f);

      vklSetParam(vklVolume1, "indexToObject", VKL_AFFINE3F, &rot);
      vklCommit(vklVolume1);

      vklSetParam(vklVolume2, "indexToObject", VKL_AFFINE3F, &rot);
      vklCommit(vklVolume2);

      bounding_box_equivalence(vklVolume1, vklVolume2);
      sampling_gradient_equivalence(vklVolume1, vklVolume2);
    }
  }
}

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR && OPENVKL_DEVICE_CPU_VDB
TEST_CASE("VDB volume dense consistency", "[volume_sampling]")
{
  initializeOpenVKL();

  SECTION("procedural volumes")
  {
    const vec3i dimensions(128);
    const vec3f gridOrigin(-1.f, -2.f, -3.f);
    const vec3f gridSpacing(1.f, 2.f, 3.f);

    std::unique_ptr<WaveletVdbVolumeFloat> v1(new WaveletVdbVolumeFloat(
        getOpenVKLDevice(), dimensions, gridOrigin, gridSpacing));

    std::unique_ptr<WaveletStructuredRegularVolumeFloat> v2(
        new WaveletStructuredRegularVolumeFloat(
            dimensions, gridOrigin, gridSpacing));

    VKLVolume vklVolume1 = v1->getVKLVolume(getOpenVKLDevice());
    VKLVolume vklVolume2 = v2->getVKLVolume(getOpenVKLDevice());

    // VDB volumes use cell-centered data; therefore we must also on the
    // structuredRegular volume
    vklSetBool(vklVolume2, "cellCentered", true);
    vklCommit(vklVolume2);

    test_volume_equivalence(vklVolume1, vklVolume2);
    test_volume_equivalence_rotation(vklVolume1, vklVolume2);

    // require exact interval matches for procedurals
    iterator_equivalence(vklVolume1, vklVolume2, true);
  }

#ifdef OPENVKL_UTILITY_VDB_OPENVDB_ENABLED
  SECTION(".vdb file volumes")
  {
    std::string vdbFilename =
        utility::getEnvVar<std::string>("OPENVKL_TEST_VDB_FILENAME")
            .value_or(std::string(""));

    if (vdbFilename.empty()) {
      WARN("OPENVDB_TEST_VDB_FILENAME not set; not running .vdb tests");
      return;
    }

    INFO("testing .vdb file: " << vdbFilename);

    // load OpenVDB grid
    openvdb::initialize();

    openvdb::GridBase::Ptr gridBase{nullptr};
    openvdb::FloatGrid::Ptr grid{nullptr};

    try {
      openvdb::io::File file(vdbFilename);
      file.open();
      gridBase = file.readGrid("density");
      grid     = openvdb::gridPtrCast<openvdb::FloatGrid>(gridBase);
      file.close();
    } catch (const std::exception &e) {
      throw std::runtime_error(e.what());
    }

    // instantiate VKL `vdb` volume
    auto openvdbGrid =
        openvkl::utility::vdb::OpenVdbFloatGrid(getOpenVKLDevice(), grid);
    VKLVolume vklVolume1 = openvdbGrid.createVolume(false);

    vklSetFloat(vklVolume1, "background", 0.f);
    vklCommit(vklVolume1);

    // instantiate VKL `structuredRegular` volume
    VKLVolume vklVolume2 = vdbToStructuredRegular(grid);

    test_volume_equivalence(vklVolume1, vklVolume2);

    // .vdb files may have combinations of (rootOrigin, indexClippingBounds)
    // that cause different tree node layouts, which would impact iterator
    // interval boundaries. therefore, don't require exactness in interval
    // comparisons here.
    iterator_equivalence(vklVolume1, vklVolume2, false);
  }
#endif
}
#endif
