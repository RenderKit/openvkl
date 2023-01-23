// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string.h>
#include "../api/Device.h"
#include "ispcrt.hpp"

namespace openvkl {

  // C version ////////////////////////////////////////////

  inline ISPCRTMemoryView BufferSharedCreate(Device *device, size_t size)
  {
    ispcrt::Context *context = (ispcrt::Context *)device->getContext();
    ISPCRTNewMemoryViewFlags flags;
    flags.allocType = ISPCRT_ALLOC_TYPE_SHARED;
    return ispcrtNewMemoryViewForContext(
        context->handle(), nullptr, size, &flags);
  }

  inline void BufferSharedDelete(ISPCRTMemoryView view)
  {
    ispcrtRelease(view);
  }

  // C++ version ////////////////////////////////////////////

  template <typename T>
  struct BufferShared : public ispcrt::Array<T, ispcrt::AllocType::Shared>
  {
    using ispcrt::Array<T, ispcrt::AllocType::Shared>::sharedPtr;
    BufferShared(Device *device);
    BufferShared(Device *, size_t size);
    BufferShared(Device *, const std::vector<T> &v);
    BufferShared(Device *, const T *data, size_t size);
  };

  template <typename T>
  BufferShared<T>::BufferShared(Device *device)
      : ispcrt::Array<T, ispcrt::AllocType::Shared>(
            *(ispcrt::Context *)device->getContext())
  {
    // ISPCRT lazily allocates on first access of pointer; force allocation on
    // construction to maintain thread safety
    sharedPtr();
  }

  template <typename T>
  BufferShared<T>::BufferShared(Device *device, size_t size)
      : ispcrt::Array<T, ispcrt::AllocType::Shared>(
            *(ispcrt::Context *)device->getContext(), size)
  {
    // ISPCRT lazily allocates on first access of pointer; force allocation on
    // construction to maintain thread safety
    sharedPtr();
  }

  template <typename T>
  BufferShared<T>::BufferShared(Device *device, const std::vector<T> &v)
      : ispcrt::Array<T, ispcrt::AllocType::Shared>(
            *(ispcrt::Context *)device->getContext(), v.size())
  {
    memcpy(sharedPtr(), v.data(), sizeof(T) * v.size());
  }

  template <typename T>
  BufferShared<T>::BufferShared(Device *device, const T *data, size_t size)
      : ispcrt::Array<T, ispcrt::AllocType::Shared>(
            *(ispcrt::Context *)device->getContext(), size)
  {
    memcpy(sharedPtr(), data, sizeof(T) * size);
  }

  template <typename T, typename... Args>
  inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
      Args &&...args)
  {
    return std::unique_ptr<BufferShared<T>>(
        new BufferShared<T>(std::forward<Args>(args)...));
  }

}  // namespace openvkl
