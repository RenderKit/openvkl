// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "../openvkl/drivers/ispc/common/align.h"

#include <iostream>

using namespace openvkl::ispc_driver;

TEST_CASE("Alignment", "")
{
  SECTION("Untyped alignment")
  {
    for (std::intptr_t p = 0; p < 100000; ++p)
    for (size_t a = 1; a < 1024; a<<=1)
    {
      void *ptr = reinterpret_cast<void *>(p);
      void *alignedPtr = align(ptr, a);
      REQUIRE(reinterpret_cast<std::intptr_t>(alignedPtr) % a == 0);
    }
  }
  SECTION("Typed alignment")
  {
    for (std::intptr_t p = 0; p < 100000; ++p)
    {
      void *ptr = reinterpret_cast<void *>(p);
      void *alignedPtr = align<std::uint64_t>(ptr);
      REQUIRE(reinterpret_cast<std::intptr_t>(alignedPtr) % alignof(std::uint64_t) == 0);
    }
  }
}

