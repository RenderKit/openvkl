// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "Traits.h"

namespace openvkl {
  VKLTYPEFOR_DEFINITION(VKLDevice);
  VKLTYPEFOR_DEFINITION(openvkl::ManagedObject *);
  VKLTYPEFOR_DEFINITION(openvkl::Data *);
  VKLTYPEFOR_DEFINITION(void *);
  VKLTYPEFOR_DEFINITION(bool);
  VKLTYPEFOR_DEFINITION(char *);
  VKLTYPEFOR_DEFINITION(const char *);
  VKLTYPEFOR_DEFINITION(const char[]);
  VKLTYPEFOR_DEFINITION(char);
  VKLTYPEFOR_DEFINITION(unsigned char);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2uc);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3uc);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4uc);
  VKLTYPEFOR_DEFINITION(short);
  VKLTYPEFOR_DEFINITION(unsigned short);
  VKLTYPEFOR_DEFINITION(int32_t);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4i);
  VKLTYPEFOR_DEFINITION(uint32_t);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2ui);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3ui);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4ui);
  VKLTYPEFOR_DEFINITION(int64_t);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2l);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3l);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4l);
  VKLTYPEFOR_DEFINITION(uint64_t);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2ul);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3ul);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4ul);
  VKLTYPEFOR_DEFINITION(float);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec2f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec3f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::vec4f);
  VKLTYPEFOR_DEFINITION(double);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box1i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box2i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box3i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box4i);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box1f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box2f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box3f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::box4f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::linear2f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::linear3f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::affine2f);
  VKLTYPEFOR_DEFINITION(rkcommon::math::affine3f);
}  // namespace openvkl
