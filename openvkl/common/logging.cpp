// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "logging.h"
#include "../api/Device.h"
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
    if (postAtLogLevel >= api::Device::logLevel) {
      if (api::deviceIsSet()) {
        api::Device::current->logCallback(api::Device::current->logUserData,
                                          (LOG_PREFIX + msg + '\n').c_str());
      } else {
        std::cout << LOG_PREFIX << msg << std::endl;
      }
    }
  }

}  // namespace openvkl
