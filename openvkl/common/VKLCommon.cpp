// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "VKLCommon.h"
#include <iostream>
#include <sstream>
#include "../api/Driver.h"

namespace openvkl {

  VKLError loadLocalModule(const std::string &moduleName)
  {
    std::string libName = "openvkl_module_" + moduleName;
    ospcommon::loadLibrary(libName);

    std::string initSymName = "openvkl_init_module_" + moduleName;
    void *initSym           = ospcommon::getSymbol(initSymName);
    if (!initSym) {
      throw std::runtime_error("#vkl:api: could not find module initializer " +
                               initSymName);
    }

    void (*initMethod)() = (void (*)())initSym;

    if (!initMethod)
      return VKL_INVALID_ARGUMENT;

    try {
      initMethod();
    } catch (...) {
      return VKL_UNKNOWN_ERROR;
    }

    return VKL_NO_ERROR;
  }

  std::string stringForType(VKLDataType type)
  {
    switch (type) {
    case VKL_DRIVER:
      return "driver";
    case VKL_VOID_PTR:
      return "void_ptr";
    case VKL_OBJECT:
      return "object";
    case VKL_DATA:
      return "data";
    case VKL_SAMPLES_MASK:
      return "samples_mask";
    case VKL_VOLUME:
      return "volume";
    case VKL_STRING:
      return "string";
    case VKL_CHAR:
      return "char";
    case VKL_UCHAR:
      return "uchar";
    case VKL_UCHAR2:
      return "uchar2";
    case VKL_UCHAR3:
      return "uchar3";
    case VKL_UCHAR4:
      return "uchar4";
    case VKL_SHORT:
      return "short";
    case VKL_USHORT:
      return "ushort";
    case VKL_INT:
      return "int";
    case VKL_INT2:
      return "int2";
    case VKL_INT3:
      return "int3";
    case VKL_INT4:
      return "int4";
    case VKL_UINT:
      return "uint";
    case VKL_UINT2:
      return "uint2";
    case VKL_UINT3:
      return "uint3";
    case VKL_UINT4:
      return "uint4";
    case VKL_LONG:
      return "long";
    case VKL_LONG2:
      return "long2";
    case VKL_LONG3:
      return "long3";
    case VKL_LONG4:
      return "long4";
    case VKL_ULONG:
      return "ulong";
    case VKL_ULONG2:
      return "ulong2";
    case VKL_ULONG3:
      return "ulong3";
    case VKL_ULONG4:
      return "ulong4";
    case VKL_FLOAT:
      return "float";
    case VKL_FLOAT2:
      return "float2";
    case VKL_FLOAT3:
      return "float3";
    case VKL_FLOAT4:
      return "float4";
    case VKL_FLOAT3A:
      return "float3a";
    case VKL_DOUBLE:
      return "double";
    case VKL_UNKNOWN:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VKLDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  size_t sizeOf(VKLDataType type)
  {
    switch (type) {
    case VKL_DRIVER:
    case VKL_VOID_PTR:
    case VKL_OBJECT:
    case VKL_DATA:
    case VKL_SAMPLES_MASK:
    case VKL_VOLUME:
    case VKL_STRING:
      return sizeof(void *);
    case VKL_CHAR:
      return sizeof(int8);
    case VKL_UCHAR:
      return sizeof(uint8);
    case VKL_UCHAR2:
      return sizeof(vec2uc);
    case VKL_UCHAR3:
      return sizeof(vec3uc);
    case VKL_UCHAR4:
      return sizeof(vec4uc);
    case VKL_SHORT:
      return sizeof(int16);
    case VKL_USHORT:
      return sizeof(uint16);
    case VKL_INT:
      return sizeof(int32);
    case VKL_INT2:
      return sizeof(vec2i);
    case VKL_INT3:
      return sizeof(vec3i);
    case VKL_INT4:
      return sizeof(vec4i);
    case VKL_UINT:
      return sizeof(uint32);
    case VKL_UINT2:
      return sizeof(vec2ui);
    case VKL_UINT3:
      return sizeof(vec3ui);
    case VKL_UINT4:
      return sizeof(vec4ui);
    case VKL_LONG:
      return sizeof(int64);
    case VKL_LONG2:
      return sizeof(vec2l);
    case VKL_LONG3:
      return sizeof(vec3l);
    case VKL_LONG4:
      return sizeof(vec4l);
    case VKL_ULONG:
      return sizeof(uint64);
    case VKL_ULONG2:
      return sizeof(vec2ul);
    case VKL_ULONG3:
      return sizeof(vec3ul);
    case VKL_ULONG4:
      return sizeof(vec4ul);
    case VKL_FLOAT:
      return sizeof(float);
    case VKL_FLOAT2:
      return sizeof(vec2f);
    case VKL_FLOAT3:
      return sizeof(vec3f);
    case VKL_FLOAT4:
      return sizeof(vec4f);
    case VKL_FLOAT3A:
      return sizeof(vec3fa);
    case VKL_DOUBLE:
      return sizeof(double);
    case VKL_UNKNOWN:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VKLDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  void handleError(VKLError e, const std::string &message)
  {
    if (api::driverIsSet()) {
      auto &driver = api::currentDriver();

      driver.lastErrorCode    = e;
      driver.lastErrorMessage = message;

      driver.errorFunction(e, message.c_str());
    } else {
      std::cerr << "#openvkl: INITIALIZATION ERROR --> " << message << std::endl;
    }
  }

}  // namespace openvkl
