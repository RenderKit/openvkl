// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RayMarchIterator.h"
#include "RayMarchIterator_ispc.h"

namespace openvkl {
  namespace examples {

    // -------------------------------------------------------------------------

    template <class R>
    RayMarchIteratorShared<R>::RayMarchIteratorShared(
        const Scene *scene,
        const RendererParams *rendererParams,
        const Scheduler *scheduler)
        : scene{scene}, rendererParams{rendererParams}, scheduler{scheduler}
    {
    }

    template <class R>
    RayMarchIteratorShared<R>::~RayMarchIteratorShared()
    {
      assert(!intervalContext);
    }

    template <class R>
    void RayMarchIteratorShared<R>::updateIntervalContext()
    {
      vklSetInt(
          *intervalContext, "attributeIndex", rendererParams->attributeIndex);

      vklSetFloat(*intervalContext,
                   "intervalResolutionHint",
                   params->intervalResolutionHint);

      // set interval context value ranges based on transfer function positive
      // opacity intervals, if we have any
      VKLData valueRangesData;

      const std::vector<range1f> valueRanges =
          rendererParams->transferFunction.getPositiveOpacityValueRanges();

      if (!valueRanges.empty()) {
        valueRangesData = vklNewData(getOpenVKLDevice(),
                                     valueRanges.size(),
                                     VKL_BOX1F,
                                     valueRanges.data());
      }

      vklSetData(*intervalContext, "valueRanges", valueRangesData);

      if (valueRangesData) {
        vklRelease(valueRangesData);
      }

      vklCommit(*intervalContext);
    }

    template <class R>
    void RayMarchIteratorShared<R>::beforeStart()
    {
      if (intervalContext) {
        vklRelease(*intervalContext);
        intervalContext = nullptr;
      }

      VKLSampler sampler = scene->volume.getSampler();
      intervalContext    = rkcommon::make_unique<VKLIntervalIteratorContext>(
          vklNewIntervalIteratorContext(sampler));
      updateIntervalContext();
    }

    template <class R>
    void RayMarchIteratorShared<R>::afterStop()
    {
      if (intervalContext) {
        vklRelease(*intervalContext);
        intervalContext = nullptr;
      }
    }

    template <class R>
    void RayMarchIteratorShared<R>::beforeFrame(bool &needToClear)
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

    RayMarchIterator::RayMarchIterator(Scene &scene)
        : ScalarRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
    }

    RayMarchIterator::~RayMarchIterator()
    {
      scheduler.stop(*this);  // Causes interval context to be cleared.
    }

    void RayMarchIterator::beforeStart()
    {
      ScalarRenderer::beforeStart();
      shared->beforeStart();
    }

    void RayMarchIterator::afterStop()
    {
      ScalarRenderer::afterStop();
      shared->afterStop();
    }

    void RayMarchIterator::beforeFrame(bool &needToClear)
    {
      ScalarRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);
    }

    void RayMarchIterator::renderPixel(size_t seed,
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

      vec3f color(0.f);
      float alpha = 0.f;

      while (vklIterateInterval(iterator, &interval) && alpha < 0.99f) {
        const float nominalSamplingDt =
            interval.nominalDeltaT / shared->params->samplingRate;

        // initial sub interval, based on our renderer-defined sampling rate
        // and the volume's nominal dt
        box1f subInterval(interval.tRange.lower,
                          min(interval.tRange.lower + nominalSamplingDt,
                              interval.tRange.upper));

        // integrate as long as we have valid sub intervals and are not
        // fully opaque
        while (subInterval.upper - subInterval.lower > 0.f && alpha < 0.99f) {
          const float t  = 0.5f * (subInterval.lower + subInterval.upper);
          const float dt = subInterval.upper - subInterval.lower;

          // get volume sample
          vec3f c      = ray.org + t * ray.dir;
          float sample = vklComputeSample(scene.volume.getSamplerPtr(),
                                          (vkl_vec3f *)&c,
                                          rendererParams->attributeIndex,
                                          rendererParams->time);

          // map through transfer function
          vec4f sampleColorAndOpacity = sampleTransferFunction(sample);

          // accumulate contribution
          const float clampedOpacity = clamp(sampleColorAndOpacity.w * dt);

          sampleColorAndOpacity = sampleColorAndOpacity * clampedOpacity;

          color = color + (1.f - alpha) * vec3f(sampleColorAndOpacity);
          alpha = alpha + (1.f - alpha) * clampedOpacity;

          // compute next sub interval
          subInterval.lower = subInterval.upper;
          subInterval.upper =
              min(subInterval.lower + nominalSamplingDt, interval.tRange.upper);
        }
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    // -------------------------------------------------------------------------

    RayMarchIteratorIspc::RayMarchIteratorIspc(Scene &scene)
        : IspcRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
      ispcParams = ispc::RayMarchIteratorParams_create();
    }

    RayMarchIteratorIspc::~RayMarchIteratorIspc()
    {
      scheduler.stop(*this);  // Causes interval context to be cleared.

      ispc::RayMarchIteratorParams_destroy(ispcParams);
      ispcParams = nullptr;
    }

    void RayMarchIteratorIspc::beforeStart()
    {
      IspcRenderer::beforeStart();
      shared->beforeStart();
    }

    void RayMarchIteratorIspc::afterStop()
    {
      IspcRenderer::afterStop();
      shared->afterStop();
    }

    void RayMarchIteratorIspc::beforeFrame(bool &needToClear)
    {
      IspcRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);

      if (needToClear) {
        ispc::RayMarchIteratorParams_set(ispcParams,
                                                 shared->params->samplingRate);
      }
    }

    void RayMarchIteratorIspc::renderPixelBlock(const vec2i &resolution,
                                                        uint32_t offset,
                                                        vec4f *rgbas,
                                                        float *weights) const
    {
      ispc::RayMarchIterator_renderPixel(
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
