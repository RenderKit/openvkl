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

#pragma once

#include <ospray/ospcommon/containers/AlignedVector.h>
#include <ospray/ospcommon/vec.h>

namespace ospray {
  namespace scalar_volley_device {

    using namespace ospcommon;

    struct Tile
    {
      Tile(const vec2i &origin, const vec2i &size);
      ~Tile() = default;

      void clear(const vec4f &backgroundColor);

      size_t indexOf(const vec2i &coords) const;

      // color buffer for this tile
      containers::AlignedVector<vec4f> colorBuffer;

      // origin within the frame buffer
      const vec2i origin;

      // size of this tile
      const vec2i size;
    };

    // Inlined members ////////////////////////////////////////////////////////

    inline size_t Tile::indexOf(const vec2i &coords) const
    {
      return coords.y * size.x + coords.x;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
