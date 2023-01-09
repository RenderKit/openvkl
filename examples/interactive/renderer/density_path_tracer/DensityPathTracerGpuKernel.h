// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "openvkl_testing.h"

#include "../Random.h"
#include "../Ray.h"
#include "../RendererParams.h"
#include "../RendererParamsShared.h"
#include "DensityPathTracerParams.h"

namespace openvkl {
  namespace examples {

    class DensityPathTracerGpuKernel
    {
     private:
      VKLSampler sampler;
      box3f volumeBounds;
      DensityPathTracerParams params;
      RendererParamsShared rendererParams;
      unsigned int frameId;

      vec3f integrate(RNG &rng,
                      const Ray *inputRay,
                      int &maxScatterIndex,
                      bool &primaryRayIntersect) const;

      bool sampleWoodcock(RandomTEA &rng,
                          const Ray &ray,
                          const range1f &hits,
                          float &t,
                          float &sample,
                          float &transmittance) const;

     public:
      void setObjectAttributes(const VKLSampler sampler,
                               const box3f volumeBounds,
                               const DensityPathTracerParams &params,
                               const RendererParams &rendererParams,
                               const unsigned int frameId)
      {
        this->sampler      = sampler;
        this->volumeBounds = volumeBounds;
        this->params       = params;

        this->rendererParams.attributeIndex = rendererParams.attributeIndex;
        this->rendererParams.time           = rendererParams.time;
        this->rendererParams.fixedFramebufferSize =
            rendererParams.fixedFramebufferSize;
        this->rendererParams.restrictPixelRange =
            rendererParams.restrictPixelRange;
        this->rendererParams.pixelRange = rendererParams.pixelRange;

        TransferFunctionShared *tf = &(this->rendererParams.transferFunction);
        tf->numValues =
            rendererParams.transferFunction.colorsAndOpacities.size();
        const size_t maxTfNumValues =
            sizeof(tf->colorsAndOpacities) / sizeof(tf->colorsAndOpacities[0]);
        if (tf->numValues > maxTfNumValues) {
          throw std::runtime_error("transferFunction.numValues overflow max: " +
                                   std::to_string(maxTfNumValues));
        }
        std::memcpy(tf->colorsAndOpacities,
                    rendererParams.transferFunction.colorsAndOpacities.data(),
                    sizeof(vec4f) * tf->numValues);
        tf->valueRange = rendererParams.transferFunction.valueRange;

        this->frameId = frameId;
      }
      inline bool pixelOnScene(const unsigned int idx,
                               const unsigned int width,
                               const unsigned int height,
                               const unsigned int x,
                               const unsigned int y);

      SYCL_EXTERNAL void renderPixel(const unsigned int seed,
                                     const Ray *ray,
                                     vec4f &rgba,
                                     float &weight) const;
    };

    // Inlined definitions ////////////////////////////////////////////////////
    inline range1f intersectBox(const vec3f &org,
                                const vec3f &dir,
                                const box3f &box)
    {
      const vec3f dirRecip = vec3f(sycl::native::recip(dir.x),
                                   sycl::native::recip(dir.y),
                                   sycl::native::recip(dir.z));

      const auto mins = (box.lower - org) * dirRecip;
      const auto maxs = (box.upper - org) * dirRecip;
      return range1f(
          reduce_max(vec_t<float, 4>(min(mins, maxs), 0)),
          reduce_min(vec_t<float, 4>(max(mins, maxs),
                                     std::numeric_limits<float>::infinity())));
    }

    inline vec4f sampleTransferFunction(const TransferFunctionShared &tf,
                                        float value)
    {
      vec4f colorAndOpacity{0.f};

      if (sycl::isnan(value) || tf.numValues == 0) {
        return colorAndOpacity;
      }

      if (value <= tf.valueRange.lower) {
        return tf.colorsAndOpacities[0];
      }

      if (value >= tf.valueRange.upper) {
        return tf.colorsAndOpacities[tf.numValues - 1];
      }

      // map the value into the range [0, size - 1]
      value = (value - tf.valueRange.lower) /
              (tf.valueRange.upper - tf.valueRange.lower) * (tf.numValues - 1);

      // index and fractional offset
      const int index       = floor(value);
      const float remainder = value - index;
      // the final interpolated value
      return ((1.f - remainder) * tf.colorsAndOpacities[index] +
              remainder *
                  tf.colorsAndOpacities[min(index + 1, int(tf.numValues - 1))]);
    }

    inline bool DensityPathTracerGpuKernel::pixelOnScene(
        const unsigned int idx,
        const unsigned int width,
        const unsigned int height,
        const unsigned int x,
        const unsigned int y)
    {
      if (rendererParams.fixedFramebufferSize &&
          rendererParams.restrictPixelRange) {
        // The output is mirrored!
        if ((height - y - 1) < rendererParams.pixelRange.lower.y ||
            (height - y - 1) >= rendererParams.pixelRange.upper.y ||
            (width - x - 1) < rendererParams.pixelRange.lower.x ||
            (width - x - 1) >= rendererParams.pixelRange.upper.x) {
          return false;
        }
      }
      return true;
    }
  }  // namespace examples
}  // namespace openvkl