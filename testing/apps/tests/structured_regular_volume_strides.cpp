// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <sstream>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "structured_regular_volume.h"

using namespace rkcommon;
using namespace openvkl::testing;

template <typename VOXEL_TYPE>
inline void test_32bit_addressing(VKLDataCreationFlags dataCreationFlags,
                                  float strideFactor)
{
  using VOLUME_TYPE =
      typename ProceduralStructuredRegularVolumes<VOXEL_TYPE>::Wavelet;

  const size_t byteStride = strideFactor * sizeof(VOXEL_TYPE);

  INFO("byteStride = " << byteStride);

  sampling_on_vertices_vs_procedural_values<VOLUME_TYPE>(
      vec3i(128), dataCreationFlags, byteStride);
}

template <typename VOXEL_TYPE>
inline void test_64_32bit_addressing(VKLDataCreationFlags dataCreationFlags,
                                     float strideFactor)
{
  using VOLUME_TYPE =
      typename ProceduralStructuredRegularVolumes<VOXEL_TYPE>::Wavelet;

  const size_t byteStride = strideFactor == 0.f
                                ? sizeof(VOXEL_TYPE)
                                : strideFactor * sizeof(VOXEL_TYPE);
  INFO("byteStride = " << byteStride);

  // corresponds to limits in Data.ih
  constexpr size_t maxSize32bit = 1ULL << 31;

  const size_t dim =
      size_t(std::cbrt(double(maxSize32bit) / double(byteStride))) + 1;
  INFO("dim = " << dim);

  if (dim * dim * dim * byteStride <= maxSize32bit) {
    throw std::runtime_error(
        "incorrect dimension computed for 64/32-bit addressing mode");
  }

  sampling_on_vertices_vs_procedural_values<VOLUME_TYPE>(
      vec3i(dim), dataCreationFlags, byteStride, 16);
}

TEST_CASE("Structured regular volume strides", "[volume_strides]")
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);

  std::vector<VKLDataCreationFlags> dataCreationFlags{VKL_DATA_DEFAULT,
                                                      VKL_DATA_SHARED_BUFFER};

  std::vector<float> strideFactors{0.f, 1.f, 1.5f, 2.f};

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
            test_32bit_addressing<unsigned char>(dcf, strideFactor);
          }

          SECTION("short")
          {
            test_32bit_addressing<short>(dcf, strideFactor);
          }

          SECTION("unsigned short")
          {
            test_32bit_addressing<unsigned short>(dcf, strideFactor);
          }

          SECTION("float")
          {
            test_32bit_addressing<float>(dcf, strideFactor);
          }

          SECTION("double")
          {
            test_32bit_addressing<double>(dcf, strideFactor);
          }
        }

        SECTION("64/32-bit addressing")
        {
          SECTION("unsigned char")
          {
            test_64_32bit_addressing<unsigned char>(dcf, strideFactor);
          }

          SECTION("short")
          {
            test_64_32bit_addressing<short>(dcf, strideFactor);
          }

          SECTION("unsigned short")
          {
            test_64_32bit_addressing<unsigned short>(dcf, strideFactor);
          }

          SECTION("float")
          {
            test_64_32bit_addressing<float>(dcf, strideFactor);
          }

          SECTION("double")
          {
            test_64_32bit_addressing<double>(dcf, strideFactor);
          }
        }
      }
    }
  }
}
