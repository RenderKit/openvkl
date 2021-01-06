// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
#include "Traits.h"
#include "openvkl/openvkl.h"

#ifndef __ISPC_STRUCT_Data1D__
#define __ISPC_STRUCT_Data1D__
namespace ispc {
  struct Data1D
  {
    const uint8_t *addr;
    uint64_t byteStride;
    uint64_t numItems;
    VKLDataType dataType;
    bool compact;
  };
}  // namespace ispc
#endif

namespace openvkl {

  template <typename T>
  struct DataT;

  struct OPENVKL_CORE_INTERFACE Data : public ManagedObject
  {
    Data(size_t numItems,
         VKLDataType dataType,
         const void *source,
         VKLDataCreationFlags dataCreationFlags,
         size_t byteStride);

    virtual ~Data() override;

    virtual std::string toString() const override;

    size_t size() const;

    bool compact() const;  // all strides are natural

    template <typename T>
    const DataT<T> &as() const;

    template <typename T>
    typename std::enable_if<std::is_pointer<T>::value, bool>::type is() const;

    template <typename T>
    typename std::enable_if<!std::is_pointer<T>::value, bool>::type is() const;

    size_t numItems;
    VKLDataType dataType;
    VKLDataCreationFlags dataCreationFlags;
    size_t byteStride;

    ispc::Data1D ispc;
    static ispc::Data1D emptyData1D;  // dummy, zero-initialized

   protected:
    char *addr;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename T>
  inline const DataT<T> &Data::as() const
  {
    if (is<T>()) {
      return (const DataT<T> &)*this;
    } else {
      std::stringstream ss;
      ss << "Incompatible type for DataT; requested type: "
         << stringFor(VKLTypeFor<T>::value)
         << ", actual: " << stringFor(dataType);
      throw std::runtime_error(ss.str());
    }
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

  // DataT ////////////////////////////////////////////////////////////////////

  template <typename T>
  class Iter1D : public std::iterator<std::forward_iterator_tag, T>
  {
    char *addr{nullptr};
    size_t byteStride{0};

   public:
    Iter1D(char *addr, size_t byteStride) : addr(addr), byteStride(byteStride)
    {
    }
    Iter1D &operator++()
    {
      addr += byteStride;
      return *this;
    }
    Iter1D operator++(int)
    {
      Iter1D retv(*this);
      ++(*this);
      return retv;
    }
    bool operator==(const Iter1D &other) const
    {
      return addr == other.addr;
    }
    bool operator!=(const Iter1D &other) const
    {
      return addr != other.addr;
    }
    const T &operator*() const
    {
      return *reinterpret_cast<T *>(addr);
    }
    const T *operator->() const
    {
      return reinterpret_cast<T *>(addr);
    }
  };

  template <typename T>
  struct DataT : public Data
  {
    using value_type = T;
    using interator  = Iter1D<T>;

    Iter1D<T> begin() const
    {
      return Iter1D<T>(addr, byteStride);
    }
    Iter1D<T> end() const
    {
      return Iter1D<T>(addr + byteStride * size(), byteStride);
    }

    T &operator[](size_t idx)
    {
      return *reinterpret_cast<T *>(addr + byteStride * idx);
    }
    const T &operator[](size_t idx) const
    {
      return const_cast<DataT<T> *>(this)->operator[](idx);
    }

    const T *data() const
    {
      return reinterpret_cast<T *>(addr);
    }
  };

  // ManagedObject specializations ////////////////////////////////////////////

  template <typename T>
  inline bool ManagedObject::hasParamDataT(const char *name)
  {
    Data *data = getParam<Data *>(name, nullptr);

    if (data && data->is<T>()) {
      return true;
    }

    return false;
  }

  template <typename T>
  inline const Ref<const DataT<T>> ManagedObject::getParamDataT(
      const char *name, DataT<T> *valIfNotFound)
  {
    Data *data = getParam<Data *>(name, nullptr);

    if (data && data->is<T>()) {
      return &(data->as<T>());
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
  inline const Ref<const DataT<T>> ManagedObject::getParamDataT(
      const char *name)
  {
    Data *data = getParam<Data *>(name);

    if (data && data->is<T>()) {
      return &(data->as<T>());
    }

    throw std::runtime_error(toString() + " must have '" + name +
                             "' array with element type " +
                             stringFor(VKLTypeFor<T>::value));
  }

  inline void ManagedObject::requireParamDataIsCompact(const char *name)
  {
    Data *data = getParam<Data *>(name);

    if (!data) {
      return;
    }

    if (!data->compact()) {
      throw std::runtime_error(toString() +
                               " only supports naturally strided data for '" +
                               name + "' array");
    }
  }

  // Helper functions /////////////////////////////////////////////////////////

  inline const ispc::Data1D *ispc(const Ref<const Data> &dataRef)
  {
    return dataRef ? &dataRef->ispc : &Data::emptyData1D;
  }

  template <typename T>
  const ispc::Data1D *ispc(const Ref<const DataT<T>> &dataRef)
  {
    return dataRef ? &dataRef->ispc : &Data::emptyData1D;
  }

  template <typename T>
  const ispc::Data1D *ispc(const DataT<T> &data)
  {
    return &data.ispc;
  }

  inline std::vector<const ispc::Data1D *> ispcs(
      const std::vector<Ref<const Data>> &dataRefs)
  {
    std::vector<const ispc::Data1D *> r;

    for (const auto &d : dataRefs) {
      r.push_back(ispc(d));
    }

    return r;
  }

  template <typename T>
  inline std::vector<const ispc::Data1D *> ispcs(
      const std::vector<Ref<const DataT<T>>> &dataRefs)
  {
    std::vector<const ispc::Data1D *> r;

    for (const auto &d : dataRefs) {
      r.push_back(ispc(d));
    }

    return r;
  }

}  // namespace openvkl
