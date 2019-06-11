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

#pragma once

#include <cstddef>
#include "VKLDataType.h"

// flags that can be passed to vklNewData(), which can be OR'ed together
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_DATA_DEFAULT       = 0,
  VKL_DATA_SHARED_BUFFER = (1 << 0),
} VKLDataCreationFlags;

#ifdef __cplusplus
extern "C" {
#endif

#define VKL_DEFAULT_VAL(a) a

struct Data : public ManagedObject
{
};

typedef Data *VKLData;

VKLData vklNewData(size_t numItems,
                   VKLDataType dataType,
                   const void *source,
                   VKLDataCreationFlags dataCreationFlags
                       VKL_DEFAULT_VAL(= VKL_DATA_DEFAULT));

#ifdef __cplusplus
}  // extern "C"
#endif
