// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "logging.h"
#include "../api/Driver.h"
#include "rkcommon/utility/StringManip.h"
#include "rkcommon/utility/getEnvVar.h"

#define LOG_PREFIX "[openvkl] "

namespace openvkl {

  LogMessageStream postLogMessage(VKLLogLevel postAtLogLevel)
  {
    return LogMessageStream(postAtLogLevel);
  }

  void postLogMessage(const std::string &msg, VKLLogLevel postAtLogLevel)
  {
    if (postAtLogLevel >= api::Driver::logLevel) {
      if (api::driverIsSet()) {
        api::Driver::current->logCallback(api::Driver::current->logUserData,
                                          (LOG_PREFIX + msg + '\n').c_str());
      } else {
        std::cout << LOG_PREFIX << msg << std::endl;
      }
    }
  }

}  // namespace openvkl
