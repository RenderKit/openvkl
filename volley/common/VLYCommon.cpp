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

#include "VLYCommon.h"
#include <iostream>
#include <sstream>
#include "api/Driver.h"

namespace volley {

  VLYError loadLocalModule(const std::string &moduleName)
  {
    std::string libName = "volley_module_" + moduleName;
    ospcommon::loadLibrary(libName);

    std::string initSymName = "volley_init_module_" + moduleName;
    void *initSym           = ospcommon::getSymbol(initSymName);
    if (!initSym) {
      throw std::runtime_error("#vly:api: could not find module initializer " +
                               initSymName);
    }

    void (*initMethod)() = (void (*)())initSym;

    if (!initMethod)
      return VLY_INVALID_ARGUMENT;

    try {
      initMethod();
    } catch (...) {
      return VLY_UNKNOWN_ERROR;
    }

    return VLY_NO_ERROR;
  }

  std::string stringForType(VLYDataType type)
  {
    switch (type) {
    case VLY_DRIVER:
      return "driver";
    case VLY_VOID_PTR:
      return "void_ptr";
    case VLY_OBJECT:
      return "object";
    case VLY_DATA:
      return "data";
    case VLY_SAMPLES_MASK:
      return "samples_mask";
    case VLY_VOLUME:
      return "volume";
    case VLY_STRING:
      return "string";
    case VLY_CHAR:
      return "char";
    case VLY_UCHAR:
      return "uchar";
    case VLY_UCHAR2:
      return "uchar2";
    case VLY_UCHAR3:
      return "uchar3";
    case VLY_UCHAR4:
      return "uchar4";
    case VLY_SHORT:
      return "short";
    case VLY_USHORT:
      return "ushort";
    case VLY_INT:
      return "int";
    case VLY_INT2:
      return "int2";
    case VLY_INT3:
      return "int3";
    case VLY_INT4:
      return "int4";
    case VLY_UINT:
      return "uint";
    case VLY_UINT2:
      return "uint2";
    case VLY_UINT3:
      return "uint3";
    case VLY_UINT4:
      return "uint4";
    case VLY_LONG:
      return "long";
    case VLY_LONG2:
      return "long2";
    case VLY_LONG3:
      return "long3";
    case VLY_LONG4:
      return "long4";
    case VLY_ULONG:
      return "ulong";
    case VLY_ULONG2:
      return "ulong2";
    case VLY_ULONG3:
      return "ulong3";
    case VLY_ULONG4:
      return "ulong4";
    case VLY_FLOAT:
      return "float";
    case VLY_FLOAT2:
      return "float2";
    case VLY_FLOAT3:
      return "float3";
    case VLY_FLOAT4:
      return "float4";
    case VLY_FLOAT3A:
      return "float3a";
    case VLY_DOUBLE:
      return "double";
    case VLY_UNKNOWN:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VLYDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  size_t sizeOf(VLYDataType type)
  {
    switch (type) {
    case VLY_DRIVER:
    case VLY_VOID_PTR:
    case VLY_OBJECT:
    case VLY_DATA:
    case VLY_SAMPLES_MASK:
    case VLY_VOLUME:
    case VLY_STRING:
      return sizeof(void *);
    case VLY_CHAR:
      return sizeof(int8);
    case VLY_UCHAR:
      return sizeof(uint8);
    case VLY_UCHAR2:
      return sizeof(vec2uc);
    case VLY_UCHAR3:
      return sizeof(vec3uc);
    case VLY_UCHAR4:
      return sizeof(vec4uc);
    case VLY_SHORT:
      return sizeof(int16);
    case VLY_USHORT:
      return sizeof(uint16);
    case VLY_INT:
      return sizeof(int32);
    case VLY_INT2:
      return sizeof(vec2i);
    case VLY_INT3:
      return sizeof(vec3i);
    case VLY_INT4:
      return sizeof(vec4i);
    case VLY_UINT:
      return sizeof(uint32);
    case VLY_UINT2:
      return sizeof(vec2ui);
    case VLY_UINT3:
      return sizeof(vec3ui);
    case VLY_UINT4:
      return sizeof(vec4ui);
    case VLY_LONG:
      return sizeof(int64);
    case VLY_LONG2:
      return sizeof(vec2l);
    case VLY_LONG3:
      return sizeof(vec3l);
    case VLY_LONG4:
      return sizeof(vec4l);
    case VLY_ULONG:
      return sizeof(uint64);
    case VLY_ULONG2:
      return sizeof(vec2ul);
    case VLY_ULONG3:
      return sizeof(vec3ul);
    case VLY_ULONG4:
      return sizeof(vec4ul);
    case VLY_FLOAT:
      return sizeof(float);
    case VLY_FLOAT2:
      return sizeof(vec2f);
    case VLY_FLOAT3:
      return sizeof(vec3f);
    case VLY_FLOAT4:
      return sizeof(vec4f);
    case VLY_FLOAT3A:
      return sizeof(vec3fa);
    case VLY_DOUBLE:
      return sizeof(double);
    case VLY_UNKNOWN:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VLYDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  void handleError(VLYError e, const std::string &message)
  {
    if (api::driverIsSet()) {
      auto &driver = api::currentDriver();

      driver.lastErrorCode    = e;
      driver.lastErrorMessage = message;

      driver.errorFunction(e, message.c_str());
    } else {
      std::cerr << "#volley: INITIALIZATION ERROR --> " << message << std::endl;
    }
  }

}  // namespace volley
