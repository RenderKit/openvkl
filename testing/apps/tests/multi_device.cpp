// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <atomic>
#include <thread>
#include "../../external/catch.hpp"
#include "openvkl_testing.h"
#include "sampling_utility.h"

using namespace openvkl::testing;

struct DeviceContext
{
  DeviceContext() = delete;

  DeviceContext(VKLDevice device,
                std::shared_ptr<XProceduralVolume> proceduralVolume)
      : device(device), proceduralVolume(proceduralVolume)
  {
  }

  ~DeviceContext()
  {
    proceduralVolume = nullptr;

    if (device) {
      vklReleaseDevice(device);
      device = nullptr;
    }
  }

  VKLDevice device = nullptr;

  // this holds a VKLVolume handle, which will be released upon destruction
  std::shared_ptr<XProceduralVolume> proceduralVolume;
};

// returns a bool instead of using REQUIRE() to allow use in multi-threaded
// Catch2 tests
bool test_value_range(const std::shared_ptr<DeviceContext> deviceContext)
{
  VKLVolume vklVolume =
      deviceContext->proceduralVolume->getVKLVolume(deviceContext->device);

  vkl_range1f apiValueRange = vklGetValueRange(vklVolume);

  range1f computedValueRange =
      deviceContext->proceduralVolume->getComputedValueRange();

  return ((apiValueRange.lower == computedValueRange.lower &&
           apiValueRange.upper == computedValueRange.upper));
}

// returns a bool instead of using REQUIRE() to allow use in multi-threaded
// Catch2 tests
bool test_sampling(const std::shared_ptr<DeviceContext> deviceContext)
{
  bool success = true;

  VKLVolume vklVolume =
      deviceContext->proceduralVolume->getVKLVolume(deviceContext->device);
  VKLSampler vklSampler = vklNewSampler(vklVolume);
  vklCommit(vklSampler);

  // first, gather object coordinates and procecural (truth) values
  const vec3i step(1);
  multidim_index_sequence<3> mis(
      deviceContext->proceduralVolume->getDimensions() / step);

  std::vector<vec3f> objectCoordinates;
  std::vector<float> proceduralValues;

  for (const auto &offset : mis) {
    const auto offsetWithStep = offset * step;

    vec3f oc =
        deviceContext->proceduralVolume->transformLocalToObjectCoordinates(
            offsetWithStep);

    const float proceduralValue =
        deviceContext->proceduralVolume->computeProceduralValue(oc);

    objectCoordinates.push_back(oc);
    proceduralValues.push_back(proceduralValue);
  }

  assert(objectCoordinates.size() == proceduralValues.size());

  // now test scalar sampling
  const float tol = 1e-4f;

  for (size_t i = 0; i < objectCoordinates.size(); i++) {
    const float scalarSampledValue =
        vklComputeSample(vklSampler, (const vkl_vec3f *)&objectCoordinates[i]);

    success =
        success && (fabs(proceduralValues[i] - scalarSampledValue) <= tol);
  }

  // and finally test stream sampling
  std::vector<float> streamSamples(objectCoordinates.size());

  vklComputeSampleN(vklSampler,
                    objectCoordinates.size(),
                    (const vkl_vec3f *)objectCoordinates.data(),
                    streamSamples.data());

  for (size_t i = 0; i < objectCoordinates.size(); i++) {
    success = success && (fabs(proceduralValues[i] - streamSamples[i]) <= tol);
  }

  vklRelease(vklSampler);

  return success;
}

TEST_CASE("Multiple devices", "[multi_device]")
{
  vklLoadModule("cpu_device");

  // create multiple devices and objects on those devices
  std::vector<std::shared_ptr<DeviceContext>> deviceContexts;

  const int numDevices = 4;

  for (int i = 0; i < numDevices; i++) {
    VKLDevice device = vklNewDevice("cpu");
    vklCommitDevice(device);

    const vec3i dimensions(128);
    const vec3f gridOrigin(0.f);

    // each context's volume will have a unique grid spacing to differentiate
    // the results
    const vec3f gridSpacing(1.f + float(i));

    std::shared_ptr<XProceduralVolume> proceduralVolume =
        std::make_shared<XProceduralVolume>(
            dimensions, gridOrigin, gridSpacing);

    deviceContexts.push_back(
        std::make_shared<DeviceContext>(device, proceduralVolume));

    // tests immediately after volume is created
    REQUIRE(test_value_range(deviceContexts.back()) == true);
    REQUIRE(test_sampling(deviceContexts.back()) == true);
  }

  // now switch between the devices multiple times and perform tests
  const int numIterations = 3;

  for (int i = 0; i < numIterations; i++) {
    for (const auto &deviceContext : deviceContexts) {
      REQUIRE(test_value_range(deviceContext) == true);
      REQUIRE(test_sampling(deviceContext) == true);
    }
  }

  // now, interact with different devices simultaneously across multiple
  // threads. note that Catch2's macros are not thread-safe, so we'll run
  // assertions at the end
  const int numThreads            = 4 * numDevices;
  const int numThreadedIterations = 16;

  std::vector<std::thread> threads;
  std::atomic<int> threadedTestFailures{0};

  for (int i = 0; i < numThreads; i++) {
    threads.emplace_back([=, &threadedTestFailures]() {
      const auto &deviceContext = deviceContexts[i % deviceContexts.size()];

      bool success = true;

      for (int j = 0; j < numThreadedIterations; j++) {
        success = success && test_value_range(deviceContext);
        success = success && test_sampling(deviceContext);
      }

      threadedTestFailures += !success;
    });
  }

  for (auto &t : threads) {
    t.join();
  }

  REQUIRE(threadedTestFailures == 0);
}
