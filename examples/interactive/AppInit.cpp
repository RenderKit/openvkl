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
    vklLoadModule("cpu_device");

    VKLDevice device = vklNewDevice("cpu");
    vklCommitDevice(device);
    vklSetCurrentDevice(device);

    initialized = true;
  }
}
