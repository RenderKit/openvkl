// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../api/Device.h"
#include "VKLCommon.h"
#include "objectFactory.h"
#include "rkcommon/memory/IntrusivePtr.h"
#include "rkcommon/memory/RefCount.h"
#include "rkcommon/utility/ParameterizedObject.h"

using namespace rkcommon::memory;

namespace openvkl {

  struct Data;

  template <typename T>
  struct DataT;

  struct OPENVKL_CORE_INTERFACE ManagedObject
      : public rkcommon::memory::RefCountedObject,
        public rkcommon::utility::ParameterizedObject
  {
    using VKL_PTR = ManagedObject *;

    ManagedObject() = default;

    virtual ~ManagedObject() override;

    // uses the provided default value if the parameter is not set
    template <typename T>
    T getParam(const char *name, T valIfNotFound);

    // throws an error if the requested parameter is not set
    template <typename T>
    T getParam(const char *name);

    // returns if a data array of the given element type is present; can be used
    // to avoid warning messages in getParamDataT() if parameter fallbacks are
    // allowed
    template <typename T>
    bool hasParamDataT(const char *name);

    // gets a data array of elements of the given type; uses the provided
    // default value if the parameter is not set
    template <typename T>
    const Ref<const DataT<T>> getParamDataT(const char *name,
                                            DataT<T> *valIfNotFound);

    // gets a data array of elements of the given type; throws an error if the
    // requested parameter is not set
    template <typename T>
    const Ref<const DataT<T>> getParamDataT(const char *name);

    // gets a data array of the given size and type.
    // uses the provided default value if the parameter is not set.
    // expands the data into an array if only a single value is given.
    // throws an error if the array has more than one element but a different
    // size than expectedSize.
    template <typename T>
    const Ref<const DataT<T>> getParamDataT(const char *name,
                                            size_t expectedSize,
                                            T valIfNotFound);


    // throws an error if the named Data parameter is present and not compact
    void requireParamDataIsCompact(const char *name);

    // commit the object's outstanding changes (such as changed parameters)
    virtual void commit() {}

    // common function to help printf-debugging; every derived class should
    // overrride this!
    virtual std::string toString() const;

    // subtype of this ManagedObject
    VKLDataType managedObjectType{VKL_UNKNOWN};

    // device this ManagedObject belongs to
    rkcommon::memory::IntrusivePtr<Device> device;
  };

  template <typename OPENVKL_CLASS, VKLDataType VKL_TYPE>
  inline OPENVKL_CLASS *createInstanceHelper(Device *device,
                                             const std::string &type)
  {
    static_assert(std::is_base_of<ManagedObject, OPENVKL_CLASS>::value,
                  "createInstanceHelper<>() is only for VKL classes, not"
                  " generic types!");

    auto *object = objectFactory<OPENVKL_CLASS, VKL_TYPE>(device, type);

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

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename T>
  inline T ManagedObject::getParam(const char *name, T valIfNotFound)
  {
    return ParameterizedObject::getParam<T>(name, valIfNotFound);
  }

  template <typename T>
  inline T ManagedObject::getParam(const char *name)
  {
    if (!hasParam(name)) {
      throw std::runtime_error("missing required parameter '" +
                               std::string(name) + "'");
    }

    Param *param = findParam(name);

    if (!param->data.is<T>()) {
      throw std::runtime_error("found parameter '" + std::string(name) +
                               "', but it is not the expected type");
    }

    // we're guaranteed that the parameter exists and is the correct type; use
    // the method on ParameterizedObject since it does other things like set the
    // `query` flag.
    return getParam<T>(name, T());
  }

  // Specialization for Data objects //////////////////////////////////////////

  template <>
  inline Data *ManagedObject::getParam<Data *>(const char *name,
                                               Data *valIfNotFound)
  {
    auto *obj = ParameterizedObject::getParam<ManagedObject *>(
        name, (ManagedObject *)valIfNotFound);
    if (obj && obj->managedObjectType == VKL_DATA)
      return (Data *)obj;
    else
      return valIfNotFound;
  }

  template <>
  inline Data *ManagedObject::getParam<Data *>(const char *name)
  {
    if (!hasParam(name)) {
      throw std::runtime_error("missing required parameter '" +
                               std::string(name) + "'");
    }

    Param *param = findParam(name);

    if (!(param->data.is<ManagedObject *>() &&
          param->data.get<ManagedObject *>()->managedObjectType == VKL_DATA)) {
      throw std::runtime_error("found parameter '" + std::string(name) +
                               "', but it is not the expected type");
    }

    // we're guaranteed that the parameter exists and is the correct type; use
    // the method on ParameterizedObject since it does other things like set the
    // `query` flag.
    return getParam<Data *>(name, nullptr);
  }

}  // namespace openvkl

// Specializations for CPUDevice //////////////////////////////////////////////

namespace rkcommon {
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
}  // namespace rkcommon
