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

#pragma once

#include "common/ManagedObject.h"
#include "common/objectFactory.h"
#include "volley/volley.h"

namespace volley {

  namespace scalar_driver {

    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type)
      {
        return createInstanceHelper<Volume, VLY_VOLUME>(type);
      }

      virtual void commit() override
      {
        ManagedObject::commit();
      }

      virtual float sample(const vly_vec3f *objectCoordinates) const = 0;

      virtual vly_vec3f gradient(const vly_vec3f *objectCoordinates) const = 0;

      virtual vly_box3f getBoundingBox() const = 0;
    };

#define VLY_REGISTER_VOLUME(InternalClass, external_name) \
  VLY_REGISTER_OBJECT(Volume, volume, InternalClass, external_name)

  }  // namespace scalar_driver
}  // namespace volley
