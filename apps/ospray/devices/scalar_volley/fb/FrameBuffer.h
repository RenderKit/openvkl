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

#include <ospray/ospcommon/vec.h>
#include <vector>

#include "../common/ManagedObject.h"
#include "Tile.h"

using uint32 = unsigned int;

namespace ospray {
  namespace scalar_volley_device {

    struct FrameBuffer : public ManagedObject
    {
      FrameBuffer(const vec2i &frameBufferSize, const vec2i &tileSize);

      ~FrameBuffer() = default;

      void beginFrame();
      void endFrame();

      void clear(const vec4f &backgroundColor);

      uint32 *mapColorBuffer();

      const vec2i &size() const;

      std::vector<Tile> &getTiles();

     private:

      size_t indexOf(const vec2i &coords) const;

      // Data //

      const vec2i frameBufferSize;
      const vec2i tileSize;

      // full frame buffer contents
      containers::AlignedVector<vec4f> colorBuffer;

      // individual tiles, used in performing actual rendering
      std::vector<Tile> tiles;

      // final frame buffer contents for the application
      containers::AlignedVector<uint32> mappedFrameBuffer;
    };

    // Inlined members ////////////////////////////////////////////////////////

    inline size_t FrameBuffer::indexOf(const vec2i &coords) const
    {
      return coords.y * frameBufferSize.x + coords.x;
    }

    inline const vec2i &FrameBuffer::size() const
    {
      return frameBufferSize;
    }

    inline std::vector<Tile> &FrameBuffer::getTiles()
    {
      return tiles;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
