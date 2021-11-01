// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct RayMarchIteratorRendererParams
    {
      float samplingRate{1.f};
    };

    template <class R>
    struct RayMarchIteratorRendererShared
    {
      RayMarchIteratorRendererShared(const Scene *scene,
                                     const RendererParams *rendererParams,
                                     const Scheduler *scheduler);
      ~RayMarchIteratorRendererShared();

      void updateIntervalContext();
      void beforeStart();
      void afterStop();
      void beforeFrame(bool &needToClear);

      Versioned<RayMarchIteratorRendererParams> guiParams;
      Versioned<RayMarchIteratorRendererParams> params;  // Used by the worker.
      VKLIntervalIteratorContext intervalContext{
          nullptr};  // Used by the worker.

     private:
      const Scene *scene{nullptr};
      const RendererParams *rendererParams{nullptr};
      const Scheduler *scheduler{nullptr};
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class RayMarchIteratorRenderer final : public ScalarRenderer
    {
     public:
      RayMarchIteratorRenderer(Scene &scene);
      ~RayMarchIteratorRenderer();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<RayMarchIteratorRendererParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixel(size_t seed,
                       Ray &ray,
                       vec4f &rgba,
                       float &weight) const override final;

     private:
      using Shared = RayMarchIteratorRendererShared<RayMarchIteratorRenderer>;
      std::unique_ptr<Shared> shared;
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class RayMarchIteratorRendererIspc final : public IspcRenderer
    {
     public:
      RayMarchIteratorRendererIspc(Scene &scene);
      ~RayMarchIteratorRendererIspc();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<RayMarchIteratorRendererParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixelBlock(const vec2i &resolution,
                            uint32_t offset,
                            vec4f *rgbas,
                            float *weights) const override final;

     private:
      using Shared =
          RayMarchIteratorRendererShared<RayMarchIteratorRendererIspc>;
      std::unique_ptr<Shared> shared;
      void *ispcParams{nullptr};
    };

  }  // namespace examples
}  // namespace openvkl

