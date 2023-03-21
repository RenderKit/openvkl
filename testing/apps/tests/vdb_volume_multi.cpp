// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include "../../external/catch.hpp"
#include "gradient_utility.h"
#include "multi_attrib_utility.h"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace rkcommon;
using namespace openvkl::testing;

#if OPENVKL_DEVICE_CPU_VDB || defined(OPENVKL_TESTING_GPU)
TEST_CASE("VDB volume multiple attributes", "[volume_multi_attributes]")
{
  initializeOpenVKL();

  const vec3i dimensions(64);
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f / (128.f - 1.f));

#if defined(OPENVKL_TESTING_GPU)
  const std::vector<VKLDataCreationFlags> dataCreationFlags{VKL_DATA_DEFAULT};
#else
  const std::vector<VKLDataCreationFlags> dataCreationFlags{
      VKL_DATA_DEFAULT, VKL_DATA_SHARED_BUFFER};
#endif

  const std::vector<bool> useAOSLayouts{true, false};

// For GPU limit number of iterations
#ifdef OPENVKL_TESTING_GPU
  const vec3i step = vec3i(16);
#else
  const vec3i step = vec3i(8);
#endif

  for (const auto &repackNodes : {true, false}) {
    for (const auto &dcf : dataCreationFlags) {
      for (const auto &aos : useAOSLayouts) {
        for (auto filter : {VKL_FILTER_TRILINEAR, VKL_FILTER_TRICUBIC}) {
          std::stringstream sectionName;
          sectionName << (repackNodes ? "repackNodes=true"
                                      : "repackNodes=false");
          sectionName << " ";
          sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                                  : "VKL_DATA_SHARED_BUFFER");
          sectionName << " ";
          sectionName << (filter == VKL_FILTER_TRILINEAR
                              ? "VKL_FILTER_TRILINEAR"
                              : "VKL_FILTER_TRICUBIC");
          sectionName << " ";
          sectionName << (aos == true ? "AOS layout" : "SOA layout");

          // ignore boundary areas where the filter will cause us to interpolate
          // with the background (which may be NaN!)
          int lowerSpan = 0;
          int upperSpan = 0;

          if (filter == VKL_FILTER_TRILINEAR) {
            upperSpan = 1;
          } else if (filter == VKL_FILTER_TRICUBIC) {
            lowerSpan = 1;
            upperSpan = 2;
          }

#if !defined(OPENVKL_TESTING_GPU)
          // not supported on GPU
          DYNAMIC_SECTION(std::string("half ") + sectionName.str())
          {
            std::shared_ptr<ProceduralVdbVolumeMulti> v(
                generateMultiAttributeVdbVolumeHalf(getOpenVKLDevice(),
                                                    dimensions,
                                                    gridOrigin,
                                                    gridSpacing,
                                                    repackNodes,
                                                    dcf,
                                                    aos));

            num_attributes(v);
            sampling_on_vertices_vs_procedural_values_multi(
                v, step, lowerSpan, upperSpan, filter);

            // higher gradient tolerance for half due to precision issues
            gradients_on_vertices_vs_procedural_values_multi(v, step, 0.3f);

            for (unsigned int i = 0; i < v->getNumAttributes(); i++) {
              computed_vs_api_value_range(v, i);
              test_stream_sampling(v, i);
              test_stream_gradients(v, i);
            }

            std::vector<unsigned int> attributeIndices(v->getNumAttributes());
            std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

            test_stream_sampling_multi(v, attributeIndices);
          }
#endif

          DYNAMIC_SECTION(std::string("float ") + sectionName.str())
          {
            std::shared_ptr<ProceduralVdbVolumeMulti> v(
                generateMultiAttributeVdbVolumeFloat(getOpenVKLDevice(),
                                                     dimensions,
                                                     gridOrigin,
                                                     gridSpacing,
                                                     repackNodes,
                                                     dcf,
                                                     aos));

            num_attributes(v);
            sampling_on_vertices_vs_procedural_values_multi(
                v, step, lowerSpan, upperSpan, filter);
            gradients_on_vertices_vs_procedural_values_multi(v, step);

            for (unsigned int i = 0; i < v->getNumAttributes(); i++) {
              computed_vs_api_value_range(v, i);

#if !defined(OPENVKL_TESTING_GPU)
              test_stream_sampling(v, i);
              test_stream_gradients(v, i);
#endif
            }

            std::vector<unsigned int> attributeIndices(v->getNumAttributes());
            std::iota(attributeIndices.begin(), attributeIndices.end(), 0);

#if !defined(OPENVKL_TESTING_GPU)
            test_stream_sampling_multi(v, attributeIndices);
#endif
          }
        }
      }
    }
  }

  shutdownOpenVKL();
}
#endif
