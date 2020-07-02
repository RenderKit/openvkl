// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(ISPC)

#include "openvkl/openvkl.isph"

#endif  // defined(ISPC)

#if defined(__cplusplus)

#include "TransferFunction.h"
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
      VKLValueSelector valueSelector;

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
            valueSelector(nullptr),
            tfNumColorsAndOpacities(0),
            tfColorsAndOpacities(nullptr)
      {
      }

      ~Scene()
      {
        if (valueSelector) {
          vklRelease(valueSelector);
        }
        if (sampler) {
          vklRelease(sampler);
        }
      }

      void updateValueSelector(const TransferFunction &transferFunction,
                               const std::vector<float> &isoValues)
      {
        if (valueSelector) {
          vklRelease(valueSelector);
          valueSelector = nullptr;
        }

        if (!volume)
          return;

        valueSelector = vklNewValueSelector(volume);

        // set value selector value ranges based on transfer function positive
        // opacity intervals
        std::vector<range1f> valueRanges =
            transferFunction.getPositiveOpacityValueRanges();

        vklValueSelectorSetRanges(valueSelector,
                                  valueRanges.size(),
                                  (const vkl_range1f *)valueRanges.data());

        // if we have isovalues, set these values on the value selector
        if (!isoValues.empty()) {
          vklValueSelectorSetValues(
              valueSelector, isoValues.size(), isoValues.data());
        }

        vklCommit(valueSelector);
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
      }
#endif  // defined(__cplusplus)
    };

#if defined(__cplusplus)

  }  // namespace examples
}  // namespace openvkl

#endif  // defined(__cplusplus)

