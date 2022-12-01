// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "rkcommon/utility/multidim_index_sequence.h"
#include "sampling_utility.h"

template <typename PROCEDURAL_VOLUME_TYPE>
inline void sampling_on_vertices_vs_procedural_values(
    vec3i dimensions,
    VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
    size_t byteStride                      = 0,
    vec3i step                             = vec3i(1),
    int lowerSpan                          = 0,
    int upperSpan                          = 0)
{
  const vec3f gridOrigin(0.f);
  const vec3f gridSpacing(1.f);

  auto v = rkcommon::make_unique<PROCEDURAL_VOLUME_TYPE>(dimensions,
                                                         gridOrigin,
                                                         gridSpacing,
                                                         TemporalConfig(),
                                                         dataCreationFlags,
                                                         byteStride);

  VKLVolume vklVolume   = v->getVKLVolume(getOpenVKLDevice());
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit2(vklSampler);

  multidim_index_sequence<3> mis(v->getDimensions() / step);

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    if (coordinate_in_boundary_span(
            offsetWithStep, v->getDimensions(), lowerSpan, upperSpan)) {
      continue;
    }

    vec3f objectCoordinates =
        v->transformLocalToObjectCoordinates(offsetWithStep);

    const float proceduralValue = v->computeProceduralValue(objectCoordinates);

    INFO("offset = " << offsetWithStep.x << " " << offsetWithStep.y << " "
                     << offsetWithStep.z);
    INFO("objectCoordinates = " << objectCoordinates.x << " "
                                << objectCoordinates.y << " "
                                << objectCoordinates.z);

    test_scalar_and_vector_sampling(
        vklSampler, objectCoordinates, proceduralValue, 1e-4f);
  }

  vklRelease2(vklSampler);
}

template <typename VOLUME_TYPE>
inline void test_32bit_addressing(VKLDataCreationFlags dataCreationFlags,
                                  float strideFactor)
{
  const size_t byteStride =
      strideFactor * sizeof(typename VOLUME_TYPE::voxelType);
  INFO("byteStride = " << byteStride);

  // For GPU limit number of iterations
#ifdef OPENVKL_TESTING_GPU
  const vec3i step = vec3i(8);
#else
  const vec3i step = vec3i(2);
#endif

  sampling_on_vertices_vs_procedural_values<VOLUME_TYPE>(
      vec3i(128), dataCreationFlags, byteStride, step);
}

inline const vec3i get_dimensions_for_64_32bit_addressing(size_t byteStride)
{
  // corresponds to limits in Data.ih
  constexpr size_t maxSize32bit = 1ULL << 31;

  const size_t dim =
      size_t(std::cbrt(double(maxSize32bit) / double(byteStride))) + 1;

  if (dim * dim * dim * byteStride <= maxSize32bit) {
    throw std::runtime_error(
        "incorrect dimension computed for 64/32-bit addressing mode");
  }

  return vec3i(dim);
}

template <typename VOLUME_TYPE>
inline void test_64_32bit_addressing(VKLDataCreationFlags dataCreationFlags,
                                     float strideFactor)
{
  const size_t byteStride =
      strideFactor == 0.f
          ? sizeof(typename VOLUME_TYPE::voxelType)
          : strideFactor * sizeof(typename VOLUME_TYPE::voxelType);

  INFO("byteStride = " << byteStride);

  const vec3i dimensions = get_dimensions_for_64_32bit_addressing(byteStride);

  INFO("dimensions = " << dimensions.x << " " << dimensions.y << " "
                       << dimensions.z);

  const vec3i step = vec3i(32);

  sampling_on_vertices_vs_procedural_values<VOLUME_TYPE>(
      dimensions, dataCreationFlags, byteStride, step);
}
