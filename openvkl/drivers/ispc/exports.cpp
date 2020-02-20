// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "api/ISPCDriver.h"
#include "openvkl/common/ManagedObject.h"

// For Windows in particular, we need registered object creation functions
// available in the top level module library, as exports from dependencies will
// not be visible.

#define VKL_WRAP_DRIVER_REGISTRATION(                                    \
    module_name, internal_name, external_name)                           \
  extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_##module_name(); \
                                                                         \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::api::Driver                    \
      *openvkl_create_driver__##internal_name();                         \
                                                                         \
  extern "C" OPENVKL_DLLEXPORT ::openvkl::api::Driver                    \
      *openvkl_create_driver__##external_name()                          \
  {                                                                      \
    return openvkl_create_driver__##internal_name();                     \
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
VKL_WRAP_DRIVER_REGISTRATION(ispc_driver_4, internal_ispc_4, ispc_4)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_4, amr_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_regular_4, structured_regular_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_spherical_4, structured_spherical_4)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_4, unstructured_4)
#endif

#if VKL_TARGET_WIDTH_ENABLED_8
VKL_WRAP_DRIVER_REGISTRATION(ispc_driver_8, internal_ispc_8, ispc_8)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_8, amr_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_regular_8, structured_regular_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_spherical_8, structured_spherical_8)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_8, unstructured_8)
#endif

#if VKL_TARGET_WIDTH_ENABLED_16
VKL_WRAP_DRIVER_REGISTRATION(ispc_driver_16, internal_ispc_16, ispc_16)

VKL_WRAP_VOLUME_REGISTRATION(internal_amr_16, amr_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_regular_16, structured_regular_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_structured_spherical_16, structured_spherical_16)
VKL_WRAP_VOLUME_REGISTRATION(internal_unstructured_16, unstructured_16)
#endif

// calls init functions in [4, 8, 16] drivers to ensure proper linkage
extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_ispc_driver()
{
#if VKL_TARGET_WIDTH_ENABLED_4
  openvkl_init_module_ispc_driver_4();
#endif

#if VKL_TARGET_WIDTH_ENABLED_8
  openvkl_init_module_ispc_driver_8();
#endif

#if VKL_TARGET_WIDTH_ENABLED_16
  openvkl_init_module_ispc_driver_16();
#endif
}
