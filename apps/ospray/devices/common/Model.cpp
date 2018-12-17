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

#include "Model.h"

namespace ospray {
  namespace scalar_volley_device {

    void Model::addVolume(Volume &volume)
    {
      auto ptr = std::static_pointer_cast<Volume>(volume.shared_from_this());
      volumes.push_back(ptr);
    }

    void Model::removeVolume(Volume &volume)
    {
      auto *addressOfObjectToRemove = &volume;

      auto addr_matches = [&](const std::shared_ptr<Volume> &m) {
        return m.get() == addressOfObjectToRemove;
      };

      auto &v = volumes;
      v.erase(std::remove_if(v.begin(), v.end(), addr_matches), v.end());
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
