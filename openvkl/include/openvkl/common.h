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

#ifdef __cplusplus
#define VKL_DEFAULT_VAL(a) a
#else
#define VKL_DEFAULT_VAL(a)
#endif

#ifdef _WIN32
#  define VKL_ALIGN(...) __declspec(align(__VA_ARGS__))
#else
#  define VKL_ALIGN(...) __attribute__((aligned(__VA_ARGS__)))
#endif

#ifdef _WIN32
#  ifdef openvkl_EXPORTS
#    define OPENVKL_INTERFACE __declspec(dllexport)
#  else
#    define OPENVKL_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OPENVKL_INTERFACE
#endif

#ifdef __cplusplus
struct ManagedObject
{
};
#else
typedef void ManagedObject;
#endif

typedef ManagedObject *VKLObject;

typedef struct
{
  int x, y, z;
} vkl_vec3i;

typedef struct
{
  float x, y, z;
} vkl_vec3f;

typedef struct
{
  float lower, upper;
} vkl_range1f;

typedef struct
{
  vkl_vec3i lower, upper;
} vkl_box3i;

typedef struct
{
  vkl_vec3f lower, upper;
} vkl_box3f;

#define __define_wide_types(WIDTH, ALIGNMENT) \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    float x[WIDTH];                           \
    float y[WIDTH];                           \
    float z[WIDTH];                           \
  } vkl_vvec3f##WIDTH;                        \
                                              \
  typedef struct VKL_ALIGN(ALIGNMENT)         \
  {                                           \
    float lower[WIDTH], upper[WIDTH];         \
  } vkl_vrange1f##WIDTH;

__define_wide_types(4, 16);
__define_wide_types(8, 32);
__define_wide_types(16, 64);

#undef __define_wide_types
