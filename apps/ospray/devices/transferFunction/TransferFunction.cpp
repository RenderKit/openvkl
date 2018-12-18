// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "TransferFunction.h"
#include "../common/Data.h"

namespace ospray {
  namespace scalar_volley_device {

    void TransferFunction::commit()
    {
      // the user can supply a differing number of color and opacity control
      // points!
      auto &colorData   = *getParamObject<Data>("colors");
      auto &opacityData = *getParamObject<Data>("opacities");

      colors.resize(colorData.size());
      opacities.resize(opacityData.size());

      for (int i = 0; i < colorData.size(); ++i) {
        auto color = colorData.valueAt<vec3f>(i);
        colors[i]  = vec3f(color.x, color.y, color.z);
      }

      for (int i = 0; i < opacityData.size(); ++i) {
        auto alpha   = opacityData.valueAt<float>(i);
        opacities[i] = alpha;
      }

      auto range = getParam<vec2f>("valueRange", valueRange.toVec2f());
      valueRange = range1f(range.x, range.y);
    }

    vec4f TransferFunction::getColorAndOpacity(float value) const
    {
      if (isnan(value))
        return vec4f{0.0f};

      if (colors.empty())
        return vec4f{1.0f};

      const int numColors    = static_cast<int>(colors.size());
      const int numOpacities = static_cast<int>(opacities.size());

      vec3f color;
      float opacity;

      if (value <= valueRange.lower) {
        color   = colors.front();
        opacity = opacities.front();
      } else if (value >= valueRange.upper) {
        color   = colors.back();
        opacity = opacities.back();
      } else {
        const float colorIndexF =
            (value - valueRange.lower) / valueRange.size() * (numColors - 1);
        const float opacityIndexF =
            (value - valueRange.lower) / valueRange.size() * (numOpacities - 1);

        const int colorIndex       = ospcommon::floor(colorIndexF);
        const float colorRemainder = colorIndexF - colorIndex;

        const int opacityIndex       = ospcommon::floor(opacityIndexF);
        const float opacityRemainder = opacityIndexF - opacityIndex;

        color = lerp(
            1.f - colorRemainder, colors[colorIndex], colors[colorIndex + 1]);
        opacity = lerp(1.f - opacityRemainder,
                       opacities[opacityIndex],
                       opacities[opacityIndex + 1]);
      }

      return vec4f(color.x, color.y, color.z, opacity);
    }

  }  // namespace scalar_volley_device
}  // namespace ospray