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

#include "FrameBuffer.h"
#include <ospray/ospcommon/common.h>

namespace ospray {
  namespace scalar_volley_device {

    // Helper functions ///////////////////////////////////////////////////////

    // helper function to convert float-color into rgba-uint format
    static inline uint32 cvt_uint32(const float &f)
    {
      return (int)(255.f * clamp(f, 0.f, 1.f));
    }

    // helper function to convert float-color into rgba-uint format
    static inline uint32 cvt_uint32(const vec4f &v)
    {
      return (cvt_uint32(v.x) << 0) | (cvt_uint32(v.y) << 8) |
             (cvt_uint32(v.z) << 16) | (cvt_uint32(v.w) << 24);
    }

    // FrameBuffer definitions ////////////////////////////////////////////////

    FrameBuffer::FrameBuffer(const vec2i &frameBufferSize,
                             const vec2i &tileSize)
        : frameBufferSize(frameBufferSize), tileSize(tileSize)
    {
      const int numPixels = frameBufferSize.product();

      colorBuffer.resize(numPixels);
      mappedFrameBuffer.resize(numPixels);

      for (int y = 0; y < frameBufferSize.y; y += tileSize.y) {
        for (int x = 0; x < frameBufferSize.x; x += tileSize.x) {
          tiles.emplace_back(vec2i(x, y), tileSize);
        }
      }
    }

    void FrameBuffer::beginFrame()
    {
    }

    void FrameBuffer::clear(const vec4f &backgroundColor)
    {
      std::for_each(tiles.begin(), tiles.end(), [&](Tile &t) {
        t.clear(backgroundColor);
      });
    }

    void FrameBuffer::endFrame()
    {
      // update frame buffer with tile contents
      for (size_t t = 0; t < tiles.size(); t++) {
        Tile &tile = tiles[t];

        for (int y = 0; y < tile.size.y; y++) {
          for (int x = 0; x < tile.size.x; x++) {
            const vec2i idx(x, y);
            size_t tileBufferIndex  = tile.indexOf(idx);
            size_t colorBufferIndex = indexOf(tile.origin + idx);

            colorBuffer[colorBufferIndex] = tile.colorBuffer[tileBufferIndex];
          }
        }
      }
    }

    uint32 *FrameBuffer::mapColorBuffer()
    {
      std::transform(colorBuffer.begin(),
                     colorBuffer.end(),
                     mappedFrameBuffer.begin(),
                     [](const vec4f &in) { return cvt_uint32(in); });

      return mappedFrameBuffer.data();
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
