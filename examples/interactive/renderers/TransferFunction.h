// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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

#include <vector>
#include "ospcommon/math/box.h"

namespace openvkl {
  namespace examples {

    using namespace ospcommon;
    using namespace ospcommon::math;

    struct TransferFunction
    {
      TransferFunction() = default;

      TransferFunction(const range1f &valueRange,
                       const std::vector<vec4f> &colorsAndOpacities);

      range1f valueRange{-1.f, 1.f};
      std::vector<vec4f> colorsAndOpacities{
          {0.f, 0.f, 1.f, 0.f}, {0.f, 1.f, 0.f, 0.5f}, {1.f, 0.f, 0.f, 1.f}};

      std::vector<range1f> getPositiveOpacityValueRanges() const;

     private:
      // returns the minimal number of [min, max) index ranges for positive
      // opacity values
      std::vector<range1i> getPositiveOpacityIndexRanges() const;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TransferFunction::TransferFunction(
        const range1f &valueRange, const std::vector<vec4f> &colorsAndOpacities)
        : valueRange(valueRange), colorsAndOpacities(colorsAndOpacities)
    {
    }

    inline std::vector<range1f>
    TransferFunction::getPositiveOpacityValueRanges() const
    {
      std::vector<range1f> valueRanges;

      // determine index ranges for positive opacities
      std::vector<range1i> indexRanges = getPositiveOpacityIndexRanges();

      // convert index ranges to value ranges
      // note that a positive opacity value has a span of +/-1 due to the linear
      // interpolation, and returned index ranges are [min, max) intervals
      for (int i = 0; i < indexRanges.size(); i++) {
        int minValueIndex = indexRanges[i].lower - 1;
        int maxValueIndex = indexRanges[i].upper;

        float minValue;

        if (minValueIndex < 0) {
          minValue = neg_inf;
        } else {
          minValue =
              valueRange.lower + (float)minValueIndex /
                                     ((float)colorsAndOpacities.size() - 1.f) *
                                     (valueRange.upper - valueRange.lower);
        }

        float maxValue;

        if (maxValueIndex >= colorsAndOpacities.size()) {
          maxValue = inf;
        } else {
          maxValue =
              valueRange.lower + (float)maxValueIndex /
                                     ((float)colorsAndOpacities.size() - 1.f) *
                                     (valueRange.upper - valueRange.lower);
        }

        valueRanges.push_back(range1f(minValue, maxValue));
      }

      return valueRanges;
    }

    inline std::vector<range1i>
    TransferFunction::getPositiveOpacityIndexRanges() const
    {
      std::vector<range1i> intervals;

      range1i interval;
      bool rangeActive = false;

      for (int i = 0; i < colorsAndOpacities.size(); i++) {
        if (colorsAndOpacities[i].w > 0.f && !rangeActive) {
          rangeActive    = true;
          interval.lower = i;
        } else if (colorsAndOpacities[i].w <= 0.f && rangeActive) {
          rangeActive    = false;
          interval.upper = i;
          intervals.push_back(interval);
        }
      }

      // special case for final value
      if (colorsAndOpacities.back().w > 0.f) {
        if (rangeActive) {
          rangeActive    = false;
          interval.upper = colorsAndOpacities.size();
          intervals.push_back(interval);
        } else {
          throw std::runtime_error("getPositiveOpacityIndexRanges() error");
        }
      }

      return intervals;
    }

  }  // namespace examples
}  // namespace openvkl
