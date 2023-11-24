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
    };

    // -------------------------------------------------------------------------

    template <class T>
    inline T *Allocator::allocate(size_t size)
    {
      const size_t numBytes = size * sizeof(T);

      void *memory = this->device->allocateSharedMemory(numBytes, alignof(T));
      if (!memory) {
        throw std::bad_alloc();
      }
      std::memset(memory, 0, numBytes);
      return reinterpret_cast<T *>(memory);
    }

    template <class T>
    inline void Allocator::deallocate(T *&ptr)
    {
      if (!ptr) {
        return;
      }
      this->device->freeSharedMemory(ptr);
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

      AllocatorStl(Device *device) : m_device(device)
      {
        assert(m_device);
      };

      ~AllocatorStl()                               = default;

      template <class U>
      bool operator==(const AllocatorStl<U> &) const = delete;

      template <class U>
      bool operator!=(const AllocatorStl<U> &) const = delete;

      template <class U>
      AllocatorStl(const AllocatorStl<U> &o)
      {
        m_device = o.getDevice();
        assert(m_device);
      }

      T *allocate(const size_t n);
      void deallocate(T *const p, size_t);
      Device *getDevice() const
      {
        return m_device;
      };

     private:
      Device *m_device{nullptr};
    };

    // -------------------------------------------------------------------------

    template <class T>
    T *AllocatorStl<T>::allocate(const size_t size)
    {
      if (size == 0) {
        return nullptr;
      }

      if (size > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }

      const size_t numBytes = size * sizeof(T);

      void *memory = m_device->allocateSharedMemory(numBytes, alignof(T));
      if (!memory) {
        throw std::bad_alloc();
      }

      std::memset(memory, 0, numBytes);

      return reinterpret_cast<T *>(memory);
    }

    template <class T>
    void AllocatorStl<T>::deallocate(T *const ptr, size_t)
    {
      if (!ptr) {
        return;
      }
      m_device->freeSharedMemory(ptr);
    }

  }  // namespace cpu_device
}  // namespace openvkl
