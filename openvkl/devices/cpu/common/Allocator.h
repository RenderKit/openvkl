// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include "../common/ManagedObject.h"
#include "rkcommon/memory/malloc.h"

namespace openvkl {
  namespace cpu_device {

    /*
     * Allocate and deallocate aligned blocks of
     * memory safely, and keep some stats.
     */
    class Allocator : public ManagedObject
    {
     public:
      Allocator(Device *device) : ManagedObject(device) {};
      Allocator(const Allocator &) = delete;
      Allocator &operator=(const Allocator &) = delete;
      Allocator(Allocator &&)                 = delete;
      Allocator &operator=(Allocator &&) = delete;
      ~Allocator()                       = default;

      template <class T>
      T *allocate(size_t size);

      template <class T>
      void deallocate(T *&ptr);

     private:
      std::atomic<size_t> bytesAllocated{0};
    };

    // -------------------------------------------------------------------------

    template <class T>
    inline T *Allocator::allocate(size_t size)
    {
      const size_t numBytes = size * sizeof(T);
      bytesAllocated += numBytes;
      T *buf = reinterpret_cast<T *>(rkcommon::memory::alignedMalloc(numBytes));
      if (!buf)
        throw std::bad_alloc();
      std::memset(buf, 0, numBytes);
      return buf;
    }

    template <class T>
    inline void Allocator::deallocate(T *&ptr)
    {
      rkcommon::memory::alignedFree(ptr);
      ptr = nullptr;
    }

  }  // namespace cpu_device
}  // namespace openvkl

