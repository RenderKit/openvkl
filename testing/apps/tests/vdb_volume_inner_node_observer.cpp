// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/multidim_index_sequence.h"

#include "openvkl/utility/vdb/InnerNodes.h"

using namespace openvkl;
using namespace openvkl::testing;

inline range1f sample_value_range_over_inner_node(VKLSampler vklSampler,
                                                  uint32_t maxDepth,
                                                  box3f boundingBox,
                                                  unsigned int attributeIndex)
{
  range1f valueRange = empty;

  const uint32_t voxelDim = vklVdbLevelRes(maxDepth + 1);

  vec3i step(1);

  // at maxDepth == 0, inner nodes will represent 4096^3 voxels, so in the
  // interest of run time use a larger step for sampling value ranges...
  if (maxDepth == 0) {
    step = vec3i(16);
  }

  multidim_index_sequence<3> mis(voxelDim / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    const vec3f oc =
        boundingBox.lower + offsetWithStep / float(voxelDim) *
                                (boundingBox.upper - boundingBox.lower);

    const float sample =
        vklComputeSample(vklSampler, (const vkl_vec3f *)&oc, attributeIndex);

    valueRange.extend(sample);
  }

  return valueRange;
}

inline void inner_node_tests(VKLVolume vklVolume, vec3i dimensions)
{
  const unsigned int numAttributes = vklGetNumAttributes(vklVolume);

  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklSetInt(vklSampler, "filter", VKL_FILTER_NEAREST);
  vklSetInt(vklSampler, "gradientFilter", VKL_FILTER_NEAREST);
  vklCommit(vklSampler);

  // the inner node observer returns _child_ nodes of nodes at the specified
  // maxDepth. so innerNodes requested at maxDepth=0 will be at level=1, etc.
  // this means it only makes sense to observe inner nodes one level _above_ the
  // leaf level!
  const uint32_t leafLevel = vklVdbNumLevels() - 1;

  for (uint32_t maxDepth = 0; maxDepth < leafLevel; maxDepth++) {
    const uint32_t innerNodeRes = vklVdbLevelRes(maxDepth + 1);

    INFO("testing at VDB maxDepth "
         << maxDepth << " with inner node resolution (voxel dim) "
         << innerNodeRes);

    const std::vector<openvkl::utility::vdb::InnerNode> innerNodes =
        openvkl::utility::vdb::getInnerNodes(vklVolume, maxDepth);

    INFO("found " << innerNodes.size() << " inner nodes at maxDepth "
                  << maxDepth);

    // verify expected number of inner nodes
    REQUIRE(innerNodes.size() ==
            ((dimensions + innerNodeRes - 1) / innerNodeRes).long_product());

    // verify inner node value ranges for all attributes
    box3f innerNodesExtents = empty;

    for (const auto &innerNode : innerNodes) {
      INFO("innerNode bbox = " << innerNode.bbox);

      REQUIRE(innerNode.valueRange.size() == numAttributes);

      innerNodesExtents.extend(innerNode.bbox);

      for (unsigned int a = 0; a < numAttributes; a++) {
        range1f sampledValueRange = sample_value_range_over_inner_node(
            vklSampler, maxDepth, innerNode.bbox, a);

        REQUIRE(sampledValueRange.lower >= innerNode.valueRange[a].lower);
        REQUIRE(sampledValueRange.upper <= innerNode.valueRange[a].upper);
      }
    }

    // verify inner node bounding boxes
    //
    // the VKL bounding box should be entirely contained in the extents of all
    // inner nodes; if we're observing leaf nodes then the bounding boxes should
    // be identical
    vkl_box3f vklBoundingBox = vklGetBoundingBox(vklVolume);

    if (maxDepth == leafLevel - 1) {
      // observing leaf level nodes
      REQUIRE(vklBoundingBox.lower.x == innerNodesExtents.lower.x);
      REQUIRE(vklBoundingBox.lower.y == innerNodesExtents.lower.y);
      REQUIRE(vklBoundingBox.lower.z == innerNodesExtents.lower.z);

      REQUIRE(vklBoundingBox.upper.x == innerNodesExtents.upper.x);
      REQUIRE(vklBoundingBox.upper.y == innerNodesExtents.upper.y);
      REQUIRE(vklBoundingBox.upper.z == innerNodesExtents.upper.z);
    } else {
      // observing inner level nodes
      REQUIRE(vklBoundingBox.lower.x >= innerNodesExtents.lower.x);
      REQUIRE(vklBoundingBox.lower.y >= innerNodesExtents.lower.y);
      REQUIRE(vklBoundingBox.lower.z >= innerNodesExtents.lower.z);

      REQUIRE(vklBoundingBox.upper.x <= innerNodesExtents.upper.x);
      REQUIRE(vklBoundingBox.upper.y <= innerNodesExtents.upper.y);
      REQUIRE(vklBoundingBox.upper.z <= innerNodesExtents.upper.z);
    }
  }

  vklRelease(vklSampler);
}

TEST_CASE("VDB volume inner node observer", "[volume_observers]")
{
  initializeOpenVKL();

  const vec3i dimensions(256);

  const std::vector<vec3f> gridOrigins{vec3f(0.f), vec3f(100.f, 200.f, 300.f)};
  const std::vector<vec3f> gridSpacings{vec3f(1.f), vec3f(1.f, 2.f, 3.f)};

  for (const auto &gridOrigin : gridOrigins) {
    for (const auto &gridSpacing : gridSpacings) {
      for (const auto &repackNodes : {true, false}) {
        // single attributes volumes
        {
          auto v =
              rkcommon::make_unique<WaveletVdbVolumeFloat>(getOpenVKLDevice(),
                                                           dimensions,
                                                           gridOrigin,
                                                           gridSpacing,
                                                           repackNodes);

          inner_node_tests(v->getVKLVolume(getOpenVKLDevice()), dimensions);
        }

        {
          auto v = rkcommon::make_unique<XYZVdbVolumeFloat>(getOpenVKLDevice(),
                                                            dimensions,
                                                            gridOrigin,
                                                            gridSpacing,
                                                            repackNodes);

          inner_node_tests(v->getVKLVolume(getOpenVKLDevice()), dimensions);
        }

        // multi-attribute volume
        {
          std::unique_ptr<ProceduralVdbVolumeMulti> v(
              generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                   dimensions,
                                                   gridOrigin,
                                                   gridSpacing,
                                                   repackNodes,
                                                   VKL_DATA_DEFAULT,
                                                   true));

          inner_node_tests(v->getVKLVolume(getOpenVKLDevice()), dimensions);
        }
      }
    }
  }

  shutdownOpenVKL();
}
