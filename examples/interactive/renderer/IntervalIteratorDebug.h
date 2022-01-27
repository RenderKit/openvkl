// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Renderer.h"

namespace openvkl {
  namespace examples {

    struct IntervalIteratorDebugParams
    {
      float intervalResolutionHint{0.5f};
      float intervalColorScale{4.f};
      float intervalOpacity{0.25f};
      bool firstIntervalOnly{false};
      bool showIntervalBorders{false};
    };

    template <class R>
    struct IntervalIteratorDebugShared
    {
      IntervalIteratorDebugShared(const Scene *scene,
                                  const RendererParams *rendererParams,
                                  const Scheduler *scheduler);
      ~IntervalIteratorDebugShared();

      void updateIntervalContext();
      void beforeStart();
      void afterStop();
      void beforeFrame(bool &needToClear);

      Versioned<IntervalIteratorDebugParams> guiParams;
      Versioned<IntervalIteratorDebugParams> params;  // Used by the worker.
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
    class IntervalIteratorDebug final : public ScalarRenderer
    {
     public:
      IntervalIteratorDebug(Scene &scene);
      ~IntervalIteratorDebug();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<IntervalIteratorDebugParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixel(size_t seed,
                       Ray &ray,
                       vec4f &rgba,
                       float &weight) const override final;

     private:
      using Shared = IntervalIteratorDebugShared<IntervalIteratorDebug>;
      std::unique_ptr<Shared> shared;
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class IntervalIteratorDebugIspc final : public IspcRenderer
    {
     public:
      IntervalIteratorDebugIspc(Scene &scene);
      ~IntervalIteratorDebugIspc();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<IntervalIteratorDebugParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixelBlock(const vec2i &resolution,
                            uint32_t offset,
                            vec4f *rgbas,
                            float *weights) const override final;

     private:
      using Shared = IntervalIteratorDebugShared<IntervalIteratorDebugIspc>;
      std::unique_ptr<Shared> shared;
      void *ispcParams{nullptr};
    };

  }  // namespace examples
}  // namespace openvkl

