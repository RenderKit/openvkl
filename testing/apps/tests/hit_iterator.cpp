// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "wrappers.h"

using namespace rkcommon;
using namespace openvkl::testing;

void scalar_hit_iteration(VKLVolume volume,
                          const unsigned int attributeIndex,
                          const float time,
                          const std::vector<float> &isoValues,
                          const std::vector<float> &expectedTValues,
                          const vkl_vec3f &origin = vkl_vec3f{0.5f, 0.5f, -1.f},
                          const vkl_vec3f &direction = vkl_vec3f{0.f, 0.f, 1.f})
{
  vkl_range1f tRange{0.f, inf};

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  VKLData valuesData = vklNewData(
      getOpenVKLDevice(), isoValues.size(), VKL_FLOAT, isoValues.data());

  VKLHitIteratorContext hitContext = vklNewHitIteratorContext(sampler);

  vklSetInt(hitContext, "attributeIndex", attributeIndex);

  vklSetData(hitContext, "values", valuesData);
  vklRelease(valuesData);

  vklCommit(hitContext);

  VKLHit *hitsBuffer   = allocate<VKLHit>(maxNumHits);
  char *iteratorBuffer = allocate<char>(vklGetHitIteratorSize(&hitContext));

  int *hitCount = allocate<int>(1);

  auto testIteratorFunc = [=]() {
    VKLHitIterator iterator = vklInitHitIterator(&hitContext,
                                                 &origin,
                                                 &direction,
                                                 &tRange,
                                                 time,
                                                 (void *)iteratorBuffer);

    VKLHit hit;
    while (vklIterateHit(iterator, &hit)) {
      hitsBuffer[*hitCount] = hit;

      *hitCount += 1;
      if (*hitCount >= maxNumHits) {
        break;
      }
    }
  };

  executeTestFunction(testIteratorFunc);

  deallocate(iteratorBuffer);

  if (*hitCount >= maxNumHits) {
    WARN("Hit iterations reached max number of hits: " << maxNumHits);
  }

  if (*hitCount > isoValues.size()) {
    WARN(
        "found too many hits; this can occur at the volume boundaries when "
        "interpolating with zero background values (instead of NaN), for "
        "example");
  }

  for (int i = 0; i < *hitCount; i++) {
    VKLHit hit = hitsBuffer[i];

    INFO("hit t = " << hit.t << ", sample = " << hit.sample);

    REQUIRE(hit.sample == isoValues[i]);
    REQUIRE(hit.t == Approx(expectedTValues[i]).margin(1e-3f));
  }

  REQUIRE(*hitCount == isoValues.size());

  deallocate(hitsBuffer);
  deallocate(hitCount);

  vklRelease(hitContext);
  vklRelease(sampler);
}

TEST_CASE("Hit iterator", "[hit_iterators]")
{
  initializeOpenVKL();

  // for a unit cube physical grid [(0,0,0), (1,1,1)]
  const vec3i dimensions(128);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

  // default isovalues
  std::vector<float> defaultIsoValues;
  std::vector<float> defaultExpectedTValues;

  for (float f = 0.1f; f < 1.f; f += 0.1f) {
    defaultIsoValues.push_back(f);
    defaultExpectedTValues.push_back(f + 1.f);
  }

  SECTION("scalar hit iteration")
  {
#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
    SECTION("structured volumes: single attribute")
    {
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(dimensions, gridOrigin, gridSpacing));

      VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

      const unsigned int attributeIndex = 0;

      scalar_hit_iteration(vklVolume,
                           attributeIndex,
                           0.f,
                           defaultIsoValues,
                           defaultExpectedTValues);
    }

    SECTION("structured volumes: multi attribute")
    {
      std::shared_ptr<TestingStructuredVolumeMulti> v(
          generateMultiAttributeStructuredRegularVolume(dimensions,
                                                        gridOrigin,
                                                        gridSpacing,
                                                        TemporalConfig(),
                                                        VKL_DATA_DEFAULT,
                                                        true));

      VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

      const unsigned int numAttributes = vklGetNumAttributes(vklVolume);

      // setup appropriate rays for the procedural volume
      // note that we'll skip the first attribute and focus only on the x-, y-,
      // and z- varying attributes
      assert(numAttributes == 4);
      std::vector<vkl_vec3f> origins{
          {-1.f, 0.5f, 0.5f}, {0.5f, -1.f, 0.5f}, {0.5f, 0.5f, -1.f}};

      std::vector<vkl_vec3f> directions{
          {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};

      for (unsigned int i = 1; i < numAttributes; i++) {
        scalar_hit_iteration(vklVolume,
                             i,
                             0.f,
                             defaultIsoValues,
                             defaultExpectedTValues,
                             origins[i - 1],
                             directions[i - 1]);
      }
    }
#endif

#if OPENVKL_DEVICE_CPU_VDB
    SECTION("VDB volumes: single attribute")
    {
      for (const auto &repackNodes : {true, false}) {
        std::stringstream sectionName;
        sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

        DYNAMIC_SECTION(sectionName.str())
        {
          std::unique_ptr<ZVdbVolumeFloat> v(
              new ZVdbVolumeFloat(getOpenVKLDevice(),
                                  dimensions,
                                  gridOrigin,
                                  gridSpacing,
                                  repackNodes));

          VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

          const unsigned int attributeIndex = 0;

          scalar_hit_iteration(vklVolume,
                               attributeIndex,
                               0.f,
                               defaultIsoValues,
                               defaultExpectedTValues);
        }
      }
    }

    SECTION("VDB volumes: multi attribute")
    {
      for (const auto &repackNodes : {true, false}) {
        std::stringstream sectionName;
        sectionName << (repackNodes ? "repackNodes=true" : "repackNodes=false");

        DYNAMIC_SECTION(sectionName.str())
        {
          std::shared_ptr<ProceduralVdbVolumeMulti> v(
              generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                   dimensions,
                                                   gridOrigin,
                                                   gridSpacing,
                                                   repackNodes,
                                                   VKL_DATA_DEFAULT,
                                                   true,
                                                   TemporalConfig()));

          VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

          const unsigned int numAttributes = vklGetNumAttributes(vklVolume);

          // setup appropriate rays for the procedural volume
          // note that we'll skip the first attribute and focus only on the
          // x-, y-, and z- varying attributes
          assert(numAttributes == 4);
          std::vector<vkl_vec3f> origins{
              {-1.f, 0.5f, 0.5f}, {0.5f, -1.f, 0.5f}, {0.5f, 0.5f, -1.f}};

          std::vector<vkl_vec3f> directions{
              {1.f, 0.f, 0.f}, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f}};

          for (unsigned int i = 1; i < numAttributes; i++) {
            scalar_hit_iteration(vklVolume,
                                 i,
                                 0.f,
                                 defaultIsoValues,
                                 defaultExpectedTValues,
                                 origins[i - 1],
                                 directions[i - 1]);
          }
        }
      }
    }
