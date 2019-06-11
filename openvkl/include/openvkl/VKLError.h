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

// Error codes returned by various API and callback functions
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_NO_ERROR         = 0,  // No error has been recorded
  VKL_UNKNOWN_ERROR    = 1,  // An unknown error has occured
  VKL_INVALID_ARGUMENT = 2,  // An invalid argument is specified
  VKL_INVALID_OPERATION =
      3,  // The operation is not allowed for the specified object
  VKL_OUT_OF_MEMORY =
      4,  // There is not enough memory left to execute the command
  VKL_UNSUPPORTED_CPU =
      5,  // The CPU is not supported as it does not support SSE4.1
} VKLError;
