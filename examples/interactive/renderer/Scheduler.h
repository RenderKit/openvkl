// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <rkcommon/common.h>
#include <atomic>
#include <memory>
#include <mutex>

namespace openvkl {
  namespace examples {

    /*
     * All renderers must implement this interface.
     */
    class Renderer;

    /*
     * This class schedules rendering, either synchronously or
     * asynchronously.
     */
    class Scheduler
    {
     public:
      using Mutex = std::mutex;
      using Lock  = std::unique_lock<Mutex>;

      class Lockable
      {
       public:
        virtual ~Lockable() = default;

       private:
        friend class Scheduler;
        mutable Mutex lockableMutex;
      };

      class Impl
      {
       public:
        virtual ~Impl()                                            = default;
        virtual void start(Renderer &renderer) const               = 0;
        virtual void renderFrame(Renderer &renderer) const         = 0;
        virtual void stop(Renderer &renderer) const                = 0;
        virtual void lock(Lock &lock) const                        = 0;
        virtual void unlock(Lock &lock) const                      = 0;
        virtual bool workerMustTerminate(Renderer &renderer) const = 0;
      };

      class Synchronous : public Impl
      {
       public:
        void start(Renderer &renderer) const override final {}
        void renderFrame(Renderer &renderer) const override final;
        void stop(Renderer &renderer) const override final {}
        void lock(Lock & /*lock*/) const override final {}
        void unlock(Lock & /*lock*/) const override final {}
        bool workerMustTerminate(Renderer &renderer) const override final
        {
          return false;
        }
      };

      class Asynchronous : public Impl
      {
       public:
        void start(Renderer &renderer) const override final;
        void renderFrame(Renderer &renderer) const override final {}
        void stop(Renderer &renderer) const override final;
        void lock(Lock &lock) const override final;
        void unlock(Lock &lock) const override final;
        bool workerMustTerminate(Renderer &renderer) const override final;
      };

     public:
      explicit Scheduler(bool synchronous = true)
      {
        if (synchronous) {
          impl = rkcommon::make_unique<Synchronous>();
        } else {
          impl = rkcommon::make_unique<Asynchronous>();
        }
      }

      void start(Renderer &renderer) const;
      void stop(Renderer &renderer) const;

      void renderFrame(Renderer &renderer) const
      {
        impl->renderFrame(renderer);
      }

      bool workerMustTerminate(Renderer &renderer) const
      {
        return impl->workerMustTerminate(renderer);
      }

      template <class F>
      void locked(const Lockable &lockable, const F &functor) const
      {
        std::unique_lock<std::mutex> lock(lockable.lockableMutex,
                                          std::defer_lock);
        impl->lock(lock);
        functor();
        impl->unlock(lock);
      }

     private:
      std::unique_ptr<Impl> impl;
    };

  }  // namespace examples
}  // namespace openvkl
