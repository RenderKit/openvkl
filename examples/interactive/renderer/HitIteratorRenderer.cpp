// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "HitIteratorRenderer.h"
#include "HitIteratorRenderer_ispc.h"

#include <openvkl/openvkl.h>
#include <rkcommon/common.h>

namespace openvkl {
  namespace examples {

    template <class R>
    HitIteratorRendererShared<R>::HitIteratorRendererShared(
        const Scene *scene,
        const RendererParams *rendererParams,
        const Scheduler *scheduler)
        : scene{scene}, rendererParams{rendererParams}, scheduler{scheduler}
    {
    }

    template <class R>
    HitIteratorRendererShared<R>::~HitIteratorRendererShared()
    {
      assert(!hitContext);
    }

    template <class R>
    void HitIteratorRendererShared<R>::updateHitContext()
    {
      vklSetInt(hitContext, "attributeIndex", rendererParams->attributeIndex);

      // if we have isovalues, set these values on the context
      VKLData valuesData = nullptr;

      if (!params->isoValues.empty()) {
        valuesData = vklNewData(getOpenVKLDevice(),
                                params->isoValues.size(),
                                VKL_FLOAT,
                                params->isoValues.data());
      }

      vklSetData(hitContext, "values", valuesData);

      if (valuesData) {
        vklRelease(valuesData);
      }

      vklCommit(hitContext);
    }

    template <class R>
    void HitIteratorRendererShared<R>::beforeStart()
    {
      assert(!hitContext);
      VKLSampler sampler = scene->volume.getSampler();
      hitContext         = vklNewHitIteratorContext(sampler);
      updateHitContext();
    }

    template <class R>
    void HitIteratorRendererShared<R>::afterStop()
    {
      if (hitContext) {
        vklRelease(hitContext);
        hitContext = nullptr;
      }
    }

    template <class R>
    void HitIteratorRendererShared<R>::beforeFrame(bool &needToClear)
    {
      bool paramsChanged = false;
      scheduler->locked(guiParams, [&]() {
        paramsChanged = params.updateIfChanged(guiParams);
      });

      needToClear |= paramsChanged;

      // Renderer params contain the attribute index. This is why
      // we update the hit context when the parent changed.
      if (needToClear) {
        updateHitContext();
      }
    }

    // -------------------------------------------------------------------------

    HitIteratorRenderer::HitIteratorRenderer(Scene &scene)
        : ScalarRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
    }

    HitIteratorRenderer::~HitIteratorRenderer()
    {
      scheduler.stop(*this);  // Causes hit context to be cleared.
    }

    void HitIteratorRenderer::beforeStart()
    {
      ScalarRenderer::beforeStart();
      shared->beforeStart();
    }

    void HitIteratorRenderer::afterStop()
    {
      ScalarRenderer::afterStop();
      shared->afterStop();
    }

