// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string.h>
#include "../api/Device.h"

namespace openvkl {

  // C version ////////////////////////////////////////////

  inline void *BufferSharedCreate(Device *device, size_t size, size_t alignment)
  {
    return device->allocateSharedMemory(size, alignment);
  }

  inline void BufferSharedDelete(Device *device, void *ptr)
  {
    device->freeSharedMemory(ptr);
  }

  // C++ version ////////////////////////////////////////////

  template <typename T>
  struct BufferShared
  {
   private:
    void *m_ptr;
    size_t m_size;
    Device *m_device;

   public:
    BufferShared(Device *, size_t size);
    BufferShared(Device *, const std::vector<T> &v);
    T *sharedPtr()
    {
      return (T *)m_ptr;
    }
    size_t size()
    {
      return m_size;
    }
    ~BufferShared()
    {
      BufferSharedDelete(m_device, m_ptr);
    }
  };

  template <typename T>
  BufferShared<T>::BufferShared(Device *device, size_t size)
      : m_size(size),
        m_device(device),
        m_ptr(BufferSharedCreate(device, sizeof(T) * size, alignof(T)))
  {
  }

  template <typename T>
  BufferShared<T>::BufferShared(Device *device, const std::vector<T> &v)
      : m_size(v.size()),
        m_device(device),
        m_ptr(BufferSharedCreate(device, sizeof(T) * v.size(), alignof(T)))
  {
    memcpy(sharedPtr(), v.data(), sizeof(T) * m_size);
  }

  template <typename T, typename... Args>
  inline std::unique_ptr<BufferShared<T>> make_buffer_shared_unique(
      Args &&...args)
  {
    return std::unique_ptr<BufferShared<T>>(
        new BufferShared<T>(std::forward<Args>(args)...));
  }

}  // namespace openvkl
