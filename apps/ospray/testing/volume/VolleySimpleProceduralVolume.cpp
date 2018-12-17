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

#include <ospcommon/box.h>
#include <ospray_testing/volume/Volume.h>
#include <vector>

using namespace ospcommon;

namespace ospray {
  namespace testing {

    struct VolleySimpleProceduralVolume : public Volume
    {
      VolleySimpleProceduralVolume()           = default;
      ~VolleySimpleProceduralVolume() override = default;

      OSPTestingVolume createVolume() const override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    OSPTestingVolume VolleySimpleProceduralVolume::createVolume() const
    {
      OSPVolume volume = ospNewVolume("volley::simple_procedural_volume");

      range1f voxelRange{-1.f, 1.f};

      const auto range  = voxelRange.toVec2f();
      const auto bounds = box3f(vec3f(-1.f), vec3f(1.f));

      OSPTestingVolume retval;
      retval.volume     = volume;
      retval.voxelRange = reinterpret_cast<const osp::vec2f &>(range);
      retval.bounds     = reinterpret_cast<const osp::box3f &>(bounds);

      return retval;
    }

    OSP_REGISTER_TESTING_VOLUME(VolleySimpleProceduralVolume,
                                volley_simple_procedural_volume);

  }  // namespace testing
}  // namespace ospray
