// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstdint>
#include <utility>

namespace openvkl {
  namespace cpu_device {

    using std::size_t;

    /*
     * Return the required buffer size for objects to be safely placed using
     * placement new.
     */
    constexpr inline size_t alignedSize(size_t size, size_t alignment)
    {
      return (size > 0 && alignment > 0) ? size + alignment - 1 : 0;
    }

    template <class T>
    constexpr inline size_t alignedSize()
    {
      return sizeof(T) + alignof(T) - 1;
    }

    /*
     * Align a pointer to the next N byte boundary.
     */
    inline void *align(void *buffer, size_t alignment)
    {
      assert((alignment & (alignment-1)) == 0);
      void *aligned = nullptr;
      if (alignment > 0) {
        const std::intptr_t aoffs = std::intptr_t(alignment) - 1;
        const std::intptr_t amask = ~aoffs;
        aligned                   = reinterpret_cast<void *>(
            (reinterpret_cast<std::intptr_t>(buffer) + aoffs) & amask);
      }
      return aligned;
    }

    template <class T>
    inline void *align(void *buffer)
    {
      static_assert(alignof(T) > 0, "Invalid alignment for type T");
      static_assert((alignof(T) & (alignof(T)-1)) == 0, "Alignment of type T is not a power of 2.");
      const std::intptr_t aoffs = std::intptr_t(alignof(T)) - 1;
      const std::intptr_t amask = ~aoffs;
      return reinterpret_cast<void *>(
          (reinterpret_cast<std::intptr_t>(buffer) + aoffs) & amask);
    }

    /*
     * Offset a pointer by offset bytes and then align it.
     */
    inline void *alignOffset(void *buffer, size_t alignment, size_t offset)
    {
      return align(reinterpret_cast<void *>(
                       reinterpret_cast<std::intptr_t>(buffer) + offset),
                   alignment);
    }

  }  // namespace cpu_device
}  // namespace openvkl

