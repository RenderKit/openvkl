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

// openvkl
#include "openvkl/openvkl.h"
// ospcommon
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/math/box.h"
#include "ospcommon/utility/ParameterizedObject.h"
#include "ospcommon/utility/multidim_index_sequence.h"
// std
#include <random>
#include <vector>

#define USE_STD_RANDOM 0

namespace openvkl {
  namespace examples {

    using namespace ospcommon;
    using namespace ospcommon::math;

    using FrameBuffer  = containers::AlignedVector<vec3f>;
    using ColorChannel = containers::AlignedVector<float>;

    struct Ray
    {
      vec3f org;
      vec3f dir;
      range1f t;
    };

    struct Renderer : public utility::ParameterizedObject
    {
      Renderer() = default;
      virtual ~Renderer();

      // Parameters //

      virtual void commit();

      // Camera setup //

      void setCamera(const vec3f &pos,
                     const vec3f &dir,
                     const vec3f &up,
                     float aspect,
                     float fovy = 60.f);

      // Framebuffer access //

      void setFrameSize(const vec2i &dims);
      vec2i frameSize() const;
      void resetAccumulation();
      const FrameBuffer &frameBuffer() const;

      // Render a frame //

      void renderFrame(VKLVolume volume, VKLSamplesMask mask);
      void renderFrame_ispc(VKLVolume volume, VKLSamplesMask mask);

     protected:
      virtual vec3f renderPixel(VKLVolume volume,
                                const box3f &volumeBounds,
                                VKLSamplesMask mask,
                                Ray &ray,
                                const vec4i &sampleID) = 0;

      Ray computeRay(const vec2f &screenCoords) const;

      // Camera data //

      vec3f camPos;
      vec3f dir_00;
      vec3f dir_du;
      vec3f dir_dv;

      // Frame data //

      index_sequence_2D pixelIndices{vec2i(0)};
      FrameBuffer framebuffer;
      ColorChannel accum_r;
      ColorChannel accum_g;
      ColorChannel accum_b;
      int spp{1};
      int frameID{0};

      // Renderer data //
      void *ispcEquivalent{nullptr};
      range1f voxelRange;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline Ray Renderer::computeRay(const vec2f &screenCoords) const
    {
      vec3f org = camPos;
      vec3f dir = dir_00 + screenCoords.x * dir_du + screenCoords.y * dir_dv;

      Ray ray;

      ray.org = org;
      ray.dir = normalize(dir);
      ray.t   = range1f(0.f, ospcommon::inf);

      return ray;
    }

    ///////////////////////////////////////////////////////////////////////////
    // RNG ////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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
