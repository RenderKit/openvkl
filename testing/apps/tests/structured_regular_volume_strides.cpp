// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "structured_regular_volume.h"

using namespace rkcommon;
using namespace openvkl::testing;

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
TEST_CASE("Structured regular volume strides", "[volume_strides]")
{
  initializeOpenVKL();

  std::vector<VKLDataCreationFlags> dataCreationFlags{VKL_DATA_DEFAULT,
                                                      VKL_DATA_SHARED_BUFFER};

  // fractional stride factors are not allowed, as this would break alignment
  // requirement on the underlying arrays
  std::vector<float> strideFactors{2.f, 3.f};

  for (const auto &dcf : dataCreationFlags) {
    for (const auto &strideFactor : strideFactors) {
      std::stringstream sectionName;
      sectionName << (dcf == VKL_DATA_DEFAULT ? "VKL_DATA_DEFAULT"
                                              : "VKL_DATA_SHARED_BUFFER");
      sectionName << ", stride factor: " << strideFactor;

      // can't have duplicate section names at the same level
      DYNAMIC_SECTION(sectionName.str())
      {
        SECTION("32-bit addressing")
        {
          SECTION("unsigned char")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeUChar>(
                dcf, strideFactor);
          }

          SECTION("short")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeShort>(
                dcf, strideFactor);
          }

          SECTION("unsigned short")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeUShort>(
                dcf, strideFactor);
          }

#if HALF_FLOAT_SUPPORT
          SECTION("half")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeHalf>(
                dcf, strideFactor);
          }
#endif

          SECTION("float")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeFloat>(
                dcf, strideFactor);
          }

#if DOUBLE_SUPPORT
          SECTION("double")
          {
            test_32bit_addressing<WaveletStructuredRegularVolumeDouble>(
                dcf, strideFactor);
          }
#endif
        }

        SECTION("64/32-bit addressing")
        {
          SECTION("unsigned char")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeUChar>(
                dcf, strideFactor);
          }

          SECTION("short")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeShort>(
                dcf, strideFactor);
          }

          SECTION("unsigned short")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeUShort>(
                dcf, strideFactor);
          }

#if HALF_FLOAT_SUPPORT
          SECTION("half")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeHalf>(
                dcf, strideFactor);
          }
#endif

          SECTION("float")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeFloat>(
                dcf, strideFactor);
          }

#if DOUBLE_SUPPORT
          SECTION("double")
          {
            test_64_32bit_addressing<WaveletStructuredRegularVolumeDouble>(
                dcf, strideFactor);
          }
#endif
        }
      }
    }
  }

  shutdownOpenVKL();
}
#endif
