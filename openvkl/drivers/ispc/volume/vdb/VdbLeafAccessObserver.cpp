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

#include "VdbLeafAccessObserver.h"

namespace openvkl {
  namespace ispc_driver {

    VdbLeafAccessObserver::VdbLeafAccessObserver(ManagedObject &target,
                                                 size_t size,
                                                 const vkl_uint32 *accessBuffer)
        : target(&target), size(size), accessBuffer(accessBuffer)
    {
      this->target->refInc();
    }

    VdbLeafAccessObserver::~VdbLeafAccessObserver()
    {
      target->refDec();
    }

    const void *VdbLeafAccessObserver::map()
    {
      return accessBuffer;
    }

    void VdbLeafAccessObserver::unmap() {}

    size_t VdbLeafAccessObserver::getNumElements() const
    {
      return size;
    }

    VKLDataType VdbLeafAccessObserver::getElementType() const
    {
      return VKL_UINT;
    }

  }  // namespace ispc_driver
}  // namespace openvkl
