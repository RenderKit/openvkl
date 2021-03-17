// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/observer.h"
#include "openvkl/openvkl.h"
#include "usda.h"
// rkcommon
#include "rkcommon/math/box.h"
#include "rkcommon/utility/CodeTimer.h"
// std
#include <algorithm>
#include <vector>

namespace openvkl {
  namespace vdb_util {
    using rkcommon::math::box3f;
    using rkcommon::math::range1f;

    struct InnerNode
    {
      // The object-space bounding box of this node.
      box3f bbox;

      // The minimum and maximum value over all voxels in this node.
      std::vector<range1f> valueRange;
    };

    inline std::vector<InnerNode> getInnerNodes(VKLVolume volume,
                                                uint32_t maxDepth)
    {
      std::vector<InnerNode> nodes;

      VKLObserver observer = vklNewVolumeObserver(volume, "InnerNode");
      vklSetInt(observer, "maxDepth", maxDepth);
      vklCommit(observer);

      const float *buffer =
          static_cast<const float *>(vklMapObserver(observer));
      if (!buffer)
        throw std::runtime_error("cannot map inner node observer buffer.");

      const size_t numAttributes       = vklGetNumAttributes(volume);
      const size_t numFloatsPerElement = 6 + 2 * numAttributes;
      const size_t numElements         = vklGetObserverNumElements(observer);

      assert(vklGetObserverElementType(observer) == VKL_OBJECT);
      assert(vklGetObserverElementSize(observer) ==
             (numFloatsPerElement * sizeof(float)));

      nodes.resize(numElements);
      for (size_t i = 0; i < numElements; ++i) {
        InnerNode &node = nodes.at(i);
        const float *f  = buffer + i * numFloatsPerElement;
        node.bbox.lower = vec3f(f[0], f[1], f[2]);
        node.bbox.upper = vec3f(f[3], f[4], f[5]);
        node.valueRange.resize(numAttributes);
        for (size_t a = 0; a < numAttributes; ++a) {
          node.valueRange.at(a).lower = f[6 + 2 * a];
          node.valueRange.at(a).upper = f[7 + 2 * a];
        }
      }
      vklUnmapObserver(observer);
      vklRelease(observer);

      return nodes;
    }

    // Export a .usda file containing the inner nodes of the given volume.
    inline void exportInnerNodes(const std::string &path,
                                 uint32_t maxDepth,
                                 VKLVolume volume)
    {
      std::cout << "Computing inner nodes ..." << std::flush;
      rkcommon::utility::CodeTimer innerNodeTimer;
      innerNodeTimer.start();
      const auto nodes = openvkl::vdb_util::getInnerNodes(volume, maxDepth);
      innerNodeTimer.stop();
      std::cout << " [" << innerNodeTimer.milliseconds() << "ms]" << std::endl;

      std::cout << "Exporting " << nodes.size() << " inner nodes to " << path
                << " ..." << std::flush;
      innerNodeTimer = rkcommon::utility::CodeTimer();
      innerNodeTimer.start();

      // Compute number of digits we need for the bbox names.
      const int swidth =
          std::max(static_cast<int>(std::log10(nodes.size())) + 1, 1);

      {
        std::ofstream f(path.c_str(), std::ios::out);

        {
          usda::Header header(f);
          usda::Attribute(f, "doc", usda::String("Open VKL inner nodes"));
          usda::Attribute(f, "metersPerUnit", 1);
          usda::Attribute(f, "upAxis", usda::String("Y"));
        }

        {
          usda::Scope scope(f, "Scope", "InnerNodes");
          size_t i = 0;
          for (const auto &node : nodes) {
            std::ostringstream osName;
            osName << "InnerNode_" << std::setw(swidth) << std::setfill('0')
                   << i++;
            {
              const vec3f o = node.bbox.lower;
              const vec3f d = node.bbox.upper - node.bbox.lower;

              usda::Scope cube(f, "Cube", osName.str());
              usda::Attribute(f,
                              "float3[]",
                              "extent",
                              usda::List<vec3f>({vec3f(-0.5f), vec3f(0.5f)}));
              usda::Attribute(f, "double", "size", 1);
              usda::Attribute(
                  f,
                  "matrix4d",
                  "xformOp:transform",
                  usda::Tuple<vec4f>(
                      {vec4f(d.x, 0, 0, 0),
                       vec4f(0, d.y, 0, 0),
                       vec4f(0, 0, d.z, 0),
                       vec4f(0.5f + o.x, 0.5f + o.y, 0.5f + o.z, 1)}));
              usda::Attribute(f,
                              "uniform token[]",
                              "xformOpOrder",
                              usda::List<usda::String>(
                                  {usda::String("xformOp:transform")}));
              std::vector<vec2f> valueRanges(node.valueRange.size());
              std::transform(
                  node.valueRange.begin(),
                  node.valueRange.end(),
                  valueRanges.begin(),
                  [](const range1f &vr) { return vec2f(vr.lower, vr.upper); });
              usda::Attribute(f,
                              "float2[]",
                              "valueRange",
                              usda::List<range1f>(valueRanges));
            }
          }
        }
      }

      innerNodeTimer.stop();
      std::cout << " [" << innerNodeTimer.milliseconds() << "ms]" << std::endl;
    }

  }  // namespace vdb_util
}  // namespace openvkl
