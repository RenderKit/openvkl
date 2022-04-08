// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "api/CPUDevice.h"
#include "openvkl/common/ManagedObject.h"

// For Windows in particular, we need registered object creation functions
// available in the top level module library, as exports from dependencies will
// not be visible.

#define VKL_WRAP_DEVICE_REGISTRATION(                                    \
    module_name, internal_name, external_name)                           \
  extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_##module_name(); \
                                                                         \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::api::Device                    \
      *openvkl_create_device__##internal_name();                         \
                                                                         \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::api::Device                    \
      *openvkl_create_device__##external_name()                          \
  {                                                                      \
    return openvkl_create_device__##internal_name();                     \
  }

#define VKL_WRAP_VOLUME_REGISTRATION(internal_name, external_name) \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::ManagedObject            \
      *openvkl_create_volume__##internal_name();                   \
                                                                   \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::ManagedObject            \
      *openvkl_create_volume__##external_name()                    \
  {                                                                \
    return openvkl_create_volume__##internal_name();               \
  }

#if VKL_TARGET_WIDTH_ENABLED_4
VKL_WRAP_DEVICE_REGISTRATION(cpu_device_4, internal_cpu_4, cpu_4)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_4, amr_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_4, structuredRegular_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_4,
                             structuredSpherical_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_4, unstructured_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_vdb_4, vdb_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_particle_4, particle_4)

// support deprecated snake case names (a warning will be triggered if these are
// used)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_4, structured_regular_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_4,
                             structured_spherical_4)
#endif

#if VKL_TARGET_WIDTH_ENABLED_8
VKL_WRAP_DEVICE_REGISTRATION(cpu_device_8, internal_cpu_8, cpu_8)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_8, amr_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_8, structuredRegular_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_8,
                             structuredSpherical_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_8, unstructured_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_vdb_8, vdb_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_particle_8, particle_8)

// support deprecated snake case names (a warning will be triggered if these are
// used)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_8, structured_regular_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_8,
                             structured_spherical_8)
#endif

#if VKL_TARGET_WIDTH_ENABLED_16
VKL_WRAP_DEVICE_REGISTRATION(cpu_device_16, internal_cpu_16, cpu_16)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_16, amr_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_16,
                             structuredRegular_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_16,
                             structuredSpherical_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_16, unstructured_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_vdb_16, vdb_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_particle_16, particle_16)

// support deprecated snake case names (a warning will be triggered if these are
// used)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredRegular_16,
                             structured_regular_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_structuredSpherical_16,
                             structured_spherical_16)
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
