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

#include "ospcommon/memory/RefCount.h"
#include "ospcommon/utility/ParameterizedObject.h"
#include "VKLCommon.h"
#include "objectFactory.h"

namespace openvkl {

  struct OPENVKL_CORE_INTERFACE ManagedObject
      : public ospcommon::memory::RefCount,
        public ospcommon::utility::ParameterizedObject
  {
    using VKL_PTR = ManagedObject *;

    ManagedObject() = default;

    virtual ~ManagedObject() override;

    // commit the object's outstanding changes (such as changed parameters)
    virtual void commit() {}

    // common function to help printf-debugging; every derived class should
    // overrride this!
    virtual std::string toString() const;

    // subtype of this ManagedObject
    VKLDataType managedObjectType{VKL_UNKNOWN};
  };

  template <typename OPENVKL_CLASS, VKLDataType VKL_TYPE>
  inline OPENVKL_CLASS *createInstanceHelper(const std::string &type)
  {
    static_assert(std::is_base_of<ManagedObject, OPENVKL_CLASS>::value,
                  "createInstanceHelper<>() is only for VKL classes, not"
                  " generic types!");

    auto *object = objectFactory<OPENVKL_CLASS, VKL_TYPE>(type);

    // denote the subclass type in the ManagedObject base class.
    if (object) {
      object->managedObjectType = VKL_TYPE;
    }

    return object;
  }

  template <typename OPENVKL_CLASS, typename OPENVKL_HANDLE>
  OPENVKL_CLASS &referenceFromHandle(OPENVKL_HANDLE handle)
  {
    return *((OPENVKL_CLASS *)handle);
  }
}  // namespace openvkl

// Specializations for ISPCDriver /////////////////////////////////////////////

namespace ospcommon {
  namespace utility {

    template <>
    inline void ParameterizedObject::Param::set(
        const openvkl::ManagedObject::VKL_PTR &object)
    {
      using VKL_PTR = openvkl::ManagedObject::VKL_PTR;

      if (object)
        object->refInc();

      if (data.is<VKL_PTR>()) {
        auto *existingObj = data.get<VKL_PTR>();
        if (existingObj != nullptr)
          existingObj->refDec();
      }

      data = object;
    }

  }  // namespace utility
}  // namespace ospcommon
