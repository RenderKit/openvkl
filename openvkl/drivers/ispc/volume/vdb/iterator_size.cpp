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

#include <iostream>
#include "VdbIterator.h"
#include "VdbIterator_ispc.h"
#include "common/export_util.h"

int main()
{
  std::cout << "sizeof(ispc::VdbIterator<" << VKL_TARGET_WIDTH
            << ">): " << CALL_ISPC(VdbIterator_sizeOf) << " B / "
            << VKL_TARGET_WIDTH << " lanes = "
            << CALL_ISPC(VdbIterator_sizeOf) /
                   static_cast<float>(VKL_TARGET_WIDTH)
            << " B / lane" << std::endl;
  std::cout << "sizeof(VdbIterator<" << VKL_TARGET_WIDTH << ">): "
            << sizeof(openvkl::ispc_driver::VdbIterator<VKL_TARGET_WIDTH>)
            << " B" << std::endl;
  return 0;
}