#endif

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR
    SECTION(
        "structured volumes: isovalues at grid accelerator macrocell "
        "boundaries")
    {
      // macrocells are currently 16**3
      std::unique_ptr<ZProceduralVolume> v(
          new ZProceduralVolume(vec3i(128), vec3f(0.f), vec3f(1.f)));

      VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

      const unsigned int attributeIndex = 0;

      std::vector<float> macroCellBoundaries;
      std::vector<float> macroCellTValues;

      for (int i = 0; i < 128; i += 16) {
        macroCellBoundaries.push_back(float(i));
        macroCellTValues.push_back(float(i) + 1.f);
      }

      scalar_hit_iteration(vklVolume,
                           attributeIndex,
                           0.f,
                           macroCellBoundaries,
                           macroCellTValues);
    }

    SECTION("structured volumes: single voxel layer edge case")
    {
      std::unique_ptr<ZProceduralVolume> v(new ZProceduralVolume(
          vec3i(17, 17, 17), vec3f(0.f), vec3f(1.f) / vec3f(16.f)));

      const unsigned int attributeIndex = 0;

      VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());
      // We're tracing from the back, so we'll hit the isovalues in reverse
      // order
      std::vector<float> reversedIsovalues = defaultIsoValues;
      std::reverse(reversedIsovalues.begin(), reversedIsovalues.end());

      scalar_hit_iteration(vklVolume,
                           attributeIndex,
                           0.f,
                           reversedIsovalues,
                           defaultExpectedTValues,
                           vkl_vec3f{0.5f, 0.5f, 2.f},
                           // vkl_vec3f{8.f, 8.f, 18.f},
                           vkl_vec3f{0.f, 0.f, -1.f});
    }

    SECTION("structured volumes: time varying")
    {
      std::vector<TemporalConfig> temporalConfigs{
          {TemporalConfig::Structured, 4}, {TemporalConfig::Unstructured, 4}};

      for (const auto &temporalConfig : temporalConfigs) {
        std::unique_ptr<ZProceduralVolume> v(new ZProceduralVolume(
            dimensions, gridOrigin, gridSpacing, temporalConfig));

        VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

        const unsigned int attributeIndex = 0;

        const std::vector<float> times{0.f, 0.2f, 0.4f, 0.6f};

        for (const float time : times) {
          std::vector<float> isoValuesTime;

          for (const auto &iso : defaultIsoValues) {
            // procedural function is:  (1.f - time) * objectCoordinates.z;
            isoValuesTime.push_back((1.f - time) * iso);
          }

          scalar_hit_iteration(vklVolume,
                               attributeIndex,
                               time,
                               isoValuesTime,
                               defaultExpectedTValues);
        }
      }
    }
#endif

#if OPENVKL_DEVICE_CPU_UNSTRUCTURED
    SECTION("unstructured volumes")
    {
      std::unique_ptr<ZUnstructuredProceduralVolume> v(
          new ZUnstructuredProceduralVolume(
              dimensions, gridOrigin, gridSpacing, VKL_HEXAHEDRON, false));

      VKLVolume vklVolume = v->getVKLVolume(getOpenVKLDevice());

      const unsigned int attributeIndex = 0;

      scalar_hit_iteration(vklVolume,
                           attributeIndex,
                           0.f,
                           defaultIsoValues,
                           defaultExpectedTValues);
    }
#endif
  }

  shutdownOpenVKL();
}
