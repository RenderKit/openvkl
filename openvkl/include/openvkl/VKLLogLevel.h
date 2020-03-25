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

// Log levels which can be set on a driver via "logLevel" parameter or
// "OPENVKL_LOG_LEVEL" environment variable
typedef enum
#if __cplusplus >= 201103L
    : uint32_t
#endif
{
  VKL_LOG_DEBUG   = 1,
  VKL_LOG_INFO    = 2,
  VKL_LOG_WARNING = 3,
  VKL_LOG_ERROR   = 4,
  VKL_LOG_NONE    = 5,
} VKLLogLevel;
