// Copyright 2021 Intel Corporation
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

  // One advantage of Open VKL is that we can use a different data structure
  // with the same sampling API.
  // Here, we replace our data structure with a structured spherical volume
  // for a spherical domain.
  VKLVolume volume = vklNewVolume(device, "structuredSpherical");

  vklSetVec3i(volume, "dimensions", res, res, res);
  const float spacing = 1.f / static_cast<float>(res);
  // We must adapt gridSpacing, as structuredSpherical expects spacing
  // in spherical coordinates.
  vklSetVec3f(volume, "gridSpacing", spacing, 180.f*spacing, 360.f*spacing);

  VKLData voxelData = vklNewData(
      device, voxels.size(), VKL_FLOAT, voxels.data(), VKL_DATA_SHARED_BUFFER);
  vklSetData(volume, "data", voxelData);
  vklRelease(voxelData);

  vklCommit(volume);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit(sampler);

  Framebuffer fb(64, 32);

  fb.generate([&](float fx, float fy) {
    // Also try slice 1.0 to demonstrate a different view.
    const vkl_vec3f p = {fx, fy, 0.f};
    return transferFunction(vklComputeSample(sampler, &p));
  });

  fb.drawToTerminal();

  vklRelease(sampler);
  vklRelease(volume);
  vklReleaseDevice(device);

  return 0;
}
