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

#include "Tile.h"

namespace ospray {
  namespace scalar_volley_device {

    Tile::Tile(const vec2i &origin, const vec2i &size)
        : origin(origin), size(size)
    {
      colorBuffer.resize(size.x * size.y);
    }

    void Tile::clear(const vec4f &backgroundColor)
    {
      std::fill(colorBuffer.begin(), colorBuffer.end(), backgroundColor);
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
