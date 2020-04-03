// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"
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

}  // namespace openvkl
