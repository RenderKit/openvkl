// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include <openvkl/openvkl.h>

#include <iostream>

TEST_CASE("Enum", "")
{
  SECTION("VKLUnstructuredCellType")
  {
    REQUIRE(sizeof(VKLUnstructuredCellType) == 1);
  }
}
