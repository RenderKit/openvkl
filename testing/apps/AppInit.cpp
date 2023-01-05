// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
// rkcommon
#include "rkcommon/common.h"

static VKLDevice device = nullptr;

#ifdef OPENVKL_TESTING_GPU
static sycl::queue *syclQueuePtr = nullptr;
auto IntelGPUDeviceSelector      = [](const sycl::device &device) {
  using namespace sycl::info;
  const std::string deviceName = device.get_info<device::name>();
  bool match                   = device.is_gpu() &&
               (deviceName.find("Intel(R) Graphics") != std::string::npos) &&
               device.get_backend() == sycl::backend::ext_oneapi_level_zero;
  return match ? 1 : -1;
};
#endif
void initializeOpenVKL()
{
  if (!device) {
    vklInit();
#ifdef OPENVKL_TESTING_GPU
    syclQueuePtr = new sycl::queue(
        IntelGPUDeviceSelector,
        {sycl::property::queue::enable_profiling(), // We need it for profiling kernel execution times
         sycl::property::queue::in_order()}); // By default sycl queues are out of order
    sycl::context syclContext = syclQueuePtr->get_context();
    std::cout << std::endl
              << "Target SYCL device: "
              << syclQueuePtr->get_device().get_info<sycl::info::device::name>()
              << std::endl
              << std::endl;
    device = vklNewDevice("gpu_4");
    vklDeviceSetVoidPtr(
        device, "syclContext", static_cast<void *>(&syclContext));
#endif
#ifdef OPENVKL_TESTING_CPU
    device = vklNewDevice("cpu");
#endif
    vklCommitDevice(device);
  }
}

void shutdownOpenVKL()
{
  if (device) {
    vklReleaseDevice(device);
    device = nullptr;

#ifdef OPENVKL_TESTING_GPU
    delete syclQueuePtr;
    syclQueuePtr = nullptr;
#endif
  }
}

VKLDevice getOpenVKLDevice()
{
  return device;
}
#ifdef OPENVKL_TESTING_GPU
sycl::queue getSyclQueue()
{
  if (!syclQueuePtr) {
    throw std::runtime_error("syclQueuePtr is not initialized");
  }
  return *syclQueuePtr;
}
#endif