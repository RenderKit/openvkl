// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Ray.h"
#include "Renderer.h"
#include "framebuffer/FramebufferCpu.h"

#pragma once

namespace openvkl {
  namespace examples {
    /*
     * A renderer based on the host.
     * This base class expects the renderer to fill the front buffer
     * in renderFrameImpl().
     */
    class RendererHost : public Renderer
    {
     public:
      RendererHost(Scene &scene);
      ~RendererHost();

      const Framebuffer &getFramebuffer(size_t w, size_t h) override final;

     protected:
      FramebufferCpu framebuffer;
    };

    /*
     * A renderer based on the host, which does not use any vectorization.
     */
    class ScalarRenderer : public RendererHost
    {
     public:
      ScalarRenderer(Scene &scene);

     protected:
      void renderFrameImpl(bool clearFramebuffer) override final;
      virtual void renderPixel(size_t seed,
                               Ray &ray,
                               vec4f &rgba,
                               float &weight) const = 0;
    };

    /*
     * A renderer based on the host, but using vectorization with ISPC.
     */
    class IspcRenderer : public RendererHost
    {
     public:
      IspcRenderer(Scene &scene);
      ~IspcRenderer();
      void beforeStart() override;
      void beforeFrame(bool &needToClear) override;

     protected:
      void renderFrameImpl(bool clearFramebuffer) override final;
      virtual void renderPixelBlock(const vec2i &resolution,
                                    uint32_t block,
                                    vec4f *rgbas,
                                    float *weights) const = 0;

     protected:
      void *ispcScene{nullptr};
    };
  }  // namespace examples
}  // namespace openvkl
