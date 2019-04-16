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
#include "VLYDataType.h"

// flags that can be passed to vlyNewData(), which can be OR'ed together
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VLY_DATA_DEFAULT       = 0,
  VLY_DATA_SHARED_BUFFER = (1 << 0),
} VLYDataCreationFlags;

#ifdef __cplusplus
extern "C" {
#endif

#define VLY_DEFAULT_VAL(a) a

struct Data : public ManagedObject
{
};

typedef Data *VLYData;

VLYData vlyNewData(size_t numItems,
                   VLYDataType dataType,
                   const void *source,
                   VLYDataCreationFlags dataCreationFlags
                       VLY_DEFAULT_VAL(= VLY_DATA_DEFAULT));

#ifdef __cplusplus
}  // extern "C"
#endif
