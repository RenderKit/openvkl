// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
// openvkl
#include "openvkl/openvkl.h"
// rkcommon
#include "rkcommon/common.h"

void initializeOpenVKL()
{
  static bool initialized = false;

  if (!initialized) {
    vklLoadModule("ispc_driver");

    VKLDriver driver = vklNewDriver("ispc");
    vklCommitDriver(driver);
    vklSetCurrentDriver(driver);

    initialized = true;
  }
}
