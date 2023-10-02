// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "structured_regular_volume.h"

using namespace rkcommon;
using namespace openvkl::testing;

#if OPENVKL_DEVICE_CPU_STRUCTURED_REGULAR || defined(OPENVKL_TESTING_GPU)
TEST_CASE("Structured regular volume sampling", "[volume_sampling]")
{
  initializeOpenVKL();

  // no strides for these tests
  const float strideFactor = 0.f;

#ifdef OPENVKL_TESTING_GPU
  // For GPU we must use VKL_DATA_DEFAULT. To use VKL_DATA_SHARED_BUFFER we
  // would need to have buffers in USM Shared memory.
  const VKLDataCreationFlags dcf = VKL_DATA_DEFAULT;
#else
  // use shared buffers to minimize memory usage and improve run time
  const VKLDataCreationFlags dcf = VKL_DATA_SHARED_BUFFER;
#endif

  SECTION("32-bit addressing")
  {
    SECTION("unsigned char")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeUChar>(dcf,
                                                                 strideFactor);
    }

    SECTION("short")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeShort>(dcf,
                                                                 strideFactor);
    }

    SECTION("unsigned short")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeUShort>(dcf,
                                                                  strideFactor);
    }

#if HALF_FLOAT_SUPPORT
    SECTION("half")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeHalf>(dcf,
                                                                strideFactor);
    }
#endif

    SECTION("float")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeFloat>(dcf,
                                                                 strideFactor);
    }

#if DOUBLE_SUPPORT
    SECTION("double")
    {
      test_32bit_addressing<WaveletStructuredRegularVolumeDouble>(dcf,
                                                                  strideFactor);
    }
#endif
  }

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
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

  // these are necessarily longer-running tests, so should maybe be split out
  // into a "large" test suite later.
  SECTION("64-bit addressing")
  {
#if DOUBLE_SUPPORT
    // only do double tests here for now; larger grids incur even more grid
    // accelerator build overhead, which we need to resolve.
    SECTION("double")
    {
      const int lowerSpan = 0;
      const int upperSpan = 1;

      sampling_on_vertices_vs_procedural_values<
          WaveletStructuredRegularVolumeDouble>(vec3i(16385, 16385, 2),
                                                dcf,
                                                0,
                                                vec3i(32, 32, 1),
                                                lowerSpan,
                                                upperSpan);
    }
#endif
  }

  shutdownOpenVKL();
}
#endif
