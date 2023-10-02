// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../common/ManagedObject.h"
#include "../common/VKLCommon.h"
#include "api/CPUDevice.h"

// For Windows in particular, we need registered object creation functions
// available in the top level module library, as exports from dependencies will
// not be visible.

#define VKL_WRAP_MODULE_REGISTRATION(module_name) \
  extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_##module_name();

#if VKL_TARGET_WIDTH_ENABLED_4
VKL_WRAP_MODULE_REGISTRATION(cpu_device_4)
#endif

#if VKL_TARGET_WIDTH_ENABLED_8
VKL_WRAP_MODULE_REGISTRATION(cpu_device_8)
#endif

#if VKL_TARGET_WIDTH_ENABLED_16
VKL_WRAP_MODULE_REGISTRATION(cpu_device_16)
#endif

// calls init functions in [4, 8, 16] devices to ensure proper linkage
extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_cpu_device()
{
#if VKL_TARGET_WIDTH_ENABLED_4
  openvkl_init_module_cpu_device_4();
#endif

#if VKL_TARGET_WIDTH_ENABLED_8
  openvkl_init_module_cpu_device_8();
#endif

#if VKL_TARGET_WIDTH_ENABLED_16
  openvkl_init_module_cpu_device_16();
#endif
}
