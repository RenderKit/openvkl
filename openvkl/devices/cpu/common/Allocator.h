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

      AllocatorStl(Device *device) noexcept : m_device(device){};

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
      Device *m_device{nullptr};

      // We need to track all allocation so we can free them
      // when object will be destroyed.
      std::vector<void *> m_allocations;
    };

    // -------------------------------------------------------------------------

    template <class T>
    AllocatorStl<T>::~AllocatorStl()
    {
      for (auto const &allocation : m_allocations) {
        m_device->freeSharedMemory(allocation);
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

      const size_t numBytes = size * sizeof(T);

      void *memory = m_device->allocateSharedMemory(numBytes, alignof(T));
      if (!memory) {
        throw std::bad_alloc();
      }

      std::memset(memory, 0, numBytes);

      m_allocations.push_back(memory);

      return reinterpret_cast<T *>(memory);
    }

    template <class T>
    void AllocatorStl<T>::deallocate(T *const ptr, size_t)
    {
      if (!ptr) {
        return;
      }

      auto it = std::find(m_allocations.begin(), m_allocations.end(), ptr);
      if (it == m_allocations.end()) {
        throw std::runtime_error(
            "AllocatorStl::deallocate(): cannot find allocation for ptr");
      }
      m_device->freeSharedMemory(ptr);
      m_allocations.erase(it);
    }

  }  // namespace cpu_device
}  // namespace openvkl
