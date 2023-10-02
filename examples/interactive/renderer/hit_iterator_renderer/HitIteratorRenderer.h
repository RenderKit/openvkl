// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "HitIteratorRendererParams.h"
#include "../RendererHost.h"

namespace openvkl {
  namespace examples {

    template <class R>
    struct HitIteratorRendererShared
    {
      HitIteratorRendererShared(const Scene *scene,
                                const RendererParams *rendererParams,
                                const Scheduler *scheduler);
      ~HitIteratorRendererShared();

      void updateHitContext();
      void beforeStart();
      void afterStop();
      void beforeFrame(bool &needToClear);

      Versioned<HitIteratorRendererParams> guiParams;
      Versioned<HitIteratorRendererParams> params;  // Used by the worker.
      std::unique_ptr<VKLHitIteratorContext> hitContext;  // Used by the worker.

     private:
      const Scene *scene{nullptr};
      const RendererParams *rendererParams{nullptr};
      const Scheduler *scheduler{nullptr};
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class HitIteratorRenderer final : public ScalarRenderer
    {
     public:
      HitIteratorRenderer(Scene &scene);
      ~HitIteratorRenderer();
      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<HitIteratorRendererParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixel(size_t seed,
                       Ray &ray,
                       vec4f &rgba,
                       float &weight) const override final;

     private:
      using Shared = HitIteratorRendererShared<HitIteratorRenderer>;
      std::unique_ptr<Shared> shared;
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class HitIteratorRendererIspc final : public IspcRenderer
    {
     public:
      HitIteratorRendererIspc(Scene &scene);
      ~HitIteratorRendererIspc();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<HitIteratorRendererParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixelBlock(const vec2i &resolution,
                            uint32_t offset,
                            vec4f *rgbas,
                            float *weights) const override final;

     private:
      using Shared = HitIteratorRendererShared<HitIteratorRendererIspc>;
      std::unique_ptr<Shared> shared;
      void *ispcParams{nullptr};
    };

  }  // namespace examples
}  // namespace openvkl
