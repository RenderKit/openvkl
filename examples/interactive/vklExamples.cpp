// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BatchApplication.h"
#include "InteractiveApplication.h"
#include "renderer/Scene.h"

using namespace openvkl::examples;

int main(int argc, char **argv)
{
  Scene scene;

  std::list<std::string> args(argv, argv + argc);

  if (!scene.parseCommandLine(args)) {
    return 0;
  }

  initializeOpenVKL();

  try {
    if (scene.interactive) {
      InteractiveApplication app;
      app.run(scene);
    } else {
      BatchApplication app;
      app.run(scene);
    }
  } catch (const std::exception &e) {
    // Handle fatal errors here. This mainly applies when invalid
    // volume parameters are specified on the command line, so that no volume
    // can be created at all.
    std::cerr << e.what() << std::endl;
  }

  shutdownOpenVKL();

  return 0;
}
