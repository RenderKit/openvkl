// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AppInit.h"
// rkcommon
#include "rkcommon/common.h"
#include "rkcommon/utility/getEnvVar.h"

static VKLDevice device = nullptr;

#ifdef OPENVKL_TESTING_GPU
static sycl::queue *syclQueuePtr = nullptr;
auto IntelGPUDeviceSelector      = [](const sycl::device &device) {
  using namespace sycl::info;
  const std::string deviceName = device.get_info<device::name>();
  bool match                   = device.is_gpu() &&
               device.get_info<sycl::info::device::vendor_id>() == 0x8086 &&
               device.get_backend() == sycl::backend::ext_oneapi_level_zero;
  return match ? 1 : -1;
};
#endif
void initializeOpenVKL()
{
  if (!device) {
    vklInit();
#ifdef OPENVKL_TESTING_GPU

    // the env var OPENVKL_GPU_DEVICE_DEBUG_USE_CPU is intended for debug
    // purposes only, and forces the Open VKL GPU device to use the CPU
    // instead.
    const bool useCpu =
        rkcommon::utility::getEnvVar<int>("OPENVKL_GPU_DEVICE_DEBUG_USE_CPU")
            .value_or(0);

    syclQueuePtr = new sycl::queue(
        useCpu ? sycl::cpu_selector_v : IntelGPUDeviceSelector,
        {sycl::property::queue::enable_profiling(),  // We need it for profiling
                                                     // kernel execution times
         sycl::property::queue::in_order()});  // By default sycl queues are out
                                               // of order
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
sycl::queue &getSyclQueue()
{
  if (!syclQueuePtr) {
    throw std::runtime_error("syclQueuePtr is not initialized");
  }
  return *syclQueuePtr;
}

static bool useDeviceOnlySharedBuffers = false;

void setUseDeviceOnlySharedBuffers(bool enabled)
{
  useDeviceOnlySharedBuffers = enabled;
}

bool getUseDeviceOnlySharedBuffers()
{
  return useDeviceOnlySharedBuffers;
}

#endif