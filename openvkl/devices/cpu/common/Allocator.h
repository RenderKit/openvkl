// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <new>
#include "../common/ManagedObject.h"

namespace openvkl {
  namespace cpu_device {

    //////////////////////////////////////////////////////////////////////////
    // Allocate and deallocate aligned blocks of memory safely, and keep stats
    //////////////////////////////////////////////////////////////////////////
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

    //////////////////////////////////////////////////////////////////////////
    // Allocator like the above, but which can be used with STL containers.
    //
    // Note: we can't inherit from the above due to restrictions on copy
    // constructors, etc. which are required by STL containers.
    //////////////////////////////////////////////////////////////////////////

    template <class T>
    struct AllocatorStl
    {
      typedef T value_type;

      AllocatorStl(Device *device) noexcept : device(device){};

      ~AllocatorStl();

      template <class U>
      AllocatorStl(const AllocatorStl<U> &) noexcept
      {
      }

      template <class U>
      bool operator==(const AllocatorStl<U> &) const noexcept
      {
        return true;
      }

      template <class U>
      bool operator!=(const AllocatorStl<U> &) const noexcept
      {
        return false;
      }

      T *allocate(const size_t n);
      void deallocate(T *const p, size_t);

     private:
      Device *device{nullptr};

      std::map<void *, api::memstate *> ptrToMemState;
    };

    // -------------------------------------------------------------------------

    template <class T>
    AllocatorStl<T>::~AllocatorStl()
    {
      for (auto const &m : ptrToMemState) {
        this->device->freeMemState(m.second);
      }
    }

    template <class T>
    T *AllocatorStl<T>::allocate(const size_t size)
    {
      if (size == 0) {
        return nullptr;
      }

      if (size > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }

      const size_t alignment = alignof(T);

      const size_t numBytes = size * sizeof(T) + alignment;

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
    void AllocatorStl<T>::deallocate(T *const ptr, size_t)
    {
      if (!ptr) {
        return;
      }

      void *voidPtr = static_cast<void *>(ptr);

      if (ptrToMemState.count(voidPtr)) {
        this->device->freeMemState(ptrToMemState[voidPtr]);
        ptrToMemState.erase(voidPtr);
      } else {
        throw std::runtime_error(
            "Allocator::deallocate(): cannot find memstate for ptr");
      }
    }

  }  // namespace cpu_device
}  // namespace openvkl
