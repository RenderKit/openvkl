// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "create_voxels.h"
#include "framebuffer.h"

#include <openvkl/openvkl.h>

int main(int argc, char **argv)
{
  vklLoadModule("cpu_device");
  VKLDevice device = vklNewDevice("cpu");
  vklCommitDevice(device);

  constexpr size_t res      = 128;
  std::vector<float> voxels = createVoxels(res);

  VKLVolume volume = vklNewVolume(device, "structuredRegular");
  vklSetVec3i(volume, "dimensions", res, res, res);

  const float spacing = 1.f / static_cast<float>(res);
  vklSetVec3f(volume, "gridSpacing", spacing, spacing, spacing);
  VKLData voxelData = vklNewData(
      device, voxels.size(), VKL_FLOAT, voxels.data(), VKL_DATA_SHARED_BUFFER);
  vklSetData(volume, "data", voxelData);
  vklRelease(voxelData);

  vklCommit(volume);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  Framebuffer fb(64, 32);

  // We trace the volume with simple ray marching.
  // Conceptually, this is a series of camera-aligned,
  // semi transparent planes.
  // We walk along the ray in regular steps.
  const int numSteps = 8;
  const float tMax = 1.f;
  const float tStep = tMax / numSteps;
  fb.generate([&](float fx, float fy) {
    Color color = {0.f};
    for (int i = 0; i < numSteps; ++i)
    {
        const vkl_vec3f p = {fx, fy, i * tStep};
        const Color c = transferFunction(vklComputeSample(sampler, &p));

        // We use the over operator to blend semi-transparent
        // "surfaces" together.
        color = over(color, c);

        // Now we've created a very simple volume renderer using 
        // Open VKL!
    }
    return color;
  });

  fb.drawToTerminal();

  vklRelease(sampler);
  vklRelease(volume);
  vklReleaseDevice(device);

  return 0;
}
