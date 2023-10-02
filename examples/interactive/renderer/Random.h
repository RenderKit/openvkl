// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

#if USE_STD_RANDOM
    inline float getRandomUniform()
    {
      static thread_local std::minstd_rand rng;
      static std::uniform_real_distribution<float> distribution{0.f, 1.f};

      return distribution(rng);
    }

    struct RandomSTD
    {
      RandomSTD(const unsigned int &, const unsigned int &)
      {
      }

      vec2f getFloats()
      {
        return vec2f{getRandomUniform(), getRandomUniform()};
      }
    };

    using RNG = RandomSTD;
#else

    // TEA - Random numbers based on Tiny Encryption Algorithm //
    const unsigned int NUM_ROUNDS = 8;
    inline void tea8(unsigned int &v0, unsigned int &v1)
    {
      unsigned int sum{0};

      for (int i = 0; i < NUM_ROUNDS; i++) {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
      }
    }

    struct RandomTEA
    {
      RandomTEA(const unsigned int &idx, const unsigned int &seed) : v0(idx), v1(seed) {}

      vec2f getFloats()
      {
        tea8(v0, v1);
        const float tofloat = 2.3283064365386962890625e-10f;  // 1/2^32
        return vec2f{v0 * tofloat, v1 * tofloat};
      }

      unsigned int v0, v1;
    };

    using RNG = RandomTEA;
#endif

  }  // namespace examples
}  // namespace openvkl