// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>

#include <openvkl/device/openvkl.h>
#include <iostream>
#include <iomanip>

void demoGpuAPI(sycl::queue &syclQueue, VKLDevice device, VKLVolume volume)
{
  std::cout << "demo of GPU API" << std::endl;

  std::cout << std::fixed << std::setprecision(6);

  VKLSampler sampler = vklNewSampler(volume);
  vklCommit2(sampler);

  // bounding box
  vkl_box3f bbox = vklGetBoundingBox(volume);
  std::cout << "\tbounding box" << std::endl;
  std::cout << "\t\tlower = " << bbox.lower.x << " " << bbox.lower.y << " "
            << bbox.lower.z << std::endl;
  std::cout << "\t\tupper = " << bbox.upper.x << " " << bbox.upper.y << " "
            << bbox.upper.z << std::endl
            << std::endl;

  // number of attributes
  unsigned int numAttributes = vklGetNumAttributes(volume);
  std::cout << "\tnum attributes = " << numAttributes << std::endl;

  // value range for all attributes
  for (unsigned int i = 0; i < numAttributes; i++) {
    vkl_range1f valueRange = vklGetValueRange(volume, i);
    std::cout << "\tvalue range (attribute " << i << ") = (" << valueRange.lower
              << " " << valueRange.upper << ")" << std::endl;
  }

  // coordinate for sampling / gradients
  vkl_vec3f coord = {1.f, 2.f, 3.f};
  std::cout << "\n\tcoord = " << coord.x << " " << coord.y << " " << coord.z
            << std::endl
            << std::endl;

  const float time = 0.f;

  // This is USM Shared allocation - it's required when
  // we want to pass result back from GPU
  float *sample = sycl::malloc_shared<float>(1, syclQueue);

  for (unsigned int attributeIndex = 0; attributeIndex < numAttributes;
       attributeIndex++) {
    syclQueue
        .single_task([=]() {
          *sample = vklComputeSample(&sampler, &coord, attributeIndex, time);
        })
        .wait();

    std::cout << "\tsampling computation (attribute " << attributeIndex << ")"
              << std::endl;
    std::cout << "\t\tsample = " << *sample << std::endl;
  }

  // Freeing USM Shared memory
  sycl::free(sample, syclQueue);

  vklRelease2(sampler);
}

int main()
{
  auto IntelGPUDeviceSelector = [](const sycl::device &device) {
    using namespace sycl::info;
    const std::string deviceName = device.get_info<device::name>();
    bool match                   = device.is_gpu() &&
                 (deviceName.find("Intel(R) Graphics") != std::string::npos) &&
                 device.get_backend() == sycl::backend::ext_oneapi_level_zero;
    return match;
  };

  sycl::queue syclQueue(IntelGPUDeviceSelector);
  sycl::context syclContext = syclQueue.get_context();

  std::cout << "Target SYCL device: "
            << syclQueue.get_device().get_info<sycl::info::device::name>()
            << std::endl
            << std::endl;

  vklInit();

  VKLDevice device = vklNewDevice("gpu_4");
  vklDeviceSetVoidPtr(device, "syclContext", static_cast<void *>(&syclContext));
  vklCommitDevice(device);

  const int dimensions[] = {128, 128, 128};

  const int numVoxels = dimensions[0] * dimensions[1] * dimensions[2];

  const int numAttributes = 3;

  VKLVolume volume = vklNewVolume(device, "structuredRegular");
  vklSetVec3i2(
      volume, "dimensions", dimensions[0], dimensions[1], dimensions[2]);
  vklSetVec3f2(volume, "gridOrigin", 0, 0, 0);
  vklSetVec3f2(volume, "gridSpacing", 1, 1, 1);

  std::vector<float> voxels(numVoxels);

  // volume attribute 0: x-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)i;

  VKLData data0 = vklNewData(
      device, numVoxels, VKL_FLOAT, voxels.data(), VKL_DATA_DEFAULT, 0);

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)j;

  VKLData data1 = vklNewData(
      device, numVoxels, VKL_FLOAT, voxels.data(), VKL_DATA_DEFAULT, 0);

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)k;

  VKLData data2 = vklNewData(
      device, numVoxels, VKL_FLOAT, voxels.data(), VKL_DATA_DEFAULT, 0);

  VKLData attributes[] = {data0, data1, data2};

  VKLData attributesData = vklNewData(
      device, numAttributes, VKL_DATA, attributes, VKL_DATA_DEFAULT, 0);

  vklRelease(data0);
  vklRelease(data1);
  vklRelease(data2);

  vklSetData2(volume, "data", attributesData);
  vklRelease(attributesData);

  vklCommit2(volume);

  demoGpuAPI(syclQueue, device, volume);

  vklRelease2(volume);

  vklReleaseDevice(device);

  std::cout << "complete." << std::endl;

  return 0;
}
