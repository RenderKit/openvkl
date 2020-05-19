// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// openvkl
#include "TransferFunction.h"
#include "Scene.h"
#include "openvkl/openvkl.h"
// rkcommon
#include "rkcommon/containers/AlignedVector.h"
#include "rkcommon/math/box.h"
#include "rkcommon/utility/ParameterizedObject.h"
#include "rkcommon/utility/multidim_index_sequence.h"
// std
#include <cmath>
#include <random>
#include <vector>

#define USE_STD_RANDOM 0

namespace openvkl {
  namespace examples {

    using namespace rkcommon;
    using namespace rkcommon::math;

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
      Renderer();

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

      void renderFrame(const Scene& scene);
      void renderFrame_ispc(const Scene& scene);

     protected:
      virtual vec3f renderPixel(const Scene& scene, Ray &ray, const vec4i &sampleID) = 0;

      Ray computeRay(const vec2f &screenCoords) const;
      vec4f sampleTransferFunction(const Scene& scene, float value) const;

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
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline Ray Renderer::computeRay(const vec2f &screenCoords) const
    {
      vec3f org = camPos;
      vec3f dir = dir_00 + screenCoords.x * dir_du + screenCoords.y * dir_dv;

      Ray ray;

      ray.org = org;
      ray.dir = normalize(dir);
      ray.t   = range1f(0.f, rkcommon::inf);

      return ray;
    }

    inline vec4f Renderer::sampleTransferFunction(const Scene& scene, float value) const
    {
      vec4f colorAndOpacity{0.f};

      if (std::isnan(value) || scene.tfNumColorsAndOpacities == 0) {
        return colorAndOpacity;
      }

      if (value <= scene.tfValueRange.lower) {
        return scene.tfColorsAndOpacities[0];
      }

      if (value >= scene.tfValueRange.upper) {
        return scene.tfColorsAndOpacities[scene.tfNumColorsAndOpacities-1];
      }

      // map the value into the range [0, size - 1]
      value = (value - scene.tfValueRange.lower) /
              (scene.tfValueRange.upper -
               scene.tfValueRange.lower) * (scene.tfNumColorsAndOpacities - 1.f);

      // index and fractional offset
      const int index       = floor(value);
      const float remainder = value - index;

      // the final interpolated value
      return ((1.f - remainder) * scene.tfColorsAndOpacities[index] +
              remainder *
                  scene.tfColorsAndOpacities[min(
                      index + 1,
                      int(scene.tfNumColorsAndOpacities - 1))]);
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
