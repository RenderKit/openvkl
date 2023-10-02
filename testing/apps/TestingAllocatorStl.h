// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <memory>
#include <new>

namespace openvkl {
  namespace testing {

    template <class T>
    struct TestingAllocatorStl
    {
      typedef T value_type;

      TestingAllocatorStl() noexcept : queue(getSyclQueue()){};

      TestingAllocatorStl(sycl::queue queue) noexcept : queue(queue){};

      ~TestingAllocatorStl();

      template <class U>
      TestingAllocatorStl(const TestingAllocatorStl<U> &) noexcept
      {
      }

      template <class U>
      bool operator==(const TestingAllocatorStl<U> &) const noexcept
      {
        return true;
      }

      template <class U>
      bool operator!=(const TestingAllocatorStl<U> &) const noexcept
      {
        return false;
      }

      T *allocate(const size_t n);
      void deallocate(T *const p, size_t);

     private:
      sycl::queue queue;

      std::vector<void *> allocations;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <class T>
    TestingAllocatorStl<T>::~TestingAllocatorStl()
    {
      for (auto const &a : allocations) {
        sycl::free(a, queue);
      }

      allocations.clear();
    }

    template <class T>
    T *TestingAllocatorStl<T>::allocate(const size_t size)
    {
      if (size == 0) {
        return nullptr;
      }

      if (size > static_cast<size_t>(-1) / sizeof(T)) {
        throw std::bad_array_new_length();
      }

      T *buf = sycl::malloc_shared<T>(size, queue);

      allocations.push_back(static_cast<void *>(buf));

      return buf;
    }

    template <class T>
    void TestingAllocatorStl<T>::deallocate(T *const ptr, size_t)
    {
      if (!ptr) {
        return;
      }

      void *voidPtr = static_cast<void *>(ptr);

      auto iter = std::find(allocations.begin(), allocations.end(), voidPtr);

      if (iter != allocations.end()) {
        sycl::free(voidPtr, queue);
        allocations.erase(iter);
      } else {
        throw std::runtime_error(
            "TestingAllocatorStl: could not find allocation to deallocate");
      }
    }

    // alias for convenience
    template <typename T>
    using UsmVector = std::vector<T, TestingAllocatorStl<T>>;

  }  // namespace testing
}  // namespace openvkl
