// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "BatchApplication.h"
#include "InteractiveApplication.h"
#include "renderer/Scene.h"

using namespace openvkl::examples;

int main(int argc, char **argv)
{
  Scene scene;

  std::list<std::string> args(argv, argv+argc);

  if (!scene.parseCommandLine(args)) {
    return 0;
  }

  initializeOpenVKL();

  if (scene.interactive) {
    InteractiveApplication app;
    app.run(scene);
  } else {
    BatchApplication app;
    app.run(scene);
  }

  shutdownOpenVKL();

  return 0;
}
