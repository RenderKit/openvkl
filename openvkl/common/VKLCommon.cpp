// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include <iostream>
#include <sstream>
#include "VKLCommon.h"

#include "../api/Device.h"
#include "logging.h"

namespace openvkl {

  std::string stringFor(VKLDataType type)
  {
    switch (type) {
    case VKL_DEVICE:
      return "device";
    case VKL_VOID_PTR:
      return "void_ptr";
    case VKL_BOOL:
      return "bool";
    case VKL_OBJECT:
      return "object";
    case VKL_DATA:
      return "data";
    case VKL_VOLUME:
      return "volume";
    case VKL_STRING:
      return "string";
    case VKL_CHAR:
      return "char";
    case VKL_VEC2C:
      return "vec2c";
    case VKL_VEC3C:
      return "vec3c";
    case VKL_VEC4C:
      return "vec4c";
    case VKL_UCHAR:
      return "uchar";
    case VKL_VEC2UC:
      return "vec2uc";
    case VKL_VEC3UC:
      return "vec3uc";
    case VKL_VEC4UC:
      return "vec4uc";
    case VKL_SHORT:
      return "short";
    case VKL_VEC2S:
      return "vec2s";
    case VKL_VEC3S:
      return "vec3s";
    case VKL_VEC4S:
      return "vec4s";
    case VKL_USHORT:
      return "ushort";
    case VKL_VEC2US:
      return "vec2us";
    case VKL_VEC3US:
      return "vec3us";
    case VKL_VEC4US:
      return "vec4us";
    case VKL_INT:
      return "int";
    case VKL_VEC2I:
      return "vec2i";
    case VKL_VEC3I:
      return "vec3i";
    case VKL_VEC4I:
      return "vec4i";
    case VKL_UINT:
      return "uint";
    case VKL_VEC2UI:
      return "vec2ui";
    case VKL_VEC3UI:
      return "vec3ui";
    case VKL_VEC4UI:
      return "vec4ui";
    case VKL_LONG:
      return "long";
    case VKL_VEC2L:
      return "vec2l";
    case VKL_VEC3L:
      return "vec3l";
    case VKL_VEC4L:
      return "vec4l";
    case VKL_ULONG:
      return "ulong";
    case VKL_VEC2UL:
      return "vec2ul";
    case VKL_VEC3UL:
      return "vec3ul";
    case VKL_VEC4UL:
      return "vec4ul";
    case VKL_HALF:
      return "half";
    case VKL_VEC2H:
      return "vec2h";
    case VKL_VEC3H:
      return "vec3h";
    case VKL_VEC4H:
      return "vec4h";
    case VKL_FLOAT:
      return "float";
    case VKL_VEC2F:
      return "vec2f";
    case VKL_VEC3F:
      return "vec3f";
    case VKL_VEC4F:
      return "vec4f";
    case VKL_DOUBLE:
      return "double";
    case VKL_VEC2D:
      return "vec2d";
    case VKL_VEC3D:
      return "vec3d";
    case VKL_VEC4D:
      return "vec4d";
    case VKL_BOX1I:
      return "box1i";
    case VKL_BOX2I:
      return "box2i";
    case VKL_BOX3I:
      return "box3i";
    case VKL_BOX4I:
      return "box4i";
    case VKL_BOX1F:
      return "box1f";
    case VKL_BOX2F:
      return "box2f";
    case VKL_BOX3F:
      return "box3f";
    case VKL_BOX4F:
      return "box4f";
    case VKL_LINEAR2F:
      return "linear2f";
    case VKL_LINEAR3F:
      return "linear3f";
    case VKL_AFFINE2F:
      return "affine2f";
    case VKL_AFFINE3F:
      return "affine3f";
    case VKL_UNKNOWN:
      return "unknown";
    default:
      break;
    };

    std::stringstream error;
    error << __FILE__ << ":" << __LINE__ << ": unknown VKLDataType "
          << (int)type;
    throw std::runtime_error(error.str());
  }

#define template_dataType_op(funcName, opFunc)                       \
  size_t funcName(VKLDataType type)                                  \
  {                                                                  \
    switch (type) {                                                  \
    case VKL_DEVICE:                                                 \
    case VKL_VOID_PTR:                                               \
    case VKL_OBJECT:                                                 \
    case VKL_DATA:                                                   \
    case VKL_VOLUME:                                                 \
    case VKL_STRING:                                                 \
      return opFunc(void *);                                         \
    case VKL_BOOL:                                                   \
      return opFunc(bool);                                           \
    case VKL_CHAR:                                                   \
      return opFunc(int8);                                           \
    case VKL_VEC2C:                                                  \
      return opFunc(vec2c);                                          \
    case VKL_VEC3C:                                                  \
      return opFunc(vec3c);                                          \
    case VKL_VEC4C:                                                  \
      return opFunc(vec4c);                                          \
    case VKL_UCHAR:                                                  \
      return opFunc(uint8);                                          \
    case VKL_VEC2UC:                                                 \
      return opFunc(vec2uc);                                         \
    case VKL_VEC3UC:                                                 \
      return opFunc(vec3uc);                                         \
    case VKL_VEC4UC:                                                 \
      return opFunc(vec4uc);                                         \
    case VKL_SHORT:                                                  \
      return opFunc(int16);                                          \
    case VKL_VEC2S:                                                  \
      return opFunc(vec2s);                                          \
    case VKL_VEC3S:                                                  \
      return opFunc(vec3s);                                          \
    case VKL_VEC4S:                                                  \
      return opFunc(vec4s);                                          \
    case VKL_USHORT:                                                 \
      return opFunc(uint16);                                         \
    case VKL_VEC2US:                                                 \
      return opFunc(vec2us);                                         \
    case VKL_VEC3US:                                                 \
      return opFunc(vec3us);                                         \
    case VKL_VEC4US:                                                 \
      return opFunc(vec4us);                                         \
    case VKL_INT:                                                    \
      return opFunc(int32);                                          \
    case VKL_VEC2I:                                                  \
      return opFunc(vec2i);                                          \
    case VKL_VEC3I:                                                  \
      return opFunc(vec3i);                                          \
    case VKL_VEC4I:                                                  \
      return opFunc(vec4i);                                          \
    case VKL_UINT:                                                   \
      return opFunc(uint32);                                         \
    case VKL_VEC2UI:                                                 \
      return opFunc(vec2ui);                                         \
    case VKL_VEC3UI:                                                 \
      return opFunc(vec3ui);                                         \
    case VKL_VEC4UI:                                                 \
      return opFunc(vec4ui);                                         \
    case VKL_LONG:                                                   \
      return opFunc(int64);                                          \
    case VKL_VEC2L:                                                  \
      return opFunc(vec2l);                                          \
    case VKL_VEC3L:                                                  \
      return opFunc(vec3l);                                          \
    case VKL_VEC4L:                                                  \
      return opFunc(vec4l);                                          \
    case VKL_ULONG:                                                  \
      return opFunc(uint64);                                         \
    case VKL_VEC2UL:                                                 \
      return opFunc(vec2ul);                                         \
    case VKL_VEC3UL:                                                 \
      return opFunc(vec3ul);                                         \
    case VKL_VEC4UL:                                                 \
      return opFunc(vec4ul);                                         \
    case VKL_HALF:                                                   \
      return opFunc(uint16);                                         \
    case VKL_VEC2H:                                                  \
      return opFunc(vec2us);                                         \
    case VKL_VEC3H:                                                  \
      return opFunc(vec3us);                                         \
    case VKL_VEC4H:                                                  \
      return opFunc(vec4us);                                         \
    case VKL_FLOAT:                                                  \
      return opFunc(float);                                          \
    case VKL_VEC2F:                                                  \
      return opFunc(vec2f);                                          \
    case VKL_VEC3F:                                                  \
      return opFunc(vec3f);                                          \
    case VKL_VEC4F:                                                  \
      return opFunc(vec4f);                                          \
    case VKL_DOUBLE:                                                 \
      return opFunc(double);                                         \
    case VKL_VEC2D:                                                  \
      return opFunc(vec2d);                                          \
    case VKL_VEC3D:                                                  \
      return opFunc(vec3d);                                          \
    case VKL_VEC4D:                                                  \
      return opFunc(vec4d);                                          \
    case VKL_BOX1I:                                                  \
      return opFunc(box1i);                                          \
    case VKL_BOX2I:                                                  \
      return opFunc(box2i);                                          \
    case VKL_BOX3I:                                                  \
      return opFunc(box3i);                                          \
    case VKL_BOX4I:                                                  \
      return opFunc(box4i);                                          \
    case VKL_BOX1F:                                                  \
      return opFunc(box1f);                                          \
    case VKL_BOX2F:                                                  \
      return opFunc(box2f);                                          \
    case VKL_BOX3F:                                                  \
      return opFunc(box3f);                                          \
    case VKL_BOX4F:                                                  \
      return opFunc(box4f);                                          \
    case VKL_LINEAR2F:                                               \
      return opFunc(linear2f);                                       \
    case VKL_LINEAR3F:                                               \
      return opFunc(linear3f);                                       \
    case VKL_AFFINE2F:                                               \
      return opFunc(affine2f);                                       \
    case VKL_AFFINE3F:                                               \
      return opFunc(affine3f);                                       \
    case VKL_UNKNOWN:                                                \
    default:                                                         \
      break;                                                         \
    };                                                               \
                                                                     \
    std::stringstream error;                                         \
    error << __FILE__ << ":" << __LINE__ << ": unknown VKLDataType " \
          << (int)type;                                              \
    throw std::runtime_error(error.str());                           \
  }

  template_dataType_op(sizeOf, sizeof);
  template_dataType_op(alignOf, alignof);
#undef template_dataType_op

  bool isManagedObject(VKLDataType type)
  {
    return type & VKL_OBJECT;
  }

  void handleError(Device *device, VKLError e, const std::string &message)
  {
    if (device) {
      device->lastErrorCode    = e;
      device->lastErrorMessage = message;
      device->errorCallback(device->errorUserData, e, message.c_str());
    } else {
      LogMessageStream(nullptr, VKL_LOG_ERROR)
          << "INITIALIZATION ERROR: " << message << std::endl;
    }
  }

}  // namespace openvkl
