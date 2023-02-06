// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "RendererParams.h"
#include "RendererParamsShared.h"

namespace openvkl {
  namespace examples {
    class RendererGpuKernel
    {
     public:
      inline bool bufferPreprocessing(const bool clearFramebuffer,
                                      const unsigned int idx,
                                      const unsigned int width,
                                      const unsigned int height,
                                      vec4f *rgbaBuffer,
                                      float *weightBuffer);

     protected:
      VKLSampler sampler;
      box3f volumeBounds;
      RendererParamsShared rendererParams;
      inline vec4f sampleTransferFunction(float value) const;
      inline void setObjectAttributes(const VKLSampler sampler,
                                      const box3f volumeBounds,
                                      const RendererParams &rendererParams);

     private:
      inline bool pixelOnScene(const unsigned int idx,
                               const unsigned int width,
                               const unsigned int height,
                               const unsigned int x,
                               const unsigned int y) const;
    };

    // Inlined definitions ////////////////////////////////////////////////////
    inline bool RendererGpuKernel::pixelOnScene(const unsigned int idx,
                                                const unsigned int width,
                                                const unsigned int height,
                                                const unsigned int x,
                                                const unsigned int y) const
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

    inline vec4f RendererGpuKernel::sampleTransferFunction(float value) const
    {
      const TransferFunctionShared &tf = rendererParams.transferFunction;
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

    inline bool RendererGpuKernel::bufferPreprocessing(
        const bool clearFramebuffer,
        const unsigned int idx,
        const unsigned int width,
        const unsigned int height,
        vec4f *rgbaBuffer,
        float *weightBuffer)
    {
      const int y = idx / width;
      const int x = idx % width;

      if (!pixelOnScene(idx, width, height, x, y)) {
        rgbaBuffer[idx]   = vec4f(.18f, .18f, .18f, 1.f);
        weightBuffer[idx] = 1.f;
        return true;
      }

      if (clearFramebuffer) {
        rgbaBuffer[idx]   = vec4f(0.f);
        weightBuffer[idx] = 0.f;
      }
      return false;
    }

    inline void RendererGpuKernel::setObjectAttributes(
        const VKLSampler sampler,
        const box3f volumeBounds,
        const RendererParams &rendererParams)
    {
      this->sampler      = sampler;
      this->volumeBounds = volumeBounds;

      this->rendererParams.attributeIndex = rendererParams.attributeIndex;
      this->rendererParams.time           = rendererParams.time;
      this->rendererParams.fixedFramebufferSize =
          rendererParams.fixedFramebufferSize;
      this->rendererParams.restrictPixelRange =
          rendererParams.restrictPixelRange;
      this->rendererParams.pixelRange = rendererParams.pixelRange;

      TransferFunctionShared *tf = &(this->rendererParams.transferFunction);
      tf->numValues = rendererParams.transferFunction.colorsAndOpacities.size();
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
    }

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
  }  // namespace examples
}  // namespace openvkl
