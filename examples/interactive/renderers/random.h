// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

// ospcommon
#include "ospcommon/math/vec.h"
// std
#include <random>

#define USE_STD_RANDOM 0

namespace openvkl {
  namespace examples {

    using namespace ospcommon::math;

#if USE_STD_RANDOM
    static float getRandomUniform()
    {
      static thread_local std::minstd_rand rng;
      static std::uniform_real_distribution<float> distribution{0.f, 1.f};

      return distribution(rng);
    }

    template <typename T>
    struct RandomSTD
    {
      RandomSTD(const T &, const T &) {}

      vec2f getFloats()
      {
        return vec2f{getRandomUniform(), getRandomUniform()};
      }
    };

    using RNG = RandomSTD<unsigned int>;
#else

    // TEA - Random numbers based on Tiny Encryption Algorithm //

    template <typename T, int NUM_ROUNDS = 8>
    inline void tea8(T &v0, T &v1)
    {
      T sum{0};

      for (int i = 0; i < NUM_ROUNDS; i++) {
        sum += 0x9e3779b9;
        v0 += ((v1 << 4) + 0xa341316c) ^ (v1 + sum) ^ ((v1 >> 5) + 0xc8013ea4);
        v1 += ((v0 << 4) + 0xad90777d) ^ (v0 + sum) ^ ((v0 >> 5) + 0x7e95761e);
      }
    }

    template <typename T, int NUM_TEA_ROUNDS = 8>
    struct RandomTEA
    {
      RandomTEA(const T &idx, const T &seed) : v0(idx), v1(seed) {}

      vec2f getFloats()
      {
        tea8<T, NUM_TEA_ROUNDS>(v0, v1);
        const float tofloat = 2.3283064365386962890625e-10f;  // 1/2^32
        return vec2f{v0 * tofloat, v1 * tofloat};
      }

      T v0, v1;
    };

    using RNG = RandomTEA<unsigned int, 8>;
#endif

  }  // namespace examples
}  // namespace openvkl
