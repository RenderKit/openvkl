// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/os/library.h"
#include <map>
#include "VKLCommon.h"
#include "logging.h"
#include "openvkl/VKLDataType.h"

namespace openvkl {

  template <typename T, VKLDataType VKL_TYPE = VKL_UNKNOWN>
  inline T *objectFactory(const std::string &type)
  {
    // Function pointer type for creating a concrete instance of a subtype of
    // this class.
    using creationFunctionPointer = T *(*)();

    // Function pointers corresponding to each subtype.
    static std::map<std::string, creationFunctionPointer> symbolRegistry;
    const auto type_string = stringFor(VKL_TYPE);

    // Find the creation function for the subtype if not already known.
    if (symbolRegistry.count(type) == 0) {
      postLogMessage(VKL_LOG_DEBUG)
          << "trying to look up " << type_string << " type '" << type
          << "' for the first time";

      // Construct the name of the creation function to look for.
      std::string creationFunctionName =
          "openvkl_create_" + type_string + "__" + type;

      // Look for the named function.
      symbolRegistry[type] =
          (creationFunctionPointer)rkcommon::getSymbol(creationFunctionName);

      // The named function may not be found if the requested subtype is not
      // known.
      if (!symbolRegistry[type]) {
        postLogMessage(VKL_LOG_WARNING)
            << "WARNING: unrecognized " << type_string << " type '" << type
            << "'.";
      }
    }

    // Create a concrete instance of the requested subtype.
    auto *object = symbolRegistry[type] ? (*symbolRegistry[type])() : nullptr;

    if (object == nullptr) {
      symbolRegistry.erase(type);
      throw std::runtime_error(
          "Could not find " + type_string + " of type: " + type +
          ".  Make sure you have the correct VKL libraries linked.");
    }

    return object;
  }

}  // namespace openvkl
