// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#include "ManagedObject.h"
#include "openvkl/openvkl.h"

namespace openvkl {

  struct OPENVKL_CORE_INTERFACE Observer : public ManagedObject
  {
    virtual ~Observer() override;
    virtual std::string toString() const override;

    virtual const void *map() = 0;
    virtual void unmap() = 0;
    virtual VKLDataType getElementType() const = 0;
    virtual size_t getNumElements() const = 0;
  };

}  // namespace openvkl

