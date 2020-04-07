// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
