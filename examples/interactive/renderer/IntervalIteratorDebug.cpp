// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "IntervalIteratorDebug.h"
#include "IntervalIteratorDebug_ispc.h"

namespace openvkl {
  namespace examples {

    template <class R>
    IntervalIteratorDebugShared<R>::IntervalIteratorDebugShared(
        const Scene *scene,
        const RendererParams *rendererParams,
        const Scheduler *scheduler)
        : scene{scene}, rendererParams{rendererParams}, scheduler{scheduler}
    {
    }

    template <class R>
    IntervalIteratorDebugShared<R>::~IntervalIteratorDebugShared()
    {
      assert(!intervalContext);
    }

    template <class R>
    void IntervalIteratorDebugShared<R>::updateIntervalContext()
    {
      vklSetInt2(
          *intervalContext, "attributeIndex", rendererParams->attributeIndex);

      vklSetFloat2(*intervalContext,
                   "intervalResolutionHint",
                   params->intervalResolutionHint);

      // set interval context value ranges based on transfer function positive
      // opacity intervals, if we have any
      VKLData valueRangesData = nullptr;

      const std::vector<range1f> valueRanges =
          rendererParams->transferFunction.getPositiveOpacityValueRanges();

      if (!valueRanges.empty()) {
        valueRangesData = vklNewData(getOpenVKLDevice(),
                                     valueRanges.size(),
                                     VKL_BOX1F,
                                     valueRanges.data());
      }

      vklSetData2(*intervalContext, "valueRanges", valueRangesData);

      if (valueRangesData) {
        vklRelease(valueRangesData);
      }

      vklCommit2(*intervalContext);
    }

    template <class R>
    void IntervalIteratorDebugShared<R>::beforeStart()
    {
      assert(!intervalContext);
      VKLSampler sampler = scene->volume.getSampler();
      intervalContext    = rkcommon::make_unique<VKLIntervalIteratorContext>(
          vklNewIntervalIteratorContext(sampler));
      updateIntervalContext();
    }

    template <class R>
    void IntervalIteratorDebugShared<R>::afterStop()
    {
      if (intervalContext) {
        vklRelease2(*intervalContext);
        intervalContext = nullptr;
      }
    }

    template <class R>
    void IntervalIteratorDebugShared<R>::beforeFrame(bool &needToClear)
    {
      scheduler->locked(guiParams, [&]() {
        needToClear |= params.updateIfChanged(guiParams);
      });

      // Renderer params contain the attribute index. This is why
      // we update the interval context when the parent changed.
      if (needToClear) {
        updateIntervalContext();
      }
    }

    // -------------------------------------------------------------------------

    IntervalIteratorDebug::IntervalIteratorDebug(Scene &scene)
        : ScalarRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
    }

    IntervalIteratorDebug::~IntervalIteratorDebug()
    {
      scheduler.stop(*this);
    }

    void IntervalIteratorDebug::beforeStart()
    {
      ScalarRenderer::beforeStart();
      shared->beforeStart();
    }

    void IntervalIteratorDebug::afterStop()
    {
      ScalarRenderer::afterStop();
      shared->afterStop();
    }

    void IntervalIteratorDebug::beforeFrame(bool &needToClear)
    {
      ScalarRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);
    }

    void IntervalIteratorDebug::renderPixel(size_t seed,
                                            Ray &ray,
                                            vec4f &rgba,
                                            float &weight) const
    {
      const box3f volumeBounds = scene.volume.getBounds();
      ray.t = intersectRayBox(ray.org, ray.dir, volumeBounds);

      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      void *intervalIteratorBuffer =
          alloca(vklGetIntervalIteratorSize(shared->intervalContext.get()));

      VKLIntervalIterator iterator =
          vklInitIntervalIterator(shared->intervalContext.get(),
                                  (vkl_vec3f *)&ray.org,
                                  (vkl_vec3f *)&ray.dir,
                                  &tRange,
                                  rendererParams->time,
                                  intervalIteratorBuffer);

      VKLInterval interval;
      int intervalCount = 0;

      vec3f color(0.f);
      float alpha = 0.f;

      const auto &params = shared->params;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
        intervalCount++;

        const float dt = interval.tRange.upper - interval.tRange.lower;

        const float normalizedValue =
            rendererParams->transferFunction.valueRange.lower +
            float(intervalCount) / params->intervalColorScale *
                (rendererParams->transferFunction.valueRange.upper -
                 rendererParams->transferFunction.valueRange.lower);

        vec4f sampleColorAndOpacity = sampleTransferFunction(normalizedValue);

        sampleColorAndOpacity.w = params->intervalOpacity;

        float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

        sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

        if (params->showIntervalBorders && dt < interval.nominalDeltaT) {
          sampleColorAndOpacity = vec4f(1.f);
          clampedOpacity        = 1.f;
        }

        color = color + (1.f - alpha) * vec3f(sampleColorAndOpacity);
        alpha = alpha + (1.f - alpha) * clampedOpacity;

        if (params->firstIntervalOnly) {
          break;
        }
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    // -------------------------------------------------------------------------

    IntervalIteratorDebugIspc::IntervalIteratorDebugIspc(Scene &scene)
        : IspcRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
      ispcParams = ispc::IntervalIteratorDebugParams_create();
    }

    IntervalIteratorDebugIspc::~IntervalIteratorDebugIspc()
    {
      scheduler.stop(*this);  // Causes interval context to be cleared.

      ispc::IntervalIteratorDebugParams_destroy(ispcParams);
      ispcParams = nullptr;
    }

    void IntervalIteratorDebugIspc::beforeStart()
    {
      IspcRenderer::beforeStart();
      shared->beforeStart();
    }

    void IntervalIteratorDebugIspc::afterStop()
    {
      IspcRenderer::afterStop();
      shared->afterStop();
    }

    void IntervalIteratorDebugIspc::beforeFrame(bool &needToClear)
    {
      IspcRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);

      if (needToClear) {
        ispc::IntervalIteratorDebugParams_set(
            ispcParams,
            shared->params->intervalColorScale,
            shared->params->intervalOpacity,
            shared->params->firstIntervalOnly,
            shared->params->showIntervalBorders);
        needToClear = true;
      }
    }

    void IntervalIteratorDebugIspc::renderPixelBlock(const vec2i &resolution,
                                                     uint32_t offset,
                                                     vec4f *rgbas,
                                                     float *weights) const
    {
      ispc::IntervalIteratorDebug_renderPixel(
          ispcParams,
          ispcScene,
          shared->intervalContext.get(),
          reinterpret_cast<const ispc::vec2i &>(resolution),
          offset,
          reinterpret_cast<ispc::vec4f *>(rgbas),
          weights);
    }

  }  // namespace examples
}  // namespace openvkl
