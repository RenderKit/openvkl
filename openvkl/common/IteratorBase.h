// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../api/Device.h"
#include "rkcommon/memory/IntrusivePtr.h"

namespace openvkl {

  struct IteratorBase
  {
    // Not a Ref<>! Destructors will not run.
    Device *device;

    // WORKAROUND ICC 15: This destructor must be public!
    virtual ~IteratorBase() = default;

    /*
     * Accessor to internal kernel structs for DPCPP kernels.
     */
    void *kernelStorage{nullptr};
  };

}  // namespace openvkl