    void HitIteratorRenderer::beforeFrame(bool &needToClear)
    {
      ScalarRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);
    }

    void HitIteratorRenderer::renderPixel(size_t seed,
                                          Ray &ray,
                                          vec4f &rgba,
                                          float &weight) const
    {
      const box3f volumeBounds = scene.volume.getBounds();
      ray.t = intersectRayBox(ray.org, ray.dir, volumeBounds);

      vkl_range1f tRange;
      tRange.lower = ray.t.lower;
      tRange.upper = ray.t.upper;

      void *hitIteratorBuffer =
          alloca(vklGetHitIteratorSize(shared->hitContext));
      void *shadowHitIteratorBuffer =
          alloca(vklGetHitIteratorSize(shared->hitContext));

      VKLHitIterator iterator = vklInitHitIterator(shared->hitContext,
                                                   (vkl_vec3f *)&ray.org,
                                                   (vkl_vec3f *)&ray.dir,
                                                   &tRange,
                                                   rendererParams->time,
                                                   hitIteratorBuffer);

      VKLHit hit;
      const float surfaceAlpha = 0.6f;
      const float emission[]   = {1.f, 0.8f};
      const vec3f lightDir[]   = {normalize(vec3f(1.f, 1.f, 1.f)),
                                normalize(vec3f(1.f, 1.f, -1.f))};

      vec3f color(0.f);
      float alpha = 0.f;

      VKLSampler sampler = scene.volume.getSampler();

      while (vklIterateHit(iterator, &hit) && alpha < 0.99f) {
        const vec3f c = ray.org + hit.t * ray.dir;
        const vkl_vec3f grad =
            vklComputeGradient(sampler,
                               (vkl_vec3f *)&c,
                               rendererParams->attributeIndex,
                               rendererParams->time);
        vec3f N = normalize(vec3f(grad.x, grad.y, grad.z));
        if (std::isnan(N.x) || std::isnan(N.y) || std::isnan(N.z))
          N = vec3f(0.f);
        // wi and wo both point away from the event.
        const vec3f wi = normalize(-1.f * ray.dir);
        const float ci = dot(N, wi);

        float illum = 0.f;
        if (length(N) > 0) {
          illum = 0.1f;  // Ambient term.
          // Passing through the surface? Include the surface we started on in
          // alpha.
          for (int i = 0; i < 2; ++i) {
            const vec3f wo = lightDir[i];
            const float co = dot(N, wo);

            // Only test for shadow if we don't have to go through this surface.
            if ((co > 0) == (ci > 0)) {
              VKLHit shadowHit;
              vkl_range1f tShadowRange;
              tShadowRange.lower = hit.epsilon;
              tShadowRange.upper = inf;
              VKLHitIterator shadowIterator =
                  vklInitHitIterator(shared->hitContext,
                                     (vkl_vec3f *)&c,
                                     (vkl_vec3f *)&wo,
                                     &tShadowRange,
                                     rendererParams->time,
                                     shadowHitIteratorBuffer);
              if (!vklIterateHit(shadowIterator, &shadowHit)) {
                illum += abs(co) * emission[i];  // Lambertian surface shading.
              }
            }
          }
        } else {
          // We have no normal, so do not attempt to shade.
          illum = 1.f;
        }
        // Lambertian surface shading.
        const vec4f surfaceColorAndOpacity = sampleTransferFunction(hit.sample);
        const vec3f albedo                 = vec3f(surfaceColorAndOpacity);
        color = color + (1 - alpha) * illum * albedo;
        alpha = alpha + (1.f - alpha) * surfaceAlpha;
      }

      rgba.x = color.x;
      rgba.y = color.y;
      rgba.z = color.z;
      rgba.w = 1.f;
      weight = 1.f;
    }

    // -------------------------------------------------------------------------

    HitIteratorRendererIspc::HitIteratorRendererIspc(Scene &scene)
        : IspcRenderer{scene},
          shared{rkcommon::make_unique<Shared>(
              &scene, &*rendererParams, &scheduler)}
    {
      ispcParams = ispc::HitIteratorRendererParams_create();
    }

    HitIteratorRendererIspc::~HitIteratorRendererIspc()
    {
      scheduler.stop(*this);  // Causes hit context to be cleared.

      ispc::HitIteratorRendererParams_destroy(ispcParams);
      ispcParams = nullptr;
    }

    void HitIteratorRendererIspc::beforeStart()
    {
      IspcRenderer::beforeStart();
      shared->beforeStart();
    }

    void HitIteratorRendererIspc::afterStop()
    {
      IspcRenderer::afterStop();
      shared->afterStop();
    }

    void HitIteratorRendererIspc::beforeFrame(bool &needToClear)
    {
      IspcRenderer::beforeFrame(needToClear);
      shared->beforeFrame(needToClear);

      if (needToClear) {
        ispc::HitIteratorRendererParams_set(
            ispcParams,
            static_cast<uint32_t>(shared->params->isoValues.size()),
            shared->params->isoValues.data());
      }
    }

    void HitIteratorRendererIspc::renderPixelBlock(const vec2i &resolution,
                                                   uint32_t offset,
                                                   vec4f *rgbas,
                                                   float *weights) const
    {
      ispc::HitIteratorRenderer_renderPixel(
          ispcParams,
          ispcScene,
          shared->hitContext,
          reinterpret_cast<const ispc::vec2i &>(resolution),
          offset,
          reinterpret_cast<ispc::vec4f *>(rgbas),
          weights);
    }

  }  // namespace examples
}  // namespace openvkl
