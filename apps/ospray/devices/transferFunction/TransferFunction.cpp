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
      auto &colorData   = *getParamObject<Data>("colors");
      auto &opacityData = *getParamObject<Data>("opacities");

      colors.resize(colorData.size());

      for (int i = 0; i < colorData.size(); ++i) {
        auto color = colorData.valueAt<vec3f>(i);
        auto alpha = opacityData.valueAt<float>(i);
        colors[i]  = vec4f(color.x, color.y, color.z, alpha);
      }

      auto range = getParam<vec2f>("valueRange", valueRange.toVec2f());
      valueRange = range1f(range.x, range.y);
    }

    vec4f TransferFunction::getColor(float value) const
    {
      if (isnan(value))
        return vec4f{0.0f};

      if (colors.empty())
        return vec4f{1.0f};

      const int numColors = static_cast<int>(colors.size());

      if (value <= valueRange.lower)
        return colors.front();

      if (value >= valueRange.upper)
        return colors.back();

      // map the value into the range [0, 1]
      value = (value - valueRange.lower) / valueRange.size() * numColors;

      // compute the color index and fractional offset
      const int index       = ospcommon::floor(value);
      const float remainder = value - index;

      // return the interpolated color
      return lerp(1.f - remainder, colors[index], colors[index + 1]);
    }

  }  // namespace scalar_volley_device
}  // namespace ospray