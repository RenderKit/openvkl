// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../../external/catch.hpp"
#include "openvkl/VKLBackgroundUndefined.h"

#include <cmath>

TEST_CASE("Background undefined", "")
{
  SECTION("Is NaN")
  {
    REQUIRE(std::isnan(VKL_BACKGROUND_UNDEFINED));
  }

  SECTION("Is non signalling")
  {
    REQUIRE(!(VKL_BACKGROUND_UNDEFINED == VKL_BACKGROUND_UNDEFINED));
  }
}
