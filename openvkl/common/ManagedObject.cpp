// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "ManagedObject.h"

#include <algorithm>

namespace openvkl {

  ManagedObject::~ManagedObject()
  {
    // make sure all ManagedObject parameter refs are decremented
    std::for_each(params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
      auto &param = *p;
      if (param.data.is<VKL_PTR>()) {
        auto *obj = param.data.get<VKL_PTR>();
        if (obj != nullptr)
          obj->refDec();
      }
    });
  }

  std::string ManagedObject::toString() const
  {
    return "openvkl::ManagedObject";
  }

}  // namespace openvkl
