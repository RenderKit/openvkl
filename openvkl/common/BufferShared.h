// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispcrt.hpp"
#include <string.h>

namespace openvkl {

extern OPENVKL_CORE_INTERFACE ispcrt::Context *g_ispcrtContext;

// C version ////////////////////////////////////////////

inline ISPCRTMemoryView BufferSharedCreate(size_t size)
{
  ISPCRTNewMemoryViewFlags flags;
  flags.allocType = ISPCRT_ALLOC_TYPE_SHARED;
  return ispcrtNewMemoryViewForContext(
      g_ispcrtContext->handle(), nullptr, size, &flags);
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
  BufferShared();
  BufferShared(size_t size);
  BufferShared(const std::vector<T> &v);
  BufferShared(const T *data, size_t size);
};

template <typename T>
BufferShared<T>::BufferShared()
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        *g_ispcrtContext)
{}

template <typename T>
BufferShared<T>::BufferShared(size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        *g_ispcrtContext, size)
{}

template <typename T>
BufferShared<T>::BufferShared(const std::vector<T> &v)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        *g_ispcrtContext, v.size())
{
  memcpy(sharedPtr(), v.data(), sizeof(T) * v.size());
}

template <typename T>
BufferShared<T>::BufferShared(const T *data, size_t size)
    : ispcrt::Array<T, ispcrt::AllocType::Shared>(
        *g_ispcrtContext, size)
{
  memcpy(sharedPtr(), data, sizeof(T) * size);
}

template <typename T, typename... Args>
inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
    Args &&... args)
{
  return std::unique_ptr<BufferShared<T>>(
      new BufferShared<T>(std::forward<Args>(args)...));
}

} // namespace ospray
