// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Observer.h"

namespace openvkl {

  Observer::~Observer() = default;

  std::string Observer::toString() const
  {
    return "openvkl::Observer";
  }

}  // namespace openvkl

