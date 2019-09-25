// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#pragma once

#include "openvkl/VKLDataType.h"
#include "openvkl/VKLError.h"
#include <string>

#ifdef _WIN32
#  ifdef openvkl_EXPORTS
#    define OPENVKL_INTERFACE __declspec(dllexport)
#  else
#    define OPENVKL_INTERFACE __declspec(dllimport)
#  endif
#  define OPENVKL_DLLEXPORT __declspec(dllexport)
#else
#  define OPENVKL_INTERFACE
#  define OPENVKL_DLLEXPORT
#endif

#define OPENVKL_CORE_INTERFACE OPENVKL_INTERFACE

#define VKL_REGISTER_OBJECT(Object, object_name, InternalClass, external_name) \
  extern "C" OPENVKL_DLLEXPORT                                                  \
      Object *openvkl_create_##object_name##__##external_name()                 \
  {                                                                            \
    auto *instance = new InternalClass;                                        \
    if (instance->getParam<std::string>("externalNameFromAPI", "").empty()) {  \
      instance->setParam<std::string>("externalNameFromeAPI",                  \
                                      TOSTRING(external_name));                \
    }                                                                          \
    return instance;                                                           \
  }                                                                            \
  // additional declaration to avoid "extra ;" -Wpedantic warnings             \
  Object *openvkl_create_##object_name##__##external_name()

namespace openvkl {

  using int64  = std::int64_t;
  using uint64 = std::uint64_t;

  using int32  = std::int32_t;
  using uint32 = std::uint32_t;

  using int16  = std::int16_t;
  using uint16 = std::uint16_t;

  using int8  = std::int8_t;
  using uint8 = std::uint8_t;

  OPENVKL_CORE_INTERFACE VKLError loadLocalModule(const std::string &moduleName);

  OPENVKL_CORE_INTERFACE std::string stringForType(VKLDataType type);

  OPENVKL_CORE_INTERFACE size_t sizeOf(VKLDataType type);

  OPENVKL_CORE_INTERFACE bool isManagedObject(VKLDataType type);

  OPENVKL_CORE_INTERFACE void handleError(VKLError e, const std::string &message);

}  // namespace openvkl
