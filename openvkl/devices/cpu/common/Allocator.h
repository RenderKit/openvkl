// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include "../common/ManagedObject.h"

namespace openvkl {
  namespace cpu_device {

    /*
     * Allocate and deallocate aligned blocks of
     * memory safely, and keep some stats.
     */
    class Allocator : public ManagedObject
    {
     public:
      Allocator(Device *device) : ManagedObject(device){};
      Allocator(const Allocator &)            = delete;
      Allocator &operator=(const Allocator &) = delete;
      Allocator(Allocator &&)                 = delete;
      Allocator &operator=(Allocator &&)      = delete;
      ~Allocator()                            = default;

      template <class T>
      T *allocate(size_t size);

      template <class T>
      void deallocate(T *&ptr);

     private:
      std::atomic<size_t> bytesAllocated{0};
      std::map<void *, api::memstate *> ptrToMemState;
    };

    // -------------------------------------------------------------------------

    template <class T>
    inline T *Allocator::allocate(size_t size)
    {
      //  matches default for rkcommon::memory::alignedMalloc(), which was used
      //  before
      const size_t alignment = 64;

      const size_t numBytes = size * sizeof(T) + alignment;
      bytesAllocated += numBytes;

      api::memstate *m = this->device->allocateBytes(numBytes);
      if (!m->allocatedBuffer) {
        throw std::bad_alloc();
      }

      // std::align() may modify ptr and space, so use temporaries here
      void *ptr    = m->allocatedBuffer;
      size_t space = numBytes;

      void *alignedBuffer = std::align(alignment, size * sizeof(T), ptr, space);
      if (!alignedBuffer) {
        throw std::bad_alloc();
      }

      T *buf = reinterpret_cast<T *>(alignedBuffer);
      std::memset(buf, 0, size * sizeof(T));

      ptrToMemState[buf] = m;

      return buf;
    }

    template <class T>
    inline void Allocator::deallocate(T *&ptr)
    {
      if (!ptr) {
        return;
      }

      if (ptrToMemState.count(ptr)) {
        this->device->freeMemState(ptrToMemState[ptr]);
        ptrToMemState.erase(ptr);
      } else {
        throw std::runtime_error(
            "Allocator::deallocate(): cannot find memstate for ptr");
      }

      ptr = nullptr;
    }

  }  // namespace cpu_device
}  // namespace openvkl
