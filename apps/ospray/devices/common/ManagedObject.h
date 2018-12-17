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

// ospcommon
#include "ospray/ospcommon/utility/ParameterizedObject.h"
// std
#include <memory>

namespace ospray {
  namespace scalar_volley_device {

    using namespace ospcommon;

    struct ManagedObject : public utility::ParameterizedObject,
                           public std::enable_shared_from_this<ManagedObject>
    {
      ManagedObject() = default;

      virtual ~ManagedObject() override = default;

      /*! \brief commit the object's outstanding changes (such as changed
       *         parameters etc) */
      virtual void commit();

      /*! get a (shared_ptr to) a object-type parameter. Note this
        assumes that the parameter is read in the same type as it is
        written; ie., trying to read a "Data" type from a parameter
        that was written as a float will throw an exception */
      template <typename T>
      std::shared_ptr<T> getParamObject(
          const std::string &name,
          std::shared_ptr<T> valIfNotFound = std::shared_ptr<T>());
    };

    using ManagedPtr = std::shared_ptr<ManagedObject>;

    // Inlined definitions ////////////////////////////////////////////////////

    inline void ManagedObject::commit() {}

    /*! get parameter of a given _derived_ manged object type - e.g, a
        model, a geometry, a volume, etc; since the base object only
        stores shared-ptrs of the base class we have to make sure that
        we always query for the base class, then dynamic-cast to the
        type the app requested */
    template <typename T>
    inline std::shared_ptr<T> ManagedObject::getParamObject(
        const std::string &name, std::shared_ptr<T> valIfNotFound)
    {
      auto obj = ParameterizedObject::getParam<ManagedPtr>(name, nullptr);
      if (obj)
        return std::dynamic_pointer_cast<T>(obj);
      else
        return valIfNotFound;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
