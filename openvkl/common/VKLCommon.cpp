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

#include "VKLCommon.h"
#include <iostream>
#include <sstream>
#include "../api/Driver.h"
#include "logging.h"
#include "ospcommon/math/AffineSpace.h"

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

  std::string stringForHandleType(VKLDataType type)
  {
    switch (type) {
    case VKL_DRIVER:
      return "driver";
    case VKL_OBJECT:
      return "object";
    case VKL_DATA:
      return "data";
    case VKL_VALUE_SELECTOR:
      return "value_selector";
    case VKL_VOLUME:
      return "volume";
    default:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__
          << ": unknown VKLDataType or non-handle type used " << (int)type;
    throw std::runtime_error(error.str());
  }

  size_t sizeOf(VKLDataType type)
  {
    switch (type) {
    case VKL_DRIVER:
    case VKL_VOID_PTR:
    case VKL_OBJECT:
    case VKL_DATA:
    case VKL_VALUE_SELECTOR:
    case VKL_VOLUME:
    case VKL_STRING:
      return sizeof(void *);
    case VKL_BOOL:
      return sizeof(bool);
    case VKL_CHAR:
      return sizeof(int8);
    case VKL_UCHAR:
      return sizeof(uint8);
    case VKL_VEC2UC:
      return sizeof(vec2uc);
    case VKL_VEC3UC:
      return sizeof(vec3uc);
    case VKL_VEC4UC:
      return sizeof(vec4uc);
    case VKL_SHORT:
      return sizeof(int16);
    case VKL_USHORT:
      return sizeof(uint16);
    case VKL_INT:
      return sizeof(int32);
    case VKL_VEC2I:
      return sizeof(vec2i);
    case VKL_VEC3I:
      return sizeof(vec3i);
    case VKL_VEC4I:
      return sizeof(vec4i);
    case VKL_UINT:
      return sizeof(uint32);
    case VKL_VEC2UI:
      return sizeof(vec2ui);
    case VKL_VEC3UI:
      return sizeof(vec3ui);
    case VKL_VEC4UI:
      return sizeof(vec4ui);
    case VKL_LONG:
      return sizeof(int64);
    case VKL_VEC2L:
      return sizeof(vec2l);
    case VKL_VEC3L:
      return sizeof(vec3l);
    case VKL_VEC4L:
      return sizeof(vec4l);
    case VKL_ULONG:
      return sizeof(uint64);
    case VKL_VEC2UL:
      return sizeof(vec2ul);
    case VKL_VEC3UL:
      return sizeof(vec3ul);
    case VKL_VEC4UL:
      return sizeof(vec4ul);
    case VKL_FLOAT:
      return sizeof(float);
    case VKL_VEC2F:
      return sizeof(vec2f);
    case VKL_VEC3F:
      return sizeof(vec3f);
    case VKL_VEC4F:
      return sizeof(vec4f);
    case VKL_DOUBLE:
      return sizeof(double);
    case VKL_BOX1I:
      return sizeof(box1i);
    case VKL_BOX2I:
      return sizeof(box2i);
    case VKL_BOX3I:
      return sizeof(box3i);
    case VKL_BOX4I:
      return sizeof(box4i);
    case VKL_BOX1F:
      return sizeof(box1f);
    case VKL_BOX2F:
      return sizeof(box2f);
    case VKL_BOX3F:
      return sizeof(box3f);
    case VKL_BOX4F:
      return sizeof(box4f);
    case VKL_LINEAR2F:
      return sizeof(linear2f);
    case VKL_LINEAR3F:
      return sizeof(linear3f);
    case VKL_AFFINE2F:
      return sizeof(affine2f);
    case VKL_AFFINE3F:
      return sizeof(affine3f);
    case VKL_UNKNOWN:
    default:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VKLDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

  bool isManagedObject(VKLDataType type)
  {
    return type == VKL_OBJECT || type == VKL_DATA ||
           type == VKL_VALUE_SELECTOR || type == VKL_VOLUME;
  }

  void handleError(VKLError e, const std::string &message)
  {
    if (api::driverIsSet()) {
      auto &driver = api::currentDriver();

      driver.lastErrorCode    = e;
      driver.lastErrorMessage = message;

      driver.errorFunction(e, message.c_str());
    } else {
      LogMessageStream(VKL_LOG_ERROR)
          << "INITIALIZATION ERROR: " << message << std::endl;
    }
  }

}  // namespace openvkl
