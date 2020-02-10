// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#if defined(ISPC)

#include "openvkl/openvkl.isph"

#endif // defined(ISPC)

#if defined(__cplusplus)

#include "openvkl/openvkl.h"
#include "TransferFunction.h"

namespace openvkl {
  namespace examples {

#endif // defined(__cplusplus)

/*
 * This object stores all scene data. Renderers
 * read this to produce pixels.
 */
struct Scene
{
  /*
   * Our examples display a single volume.
   */
  VKLVolume        volume;
  VKLValueSelector valueSelector;

  /*
   * Shading is done through a transfer function.
   */
  box1f        tfValueRange;
  unsigned int tfNumColorsAndOpacities;
  const vec4f* tfColorsAndOpacities;

#if defined(__cplusplus)
  Scene()
    : volume(nullptr),
      valueSelector(nullptr),
      tfNumColorsAndOpacities(0),
      tfColorsAndOpacities(nullptr)
  {
  }

  void updateValueSelector(const TransferFunction& transferFunction,
                           const std::vector<float>& isoValues)
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
#endif // defined(__cplusplus)
};

#if defined(__cplusplus)

  } // namespace examples
} // namespace openvkl

#endif // defined(__cplusplus)

