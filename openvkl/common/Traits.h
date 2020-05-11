// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "openvkl/openvkl.h"
#include "ospcommon/math/AffineSpace.h"
#include "ospcommon/math/box.h"
#include "ospcommon/math/vec.h"

namespace openvkl {

  // Infer (compile time) VKL_DATA_TYPE from input type
  template <typename T>
  struct VKLTypeFor
  {
    static constexpr VKLDataType value = VKL_UNKNOWN;
  };

#define VKLTYPEFOR_SPECIALIZATION(type, vkl_type)  \
  template <>                                      \
  struct VKLTypeFor<type>                          \
  {                                                \
    static constexpr VKLDataType value = vkl_type; \
  };

  VKLTYPEFOR_SPECIALIZATION(VKLDriver, VKL_DRIVER);
  VKLTYPEFOR_SPECIALIZATION(void *, VKL_VOID_PTR);
  VKLTYPEFOR_SPECIALIZATION(bool, VKL_BOOL);
  VKLTYPEFOR_SPECIALIZATION(VKLObject, VKL_OBJECT);
  VKLTYPEFOR_SPECIALIZATION(VKLData, VKL_DATA);
  VKLTYPEFOR_SPECIALIZATION(VKLValueSelector, VKL_VALUE_SELECTOR);
  VKLTYPEFOR_SPECIALIZATION(VKLVolume, VKL_VOLUME);
  VKLTYPEFOR_SPECIALIZATION(char *, VKL_STRING);
  VKLTYPEFOR_SPECIALIZATION(const char *, VKL_STRING);
  VKLTYPEFOR_SPECIALIZATION(const char[], VKL_STRING);
  VKLTYPEFOR_SPECIALIZATION(char, VKL_CHAR);
  VKLTYPEFOR_SPECIALIZATION(unsigned char, VKL_UCHAR);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2uc, VKL_VEC2UC);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3uc, VKL_VEC3UC);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4uc, VKL_VEC4UC);
  VKLTYPEFOR_SPECIALIZATION(short, VKL_SHORT);
  VKLTYPEFOR_SPECIALIZATION(unsigned short, VKL_USHORT);
  VKLTYPEFOR_SPECIALIZATION(int32_t, VKL_INT);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2i, VKL_VEC2I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3i, VKL_VEC3I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4i, VKL_VEC4I);
  VKLTYPEFOR_SPECIALIZATION(uint32_t, VKL_UINT);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2ui, VKL_VEC2UI);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3ui, VKL_VEC3UI);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4ui, VKL_VEC4UI);
  VKLTYPEFOR_SPECIALIZATION(int64_t, VKL_LONG);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2l, VKL_VEC2L);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3l, VKL_VEC3L);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4l, VKL_VEC4L);
  VKLTYPEFOR_SPECIALIZATION(uint64_t, VKL_ULONG);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2ul, VKL_VEC2UL);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3ul, VKL_VEC3UL);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4ul, VKL_VEC4UL);
  VKLTYPEFOR_SPECIALIZATION(float, VKL_FLOAT);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec2f, VKL_VEC2F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec3f, VKL_VEC3F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::vec4f, VKL_VEC4F);
  VKLTYPEFOR_SPECIALIZATION(double, VKL_DOUBLE);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box1i, VKL_BOX1I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box2i, VKL_BOX2I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box3i, VKL_BOX3I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box4i, VKL_BOX4I);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box1f, VKL_BOX1F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box2f, VKL_BOX2F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box3f, VKL_BOX3F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::box4f, VKL_BOX4F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::linear2f, VKL_LINEAR2F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::linear3f, VKL_LINEAR3F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::affine2f, VKL_AFFINE2F);
  VKLTYPEFOR_SPECIALIZATION(ospcommon::math::affine3f, VKL_AFFINE3F);

}  // namespace openvkl
