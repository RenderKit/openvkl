// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TestingVolume.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace testing {

    /*
     * Our basic noise primitive.
     * Heavily based on Perlin's Java reference implementation of
     * the improved perlin noise paper from Siggraph 2002 from here
     * https://mrl.nyu.edu/~perlin/noise/
     */
    class PerlinNoise
    {
      struct PerlinNoiseData
      {
        PerlinNoiseData();

        inline int operator[](size_t idx) const
        {
          return p[idx];
        }
        int p[512];
      };

      static PerlinNoiseData p;

      static inline float smooth(float t)
      {
        return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
      }

      static inline float grad(int hash, float x, float y, float z)
      {
        const int h   = hash & 15;
        const float u = h < 8 ? x : y;
        const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
        return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
      }

     public:
      static float noise(vec3f q, float frequency = 8.f)
      {
        float x     = q.x * frequency;
        float y     = q.y * frequency;
        float z     = q.z * frequency;
        const int X = (int)floor(x) & 255;
        const int Y = (int)floor(y) & 255;
        const int Z = (int)floor(z) & 255;
        x -= floor(x);
        y -= floor(y);
        z -= floor(z);
        const float u = smooth(x);
        const float v = smooth(y);
        const float w = smooth(z);
        const int A   = p[X] + Y;
        const int B   = p[X + 1] + Y;
        const int AA  = p[A] + Z;
        const int BA  = p[B] + Z;
        const int BB  = p[B + 1] + Z;
        const int AB  = p[A + 1] + Z;

        return lerp(
            w,
            lerp(
                v,
                lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
                lerp(
                    u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
            lerp(v,
                 lerp(u,
                      grad(p[AA + 1], x, y, z - 1),
                      grad(p[BA + 1], x - 1, y, z - 1)),
                 lerp(u,
                      grad(p[AB + 1], x, y - 1, z - 1),
                      grad(p[BB + 1], x - 1, y - 1, z - 1))));
      }
    };

    // -----------------------------------------------------------------------------
    // Hypertexture helpers.
    // -----------------------------------------------------------------------------

    /*
     * Turbulence - noise at various scales.
     */
    inline float turbulence(const vec3f &p, float base_freqency, int octaves)
    {
      float value = 0.f;
      float scale = 1.f;
      for (int o = 0; o < octaves; ++o) {
        value += PerlinNoise::noise(scale * p, base_freqency) / scale;
        scale *= 2.f;
      }
      return value;
    };

    /*
     * A torus indicator function.
     */
    inline bool torus(vec3f X, float R, float r)
    {
      const float tmp = sqrtf(X.x * X.x + X.z * X.z) - R;
      return tmp * tmp + X.y * X.y - r * r < 0.f;
    }

    /*
     * Turbulent torus indicator function.
     * We use the turbulence() function to distort space.
     */
    inline bool turbulentTorus(vec3f p, float R, float r)
    {
      const vec3f X = 2.f * p - vec3f(1.f);
      return torus((1.4f + 0.4 * turbulence(p, 12.f, 12)) * X, R, r);
    }

    struct TestingVdbTorusVolume : public TestingVolume
    {
      TestingVdbTorusVolume(){};

      void generateVKLVolume(VKLDevice device)
      {
        std::vector<uint32_t> level;
        std::vector<vec3i> origin;
        std::vector<VKLData> data;

        constexpr uint32_t leafRes = 8;
        constexpr uint32_t N       = domainRes / leafRes;
        constexpr size_t numLeaves = N * static_cast<size_t>(N) * N;

        level.reserve(numLeaves);
        origin.reserve(numLeaves);
        data.reserve(numLeaves);

        computedValueRange = range1f(rkcommon::math::empty);

        std::vector<float> leaf(leafRes * (size_t)leafRes * leafRes, 0.f);
        for (uint32_t z = 0; z < N; ++z)
          for (uint32_t y = 0; y < N; ++y)
            for (uint32_t x = 0; x < N; ++x) {
              std::vector<range1f> leafValueRange(leafRes);
              rkcommon::tasking::parallel_for(leafRes, [&](uint32_t vz) {
                for (uint32_t vy = 0; vy < leafRes; ++vy)
                  for (uint32_t vx = 0; vx < leafRes; ++vx) {
                    float v = 0.f;
                    const vec3f p((x * leafRes + vx + 0.5f) / (float)domainRes,
                                  (y * leafRes + vy + 0.5f) / (float)domainRes,
                                  (z * leafRes + vz + 0.5f) / (float)domainRes);
                    if (turbulentTorus(p, 0.75f, 0.375f))
                      v = 0.5f + 0.5f * PerlinNoise::noise(p, 12);

                    const size_t idx = vx * (size_t)leafRes * leafRes +
                                       vy * (size_t)leafRes + vz;
                    leafValueRange[vz].extend(v);
                    leaf.at(idx) = v;
                  }
              });
              // reduction
              for (auto &vr : leafValueRange)
                leafValueRange[0].extend(vr);

              if (leafValueRange[0].lower != 0.f ||
                  leafValueRange[0].upper != 0.f) {
                level.push_back(3);
                origin.push_back(vec3i(x * leafRes, y * leafRes, z * leafRes));
                data.emplace_back(
                    vklNewData(device,
                               leafRes * (size_t)leafRes * leafRes,
                               VKL_FLOAT,
                               leaf.data()));
              }

              computedValueRange.extend(leafValueRange[0]);
            }

        if (level.empty())
          throw std::runtime_error("vdb volume is empty.");

        volume = vklNewVolume(device, "vdb");

        VKLData levelData =
            vklNewData(device, level.size(), VKL_UINT, level.data());
        vklSetData(volume, "node.level", levelData);
        vklRelease(levelData);

        VKLData originData =
            vklNewData(device, origin.size(), VKL_VEC3I, origin.data());
        vklSetData(volume, "node.origin", originData);
        vklRelease(originData);

        VKLData dataData =
            vklNewData(device, data.size(), VKL_DATA, data.data());
        for (auto &d : data) {
          vklRelease(d);
        }
        vklSetData(volume, "node.data", dataData);
        vklRelease(dataData);

        std::vector<uint32_t> format(data.size(), VKL_FORMAT_DENSE_ZYX);
        VKLData formatData =
            vklNewData(device, format.size(), VKL_UINT, format.data());
        vklSetData(volume, "node.format", formatData);
        vklRelease(formatData);

        vklCommit(volume);
      };

      range1f getComputedValueRange() const
      {
        if (computedValueRange.empty()) {
          throw std::runtime_error(
              "computedValueRange only available after VKL volume is "
              "generated");
        }

        return computedValueRange;
      };

     private:
      range1f computedValueRange = range1f(rkcommon::math::empty);

      static constexpr uint32_t domainRes = 128;
    };
  }  // namespace testing
}  // namespace openvkl
