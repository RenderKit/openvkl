// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <algorithm>
#include <random>
#include "../common/simd.h"
#include "openvkl_testing.h"
#include "rkcommon/utility/random.h"

/*
 * Utilities for our benchmarking suite.
 */

/*
 * Traits classes for API programming models.
 * These allow differentiating between benchmark implementations.
 */
namespace programming_model {

  struct Scalar
  {
  };

  template <int W>
  struct Vector
  {
  };

  template <unsigned int N>
  struct Stream
  {
  };

  template <unsigned int M>
  struct ScalarM
  {
  };

  template <unsigned int M, int W>
  struct VectorM
  {
  };

  template <unsigned int M, unsigned int N>
  struct StreamM
  {
  };

}  // namespace programming_model

/*
 * Coordinate generators for point sampling APIs (vklComputeSample,
 * vklComputeGradient).
 *
 * The required interface is:
 *
 * const char * name(): A string that is used to generate test names.
 * vkl_vec3f getNextSamplePos(): The next sample position.
 */
namespace coordinate_generator {
  using openvkl::vvec3fn;

  /*
   * The Fixed coordinate generator always generates the same point.
   */
  struct Fixed
  {
    inline static constexpr const char *name()
    {
      return "Fixed";
    }

    inline static constexpr float value()
    {
      return 0.1701f;
    }

    explicit inline Fixed(vkl_box3f const & /*bbox*/) {}

    template <unsigned int N>
    inline void getNextN(vkl_vec3f *pos)
    {
      if (firstCall) {
        std::fill(pos, pos + N, vkl_vec3f{value(), value(), value()});
        firstCall = false;
      }
    }

    template <int W>
    inline void getNextV(vvec3fn<W> *pos)
    {
      if (firstCall) {
        std::fill(pos->x.v, pos->x.v + W, value());
        std::fill(pos->y.v, pos->y.v + W, value());
        std::fill(pos->z.v, pos->z.v + W, value());
        firstCall = false;
      }
    }

    bool firstCall{true};
  };

  /*
   * The Random coordinate generator will generate points inside the
   * volume bounding box.
   * Note: This is currently non-deterministic!
   */
  struct Random
  {
    using Dist = rkcommon::utility::pcg32_biased_float_distribution;

    inline static constexpr const char *name()
    {
      return "Random";
    }

    explicit inline Random(vkl_box3f const &bbox)
        : rd(),
          distX(rd(), 0, bbox.lower.x, bbox.upper.x),
          distY(rd(), 0, bbox.lower.y, bbox.upper.y),
          distZ(rd(), 0, bbox.lower.z, bbox.upper.z)
    {
    }

    template <unsigned int N>
    inline void getNextN(vkl_vec3f *pos)
    {
      for (unsigned int i = 0; i < N; ++i) {
        pos[i].x = distX();
        pos[i].y = distY();
        pos[i].z = distZ();
      }
    }

    template <int W>
    inline void getNextV(vvec3fn<W> *pos)
    {
      for (int i = 0; i < W; ++i) {
        pos->x[i] = distX();
        pos->y[i] = distY();
        pos->z[i] = distZ();
      }
    }

    std::random_device rd;  // Generate seeds randomly.
    Dist distX;
    Dist distY;
    Dist distZ;
  };

}  // namespace coordinate_generator

/*
 * A convenience wrapper that takes an Api class and registers it as a
 * benchmark.
 */
template <class Api>
inline benchmark::internal::Benchmark *registerBenchmark()
{
  return benchmark::RegisterBenchmark(Api::name().c_str(), Api::run);
}

