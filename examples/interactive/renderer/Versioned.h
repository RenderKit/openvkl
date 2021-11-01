// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <cstdint>
#include "Scheduler.h"

namespace openvkl {
  namespace examples {

    template <class T>
    class Versioned : public Scheduler::Lockable
    {
     public:
      uint64_t getVersion() const
      {
        return version.load();
      }

      void incrementVersion()
      {
        ++version;
      }

      /*
       * Copy our internal object if the version has changed.
       * Returns true if the output was updated.
       */
      bool updateIfChanged(const Versioned &other)
      {
        bool changed = (version != other.version);
        if (changed) {
          obj     = other.obj;
          version.store(other.version.load());
        }
        return changed;
      }

      template <class... Args>
      Versioned(Args &&...args) : obj{std::forward<Args>(args)...}
      {
      }

      T &operator*()
      {
        return obj;
      }

      const T &operator*() const
      {
        return obj;
      }

      T *operator->()
      {
        return &obj;
      }

      const T *operator->() const
      {
        return &obj;
      }

     private:
      std::atomic<uint64_t> version{static_cast<uint64_t>(-1)};
      T obj;
    };

  }  // namespace examples
}  // namespace openvkl
