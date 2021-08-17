// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <list>
#include <vector>

namespace openvkl {
  namespace examples {

    struct Scene;
    class Scheduler;

    class BatchApplication
    {
     public:
      BatchApplication();
      ~BatchApplication();
      void run(Scene &scene);
    };

  }  // namespace examples
}  // namespace openvkl


