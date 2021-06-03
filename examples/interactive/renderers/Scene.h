// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(ISPC)

#include "openvkl/openvkl.isph"

#endif  // defined(ISPC)

#if defined(__cplusplus)

#include "TransferFunction.h"
#include "apps/AppInit.h"
#include "openvkl/openvkl.h"

namespace openvkl {
  namespace examples {

#endif  // defined(__cplusplus)

    /*
     * This object stores all scene data. Renderers
     * read this to produce pixels.
     */
    struct Scene
    {
      /*
       * Our examples display a single volume.
       */
      VKLVolume volume;
      VKLSampler sampler;
      VKLIntervalIteratorContext intervalContext;
      VKLHitIteratorContext hitContext;
      unsigned int attributeIndex;

      /*
       * Shading is done through a transfer function.
       */
      box1f tfValueRange;
      unsigned int tfNumColorsAndOpacities;
      const vec4f *tfColorsAndOpacities;

#if defined(__cplusplus)
      Scene()
          : volume(nullptr),
            sampler(nullptr),
            intervalContext(nullptr),
            hitContext(nullptr),
            attributeIndex(0),
            tfNumColorsAndOpacities(0),
            tfColorsAndOpacities(nullptr)
      {
      }

      ~Scene()
      {
        if (intervalContext) {
          vklRelease(intervalContext);
        }
        if (hitContext) {
          vklRelease(hitContext);
        }
        if (sampler) {
          vklRelease(sampler);
        }
      }

      // TODO: we'll split this into separate methods for interval and hit
      // context setup
      void updateValueSelector(const TransferFunction &transferFunction,
                               const std::vector<float> &isoValues)
      {
        // set interval context value ranges based on transfer function positive
        // opacity intervals, if we have any
        VKLData valueRangesData = nullptr;

        std::vector<range1f> valueRanges =
            transferFunction.getPositiveOpacityValueRanges();

        if (!valueRanges.empty()) {
          valueRangesData = vklNewData(getOpenVKLDevice(),
                                       valueRanges.size(),
                                       VKL_BOX1F,
                                       valueRanges.data());
        }

        // if we have isovalues, set these values on the value selector
        VKLData valuesData = nullptr;

        if (!isoValues.empty()) {
          valuesData = vklNewData(getOpenVKLDevice(),
                                  isoValues.size(),
                                  VKL_FLOAT,
                                  isoValues.data());
        }

        // context setup
        if (intervalContext) {
          vklRelease(intervalContext);
          intervalContext = nullptr;
        }

        intervalContext =
            vklNewIntervalIteratorContext(sampler, attributeIndex);

        if (valueRangesData) {
          vklSetData(intervalContext, "valueRanges", valueRangesData);
          vklRelease(valueRangesData);
        }

        vklCommit(intervalContext);

        if (hitContext) {
          vklRelease(hitContext);
          hitContext = nullptr;
        }

        hitContext = vklNewHitIteratorContext(sampler, attributeIndex);

        if (valuesData) {
          vklSetData(hitContext, "values", valuesData);
          vklRelease(valuesData);
        }

        vklCommit(hitContext);
      }

      void updateVolume(VKLVolume volume)
      {
        if (sampler) {
          vklRelease(sampler);
          sampler = nullptr;
        }
        this->volume = volume;

        if (!this->volume)
          return;

        sampler = vklNewSampler(volume);
        vklCommit(sampler);
      }
#endif  // defined(__cplusplus)
    };

#if defined(__cplusplus)

  }  // namespace examples
}  // namespace openvkl

#endif  // defined(__cplusplus)
