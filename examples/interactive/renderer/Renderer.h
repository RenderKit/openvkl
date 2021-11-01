// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Framebuffer.h"
#include "Ray.h"
#include "Scene.h"
#include "Scheduler.h"

#include <rkcommon/math/vec.h>
#include <atomic>
#include <mutex>
#include <thread>

namespace openvkl {
  namespace examples {

    using GLuint = unsigned int;

    /*
     * All renderers must implement this interface.
     */
    class Renderer
    {
     public:
      Renderer(Scene &scene);
      virtual ~Renderer();

      const Scene &getScene() const
      {
        return scene;
      }

      Scene &getScene()
      {
        return scene;
      }

      /*
       * Retrieve the current framebuffer, either in raw form or as an
       * Open GL texture handle.
       * Note that these may be implemented in terms of each other, depending
       * on whether the rendering happens on the GPU or CPU.
       *
       * The inputs w and h are just hints; the renderer is free to ignore them.
       * The caller should check the output for its resolution.
       */
      virtual const Framebuffer &getFramebuffer(size_t w, size_t h) = 0;

      /*
       * This method is called by the Scheduler just before the renderer
       * is started.
       * It can be used to set up resources that depend on the volume.
       */
      virtual void beforeStart() {}
      virtual void afterStop() {}

      /*
       * This method must be called by renderers just before the frame is
       * rendered. It has two jobs: to update parameter structs from the
       * GUI, and to determine if the framebuffer needs to be cleared.
       *
       * Derived classes should call this method on their parent class!
       */
      virtual void beforeFrame(bool &needToClear);

      /*
       * And a callback for when the frame has finished.
       * Again, renderers should call the base class method if they override.
       */
      virtual void afterFrame(){};

     protected:
      Scene &scene;
      const Scheduler &scheduler;

      // Derived classes may use the entities below in
      // renderFrame and methods called by renderFrame.
      // They are kept up to date by beforeFrame(...).
      Versioned<ArcballCamera> cam;
      Versioned<RendererParams> rendererParams;

     protected:
      void renderFrame()
      {
        bool clearFramebuffer = false;
        beforeFrame(clearFramebuffer);
        renderFrameImpl(clearFramebuffer);
        afterFrame();
      }

      /*
       * This method is called only from the scheduler and may or may not
       * run in a separate thread.
       *
       * Its job is to update the framebuffer.
       */
      virtual void renderFrameImpl(bool clearFramebuffer) = 0;

      vec4f sampleTransferFunction(float value) const;

     private:
      friend class Scheduler;
      friend class Scheduler::Synchronous;
      friend class Scheduler::Asynchronous;

      // Used from the asynchronous scheduler
      std::thread renderThread;
      std::atomic_bool run{false};
    };

    /*
     * A renderer based on the host.
     * This base class expects the renderer to fill the front buffer
     * in renderFrameImpl().
     */
    class HostRenderer : public Renderer
    {
     public:
      HostRenderer(Scene &scene);
      ~HostRenderer();

      const Framebuffer &getFramebuffer(size_t w, size_t h) override final;

     protected:
      Framebuffer framebuffer;
    };

    /*
     * A renderer based on the host, which does not use any vectorization.
     */
    class ScalarRenderer : public HostRenderer
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
    class IspcRenderer : public HostRenderer
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
