// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"
// std
#include <algorithm>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"
// ispc
#include "Renderer_ispc.h"

namespace openvkl {
  namespace examples {

    Renderer::Renderer() = default;

    Renderer::~Renderer()
    {
      ispc::Renderer_freeRenderer(ispcEquivalent);
    }

    void Renderer::commit()
    {
      spp = getParam<int>("spp", 1);
    }

    void Renderer::setCamera(const vec3f &pos,
                             const vec3f &dir,
                             const vec3f &up,
                             float aspect,
                             float fovy)
    {
      camPos = pos;

      dir_du = normalize(cross(dir, up));
      dir_dv = cross(dir_du, dir);

      float imgPlane_size_y = 2.f * tanf(fovy / 2.f * M_PI / 180.);
      float imgPlane_size_x = imgPlane_size_y * aspect;

      dir_du *= imgPlane_size_x;
      dir_dv *= imgPlane_size_y;

      dir_00 = dir - .5f * dir_du - .5f * dir_dv;

      ispc::Renderer_setCamera(ispcEquivalent,
                               (ispc::vec3f &)camPos,
                               (ispc::vec3f &)dir_00,
                               (ispc::vec3f &)dir_du,
                               (ispc::vec3f &)dir_dv);
    }

    void Renderer::setPixelRange(const region2i &newPixelRange)
    {
      pixelRange = newPixelRange;
    }

    void Renderer::setFrameSize(const vec2i &dims)
    {
      pixelIndices = index_sequence_2D(dims);
      pixelRange = region2i(vec2i(0), vec2i(dims));

      const auto numPixels = pixelIndices.total_indices();
      framebuffer.resize(numPixels);
      accum_r.resize(numPixels);
      accum_g.resize(numPixels);
      accum_b.resize(numPixels);

      ispc::Renderer_setFrameBuffer(ispcEquivalent,
                                    (ispc::vec3f *)framebuffer.data(),
                                    accum_r.data(),
                                    accum_g.data(),
                                    accum_b.data());
    }

    vec2i Renderer::frameSize() const
    {
      return pixelIndices.dimensions();
    }

    void Renderer::resetAccumulation()
    {
      std::fill(accum_r.begin(), accum_r.end(), 0.f);
      std::fill(accum_g.begin(), accum_g.end(), 0.f);
      std::fill(accum_b.begin(), accum_b.end(), 0.f);
      frameID = 0;

      ispc::Renderer_setFrameID(ispcEquivalent, frameID);
    }

    const FrameBuffer &Renderer::frameBuffer() const
    {
      return framebuffer;
    }

    void Renderer::renderFrame(const Scene& scene)
    {
      auto fbDims = pixelIndices.dimensions();
      auto rangeDims = pixelRange.size();

      for (int i = 0; i < spp; ++i) {
        float accumScale = 1.f / (frameID + 1);

        tasking::parallel_for(rangeDims.long_product(), [&](size_t i) {
          auto pixel = vec2i(i % rangeDims.x, i / rangeDims.x) + pixelRange.lower;

          vec2f screen(pixel.x * rcp(float(fbDims.x)),
                       pixel.y * rcp(float(fbDims.y)));

          Ray ray = computeRay(screen);
          const vec3f color = renderPixel(scene, ray, vec4i(pixel.x, pixel.y, frameID, fbDims.x));

          auto fbIndex = pixelIndices.flatten(pixel);
          float &ar    = accum_r[fbIndex];
          float &ag    = accum_g[fbIndex];
          float &ab    = accum_b[fbIndex];

          ar += color.x;
          ag += color.y;
          ab += color.z;

          framebuffer[fbIndex] = vec3f(ar, ag, ab) * accumScale;

          // linear to sRGB color space conversion
          framebuffer[fbIndex] = vec3f(pow(framebuffer[fbIndex].x, 1.f / 2.2f),
                                       pow(framebuffer[fbIndex].y, 1.f / 2.2f),
                                       pow(framebuffer[fbIndex].z, 1.f / 2.2f));
        });

        frameID++;
      }
    }

    void Renderer::renderFrame_ispc(const Scene& scene)
    {
      vec2i fbDims = pixelIndices.dimensions();
      auto rangeDims = pixelRange.size();
      ispc::vec2i fbDimsISPC{fbDims.x, fbDims.y};
      ispc::vec2i rangeDimsISPC{rangeDims.x, rangeDims.y};
      ispc::vec2i pixelOffsetISPC{pixelRange.lower.x, pixelRange.lower.y};

      const size_t numJobs =
          area(pixelRange) / ispc::Renderer_pixelsPerJob();

      for (int i = 0; i < spp; ++i) {
        float accumScale = 1.f / (frameID + 1);

        tasking::parallel_for(numJobs, [&](size_t i) {
          ispc::Renderer_renderPixel(ispcEquivalent,
                                     reinterpret_cast<const ispc::Scene*>(&scene),
                                     fbDimsISPC,
                                     rangeDimsISPC,
                                     pixelOffsetISPC,
                                     frameID,
                                     accumScale,
                                     i);
        });

        frameID++;
      }
    }
  }  // namespace examples
}  // namespace openvkl
