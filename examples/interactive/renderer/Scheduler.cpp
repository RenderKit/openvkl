// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Scheduler.h"
#include "Renderer.h"

namespace openvkl {
  namespace examples {

    void Scheduler::Synchronous::renderFrame(Renderer &renderer) const
    {
      renderer.renderFrame();
    }

    // -------------------------------------------------------------------------

    void Scheduler::Asynchronous::start(Renderer &renderer) const
    {
      if (renderer.run.load()) {
        return;
      }

      renderer.run.store(true);

      renderer.renderThread = std::thread([&renderer]() {
        while (renderer.run.load()) {
          renderer.renderFrame();
        }
      });
    }

    void Scheduler::Asynchronous::stop(Renderer &renderer) const
    {
      if (renderer.run.load()) {
        renderer.run.store(false);
        renderer.renderThread.join();
      }
    }

    void Scheduler::Asynchronous::lock(Lock &lock) const
    {
      lock.lock();
    }

    void Scheduler::Asynchronous::unlock(Lock &lock) const
    {
      lock.unlock();
    }

    bool Scheduler::Asynchronous::workerMustTerminate(Renderer &renderer) const
    {
      return !renderer.run.load();
    }

    // -------------------------------------------------------------------------

    void Scheduler::start(Renderer &renderer) const
    {
      renderer.beforeStart();
      impl->start(renderer);
    }

    void Scheduler::stop(Renderer &renderer) const
    {
      impl->stop(renderer);
      renderer.afterStop();
    }

  }  // namespace examples
}  // namespace openvkl

