// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
#include "Traits.h"
#include "openvkl/openvkl.h"

namespace openvkl {

  struct OPENVKL_CORE_INTERFACE Data : public ManagedObject
  {
    Data(size_t numItems,
         VKLDataType dataType,
         const void *source,
         VKLDataCreationFlags dataCreationFlags);

    virtual ~Data() override;

    virtual std::string toString() const override;

    size_t size() const;

    template <typename T>
    const T *begin() const;

    template <typename T>
    const T *end() const;

    template <typename T>
    const T &at(size_t i) const;

    template <typename T>
    typename std::enable_if<std::is_pointer<T>::value, bool>::type is() const;

    template <typename T>
    typename std::enable_if<!std::is_pointer<T>::value, bool>::type is() const;

    size_t numItems;
    size_t numBytes;
    VKLDataType dataType;
    const void *data;
    VKLDataCreationFlags dataCreationFlags;
  };

  template <typename T>
  inline const T *Data::begin() const
  {
    return static_cast<const T *>(data);
  }

  template <typename T>
  inline const T *Data::end() const
  {
    return begin<const T>() + numItems;
  }

  template <typename T>
  inline typename std::enable_if<std::is_pointer<T>::value, bool>::type
  Data::is() const
  {
    auto toType = VKLTypeFor<T>::value;
    return (dataType == toType ||
            (toType == VKL_OBJECT && isManagedObject(dataType)));
  }

  template <typename T>
  inline typename std::enable_if<!std::is_pointer<T>::value, bool>::type
  Data::is() const
  {
    return dataType == VKLTypeFor<T>::value;
  }

  template <typename T>
  inline const Ref<const Data> ManagedObject::getParamDataT(const char *name,
                                                            Data *valIfNotFound)
  {
    Data *data = getParam<Data *>(name, nullptr);

    if (data && data->is<T>()) {
      return data;
    }

    if (data) {
      postLogMessage(VKL_LOG_WARNING)
          << toString() << " ignoring '" << name
          << "' array with wrong element type (should be "
          << stringFor(VKLTypeFor<T>::value) << ")";
    }

    return valIfNotFound;
  }

  template <typename T>
  inline const Ref<const Data> ManagedObject::getParamDataT(const char *name)
  {
    Data *data = getParam<Data *>(name);

    if (data && data->is<T>()) {
      return data;
    }

    throw std::runtime_error(toString() + " must have '" + name +
                             "' array with element type " +
                             stringFor(VKLTypeFor<T>::value));
  }

}  // namespace openvkl
