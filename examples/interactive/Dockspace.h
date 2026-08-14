// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace openvkl {
  namespace examples {

    // separate, host-only target to workaround DPC++ hang
    // see https://github.com/intel/llvm/issues/22972
    void initDockspace(unsigned &leftNodeId,
                       unsigned &centerNodeId,
                       unsigned &rightNodeId);

  }  // namespace examples
}  // namespace openvkl
