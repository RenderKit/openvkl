// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../RendererHost.h"
#include "RayMarchIteratorParams.h"

namespace openvkl {
  namespace examples {

    template <class R>
    struct RayMarchIteratorShared
    {
      RayMarchIteratorShared(const Scene *scene,
                                     const RendererParams *rendererParams,
                                     const Scheduler *scheduler);
      ~RayMarchIteratorShared();

      void updateIntervalContext();
      void beforeStart();
      void afterStop();
      void beforeFrame(bool &needToClear);

      Versioned<RayMarchIteratorParams> guiParams;
      Versioned<RayMarchIteratorParams> params;  // Used by the worker.
      std::unique_ptr<VKLIntervalIteratorContext>
          intervalContext;  // Used by the worker.

     private:
      const Scene *scene{nullptr};
      const RendererParams *rendererParams{nullptr};
      const Scheduler *scheduler{nullptr};
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class RayMarchIterator final : public ScalarRenderer
    {
     public:
      RayMarchIterator(Scene &scene);
      ~RayMarchIterator();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<RayMarchIteratorParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixel(size_t seed,
                       Ray &ray,
                       vec4f &rgba,
                       float &weight) const override final;

     private:
      using Shared = RayMarchIteratorShared<RayMarchIterator>;
      std::unique_ptr<Shared> shared;
    };

    // Note: This renderer stops itself in the destructor, so it should never
    //       call virtual functions in derived classes in the render loop.
    //       We use final to ensure whoever tries to derive is aware of this.
    class RayMarchIteratorIspc final : public IspcRenderer
    {
     public:
      RayMarchIteratorIspc(Scene &scene);
      ~RayMarchIteratorIspc();

      void beforeStart() override final;
      void afterStop() override final;
      void beforeFrame(bool &needToClear) override final;

      Versioned<RayMarchIteratorParams> &getGuiParams() {
        return shared->guiParams;
      }

     protected:
      void renderPixelBlock(const vec2i &resolution,
                            uint32_t offset,
                            vec4f *rgbas,
                            float *weights) const override final;

     private:
      using Shared =
          RayMarchIteratorShared<RayMarchIteratorIspc>;
      std::unique_ptr<Shared> shared;
      void *ispcParams{nullptr};
    };

  }  // namespace examples
}  // namespace openvkl

