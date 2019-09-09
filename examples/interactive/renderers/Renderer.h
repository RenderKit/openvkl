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
#include "TransferFunction.h"
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
      Renderer(VKLVolume volume);

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

      // Transfer function setup //

      void setTransferFunction(const TransferFunction &transferFunction);

      // Isosurfaces setup //

      void setIsovalues(const std::vector<float> &isovalues);

      // Render a frame //

      void renderFrame();
      void renderFrame_ispc();

     protected:

      void updateSamplesMask();

      virtual vec3f renderPixel(Ray &ray, const vec4i &sampleID) = 0;

      Ray computeRay(const vec2f &screenCoords) const;

      vec4f sampleTransferFunction(float value) const;

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

      // Transfer function data //

      TransferFunction transferFunction;

      // Isosurfaces data //

      std::vector<float> isovalues{-1.f, 0.f, 1.f};

      // Renderer data //
      VKLVolume volume;
      box3f volumeBounds;
      VKLSamplesMask samplesMask{nullptr};
      void *ispcEquivalent{nullptr};
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

    inline vec4f Renderer::sampleTransferFunction(float value) const
    {
      vec4f colorAndOpacity{0.f};

      if (isnan(value) || transferFunction.colorsAndOpacities.size() == 0) {
        return colorAndOpacity;
      }

      if (value <= transferFunction.valueRange.lower) {
        return transferFunction.colorsAndOpacities.front();
      }

      if (value >= transferFunction.valueRange.upper) {
        return transferFunction.colorsAndOpacities.back();
      }

      // map the value into the range [0, size - 1]
      value = (value - transferFunction.valueRange.lower) /
              (transferFunction.valueRange.upper -
               transferFunction.valueRange.lower) *
              (transferFunction.colorsAndOpacities.size() - 1.f);

      // index and fractional offset
      const int index       = floor(value);
      const float remainder = value - index;

      // the final interpolated value
      return ((1.f - remainder) * transferFunction.colorsAndOpacities[index] +
              remainder *
                  transferFunction.colorsAndOpacities[min(
                      index + 1,
                      int(transferFunction.colorsAndOpacities.size() - 1))]);
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

    struct RandomSTD
    {
      template <typename T>
      RandomSTD(const T &, const T &) {}

      vec2f getFloats()
      {
        return vec2f{getRandomUniform(), getRandomUniform()};
      }
    };

    using RNG = RandomSTD;
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
