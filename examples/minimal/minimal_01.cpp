// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "framebuffer.h"

int main(int argc, char **argv)
{
  Framebuffer fb(64, 32);

  fb.generate([&](float fx, float fy) { 
    return transferFunction(2*fx-1); 
  });
  fb.drawToTerminal();

  return 0;
}
