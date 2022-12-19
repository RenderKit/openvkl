// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>

#include <openvkl/device/openvkl.h>
#include <iomanip>
#include <iostream>

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

  std::cout << std::endl << "\tsampling" << std::endl;

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

  sycl::free(sample, syclQueue);

  std::cout << std::endl << "\tinterval iteration" << std::endl << std::endl;

  // interval iterator context setup
  std::vector<vkl_range1f> ranges{{10, 20}, {50, 75}};
  VKLData rangesData =
      vklNewData(device, ranges.size(), VKL_BOX1F, ranges.data());

  VKLIntervalIteratorContext intervalContext =
      vklNewIntervalIteratorContext(sampler);

  vklSetInt2(intervalContext, "attributeIndex", 0);

  vklSetData2(intervalContext, "valueRanges", rangesData);
  vklRelease(rangesData);

  vklCommit2(intervalContext);

  // ray definition for iterators
  vkl_vec3f rayOrigin{0.f, 1.f, 1.f};
  vkl_vec3f rayDirection{1.f, 0.f, 0.f};
  vkl_range1f rayTRange{0.f, 200.f};
  std::cout << "\trayOrigin = " << rayOrigin.x << " " << rayOrigin.y << " "
            << rayOrigin.z << std::endl;
  std::cout << "\trayDirection = " << rayDirection.x << " " << rayDirection.y
            << " " << rayDirection.z << std::endl;
  std::cout << "\trayTRange = " << rayTRange.lower << " " << rayTRange.upper
            << std::endl
            << std::endl;

  // interval iteration
  char *iteratorBuffer = sycl::malloc_device<char>(
      vklGetIntervalIteratorSize(&intervalContext), syclQueue);

  int *numIntervals = sycl::malloc_shared<int>(1, syclQueue);
  *numIntervals     = 0;

  const size_t maxNumIntervals = 999;

  VKLInterval *intervalsBuffer =
      sycl::malloc_shared<VKLInterval>(maxNumIntervals, syclQueue);
  memset(intervalsBuffer, 0, maxNumIntervals * sizeof(VKLInterval));

  std::cout << "\tinterval iterator for value ranges";

  for (const auto &r : ranges) {
    std::cout << " {" << r.lower << " " << r.upper << "}";
  }
  std::cout << std::endl << std::endl;

  syclQueue
      .single_task([=]() {
        VKLIntervalIterator intervalIterator =
            vklInitIntervalIterator(&intervalContext,
                                    &rayOrigin,
                                    &rayDirection,
                                    &rayTRange,
                                    time,
                                    (void *)iteratorBuffer);

        for (;;) {
          VKLInterval interval;
          int result = vklIterateInterval(intervalIterator, &interval);
          if (!result) {
            break;
          }
          intervalsBuffer[*numIntervals] = interval;

          *numIntervals = *numIntervals + 1;
          if (*numIntervals >= maxNumIntervals)
            break;
        }
      })
      .wait();

  for (int i = 0; i < *numIntervals; ++i) {
    std::cout << "\t\ttRange (" << intervalsBuffer[i].tRange.lower << " "
              << intervalsBuffer[i].tRange.upper << ")" << std::endl;
    std::cout << "\t\tvalueRange (" << intervalsBuffer[i].valueRange.lower
              << " " << intervalsBuffer[i].valueRange.upper << ")" << std::endl;
    std::cout << "\t\tnominalDeltaT " << intervalsBuffer[i].nominalDeltaT
              << std::endl
              << std::endl;
  }

  sycl::free(iteratorBuffer, syclQueue);
  sycl::free(numIntervals, syclQueue);
  sycl::free(intervalsBuffer, syclQueue);

  vklRelease2(intervalContext);

  // hit iteration
  std::cout << std::endl << "\thit iteration" << std::endl << std::endl;

  // hit iterator context setup
  float values[2] = {32.f, 96.f};
  int num_values = 2;
  VKLData valuesData =
      vklNewData(device, num_values, VKL_FLOAT, values);

  VKLHitIteratorContext hitContext = vklNewHitIteratorContext(sampler);

  vklSetInt2(hitContext, "attributeIndex", 0);

  vklSetData2(hitContext, "values", rangesData);
  vklRelease(rangesData);

  vklCommit2(hitContext);

  // ray definition for iterators
  // see rayOrigin, Direction and TRange above

  char *hitIteratorBuffer = sycl::malloc_device<char>(
      vklGetHitIteratorSize(&hitContext), syclQueue);

  int *numHits = sycl::malloc_shared<int>(1, syclQueue);
  *numHits     = 0;

  const size_t maxNumHits = 999;

  VKLHit *hitBuffer =
      sycl::malloc_shared<VKLHit>(maxNumHits, syclQueue);
  memset(hitBuffer, 0, maxNumHits * sizeof(VKLHit));

  std::cout << "\thit iterator for values";

  for (const auto &r : values) {
    std::cout << " " << r << " ";
  }
  std::cout << std::endl << std::endl;

  syclQueue
      .single_task([=]() {
        VKLHitIterator hitIterator =
            vklInitHitIterator(&hitContext,
                               &rayOrigin,
                               &rayDirection,
                               &rayTRange,
                               time,
                               (void *)hitIteratorBuffer);

        for (;;) {
          VKLHit hit;
          int result = vklIterateHit(hitIterator, &hit);
          if (!result) {
            break;
          }
          hitBuffer[*numHits] = hit;

          *numHits = *numHits + 1;
          if (*numHits >= maxNumHits)
            break;
        }
      })
      .wait();

  for (int i = 0; i < *numHits; ++i) {
    std::cout << "\t\tt " << hitBuffer[i].t << std::endl;
    std::cout << "\t\tsample " << hitBuffer[i].sample << std::endl;
    std::cout << "\t\tepsilon " << hitBuffer[i].epsilon<< std::endl
              << std::endl;
  }

  sycl::free(hitIteratorBuffer, syclQueue);
  sycl::free(numHits, syclQueue);
  sycl::free(hitBuffer, syclQueue);

  vklRelease2(hitContext);

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

  VKLData data0 = vklNewData(device, numVoxels, VKL_FLOAT, voxels.data());

  // volume attribute 1: y-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)j;

  VKLData data1 = vklNewData(device, numVoxels, VKL_FLOAT, voxels.data());

  // volume attribute 2: z-grad
  for (int k = 0; k < dimensions[2]; k++)
    for (int j = 0; j < dimensions[1]; j++)
      for (int i = 0; i < dimensions[0]; i++)
        voxels[k * dimensions[0] * dimensions[1] + j * dimensions[2] + i] =
            (float)k;

  VKLData data2 = vklNewData(device, numVoxels, VKL_FLOAT, voxels.data());

  VKLData attributes[] = {data0, data1, data2};

  VKLData attributesData =
      vklNewData(device, numAttributes, VKL_DATA, attributes);

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
