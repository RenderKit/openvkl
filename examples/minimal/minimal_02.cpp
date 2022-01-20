// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "framebuffer.h"

// We must include the openvkl header.
#include <openvkl/openvkl.h>

int main(int argc, char **argv)
{
  // To initialize Open VKL, load the device module, which is essentially the
  // backend implementation. Our current release supports only "cpu_device",
  // which is highly optimized for vector CPU architectures.
  vklLoadModule("cpu_device");

  // The device itself will be manage all resources. "cpu" is the default and
  // selects the native vector width for best performance.
  VKLDevice device = vklNewDevice("cpu");

  // Devices must be committed before use. This is because they support
  // parameters, such as logging verbosity.
  vklCommitDevice(device);

  Framebuffer fb(64, 32);

  fb.generate([&](float fx, float fy) { 
    return transferFunction(2*fx-1); 
  });
  fb.drawToTerminal();

  // When the application is done with the device, release it!
  // This will clean up the internal state.
  vklReleaseDevice(device);

  return 0;
}
