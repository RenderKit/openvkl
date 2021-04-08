// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
// rkcommon
#include "rkcommon/common.h"

static VKLDevice device = nullptr;

void initializeOpenVKL()
{
  if (!device) {
    vklLoadModule("cpu_device");

    device = vklNewDevice("cpu");
    vklCommitDevice(device);
  }
}

void shutdownOpenVKL()
{
  if (device) {
    vklReleaseDevice(device);
    device = nullptr;
  }
}

VKLDevice getOpenVKLDevice()
{
  return device;
}
