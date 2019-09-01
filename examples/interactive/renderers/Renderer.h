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
#include <vector>

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
      Renderer()          = default;
      virtual ~Renderer() = default;

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

  }  // namespace examples
}  // namespace openvkl
