// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdexcept>
#include <sstream>

namespace openvkl {
  namespace cpu_device {

    /*
     * A helper for runtime errors that works a bit like Python's print, 
     * turning all arguments into strings and joining them.
     */
    template <class... Args>
    void runtimeError(Args &&...args)
    {
      std::ostringstream os;
      int dummy[sizeof...(Args)] = {(os << std::forward<Args>(args), 0)...};
      (void)dummy;
      throw std::runtime_error(os.str());
    }

  }  // namespace cpu_device
}  // namespace openvkl
