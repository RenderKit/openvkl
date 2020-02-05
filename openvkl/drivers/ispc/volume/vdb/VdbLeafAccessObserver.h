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

#include "../common/Observer.h"
#include "openvkl/ispc_cpp_interop.h"

namespace openvkl {
  namespace ispc_driver {

    /*
     * The leaf access observer simply wraps the buffer allocated by VdbVolume.
     */
    struct VdbLeafAccessObserver : public Observer
    {
      VdbLeafAccessObserver(ManagedObject &target,
                            size_t size,
                            const vkl_uint32 *accessBuffer);

      VdbLeafAccessObserver(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver &operator=(VdbLeafAccessObserver &&) = delete;
      VdbLeafAccessObserver(const VdbLeafAccessObserver &)       = delete;
      VdbLeafAccessObserver &operator=(const VdbLeafAccessObserver &) = delete;

      ~VdbLeafAccessObserver();

      const void *map() override;
      void unmap() override;
      VKLDataType getElementType() const override;
      size_t getNumElements() const override;

     private:
      ManagedObject *target{nullptr};
      size_t size{0};
      const vkl_uint32 *accessBuffer{nullptr};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
