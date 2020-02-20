// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
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

#include <vector>
#include "openvkl/openvkl.h"
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/math/vec.h"

using namespace ospcommon::containers;
using namespace ospcommon::math;

///////////////////////////////////////////////////////////////////////////////
// AOS to SOA conversion functions ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// these functions take an input vector in AOS layout and returns a flat output
// vector in SOA layout for the given calling width. the input vector may have
// length < callingWidth.

inline AlignedVector<float> AOStoSOA_vec3f(const std::vector<vec3f> &input,
                                           int callingWidth)
{
  if (input.size() > callingWidth) {
    throw std::runtime_error(
        "cannot perform AOS to SOA conversion since input width > "
        "callingWidth");
  }

  AlignedVector<float> output;

  int width = input.size();

  for (int i = 0; i < callingWidth; i++)
    output.push_back(i < width ? input[i].x : 0.f);

  for (int i = 0; i < callingWidth; i++)
    output.push_back(i < width ? input[i].y : 0.f);

  for (int i = 0; i < callingWidth; i++)
    output.push_back(i < width ? input[i].z : 0.f);

  if (output.size() != callingWidth * 3)
    throw std::runtime_error("improper AOS to SOA conversion");

  return output;
}

inline AlignedVector<float> AOStoSOA_range1f(
    const std::vector<vkl_range1f> &input, int callingWidth)
{
  if (input.size() > callingWidth) {
    throw std::runtime_error(
        "cannot perform AOS to SOA conversion since input width > "
        "callingWidth");
  }

  AlignedVector<float> output;

  int width = input.size();

  for (int i = 0; i < callingWidth; i++)
    output.push_back(i < width ? input[i].lower : 0.f);

  for (int i = 0; i < callingWidth; i++)
    output.push_back(i < width ? input[i].upper : 0.f);

  if (output.size() != callingWidth * 2)
    throw std::runtime_error("improper AOS to SOA conversion");

  return output;
}

///////////////////////////////////////////////////////////////////////////////
// SOA to AOS conversion functions ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

inline std::vector<vec3f> SOAtoAOS_vvec3f(const vkl_vvec3f4 &input)
{
  std::vector<vec3f> output;

  for (int i = 0; i < 4; i++) {
    output.push_back(vec3f(input.x[i], input.y[i], input.z[i]));
  }

  return output;
}

inline std::vector<vec3f> SOAtoAOS_vvec3f(const vkl_vvec3f8 &input)
{
  std::vector<vec3f> output;

  for (int i = 0; i < 8; i++) {
    output.push_back(vec3f(input.x[i], input.y[i], input.z[i]));
  }

  return output;
}

inline std::vector<vec3f> SOAtoAOS_vvec3f(const vkl_vvec3f16 &input)
{
  std::vector<vec3f> output;

  for (int i = 0; i < 16; i++) {
    output.push_back(vec3f(input.x[i], input.y[i], input.z[i]));
  }

  return output;
}
