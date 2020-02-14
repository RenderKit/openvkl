// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#ifndef TARGET_WIDTH
#error "export_util.h included without TARGET_WIDTH defined"
#endif

#define CONCAT2(A, B) A##B
#define CONCAT1(A, B) CONCAT2(A, B)

#define CONCAT_ARGS_V(A, ...) A(__VA_ARGS__)

#define EXPORT_UNIQUE(name, ...) \
  CONCAT_ARGS_V(CONCAT1(name, TARGET_WIDTH), __VA_ARGS__)

#define CALL_ISPC(function, ...) \
  CONCAT_ARGS_V(CONCAT1(ispc::function, TARGET_WIDTH), __VA_ARGS__)
