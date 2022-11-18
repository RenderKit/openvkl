// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>
#include "openvkl/VKLDataType.h"
#include "openvkl/VKLError.h"

#ifdef _WIN32
#ifdef openvkl_EXPORTS
#define OPENVKL_INTERFACE __declspec(dllexport)
#else
#define OPENVKL_INTERFACE __declspec(dllimport)
#endif
#define OPENVKL_DLLEXPORT __declspec(dllexport)
#else
#define OPENVKL_INTERFACE
#define OPENVKL_DLLEXPORT __attribute__((visibility("default")))
#endif

#define OPENVKL_CORE_INTERFACE OPENVKL_INTERFACE

#define VKL_REGISTER_DEVICEOBJECT(                                            \
    Object, object_name, InternalClass, external_name)                        \
  extern "C" OPENVKL_DLLEXPORT Object                                         \
      *openvkl_create_##object_name##__##external_name()                      \
  {                                                                           \
    auto *instance = new InternalClass;                                       \
    if (instance->getParam<std::string>("externalNameFromAPI", "").empty()) { \
      instance->setParam<std::string>("externalNameFromeAPI",                 \
                                      TOSTRING(external_name));               \
    }                                                                         \
    return instance;                                                          \
  }                                                                           \
  // additional declaration to avoid "extra ;" -Wpedantic warnings             \
  Object *openvkl_create_##object_name##__##external_name()

#define VKL_REGISTER_OBJECT(Object, object_name, InternalClass, external_name) \
  extern "C" OPENVKL_DLLEXPORT Object                                          \
      *openvkl_create_##object_name##__##external_name(                        \
          openvkl::api::Device *device)                                        \
  {                                                                            \
    auto *instance = new InternalClass(device);                                \
    if (instance->getParam<std::string>("externalNameFromAPI", "").empty()) {  \
      instance->setParam<std::string>("externalNameFromeAPI",                  \
                                      TOSTRING(external_name));                \
    }                                                                          \
    return instance;                                                           \
  }                                                                            \
  // additional declaration to avoid "extra ;" -Wpedantic warnings             \
  Object *openvkl_create_##object_name##__##external_name()

namespace openvkl {

  namespace api {
    struct Device;
  }

  using openvkl::api::Device;

  using int64  = std::int64_t;
  using uint64 = std::uint64_t;

  using int32  = std::int32_t;
  using uint32 = std::uint32_t;

  using int16  = std::int16_t;
  using uint16 = std::uint16_t;

  using int8  = std::int8_t;
  using uint8 = std::uint8_t;

  OPENVKL_CORE_INTERFACE std::string stringFor(VKLDataType type);

  OPENVKL_CORE_INTERFACE size_t sizeOf(VKLDataType type);

  OPENVKL_CORE_INTERFACE bool isManagedObject(VKLDataType type);

  OPENVKL_CORE_INTERFACE void handleError(Device *device,
                                          VKLError e,
                                          const std::string &message);

}  // namespace openvkl
