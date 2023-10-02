// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
using namespace rkcommon;
using namespace rkcommon::math;

#include "../api/Device.h"
#include "logging.h"
#include "rkcommon/utility/StringManip.h"
#include "rkcommon/utility/getEnvVar.h"

#define LOG_PREFIX "[openvkl] "

namespace openvkl {

  LogMessageStream postLogMessage(Device *device, VKLLogLevel postAtLogLevel)
  {
    return LogMessageStream(device, postAtLogLevel);
  }

  void postLogMessage(Device *device,
                      const std::string &msg,
                      VKLLogLevel postAtLogLevel)
  {
    if (device && postAtLogLevel >= device->logLevel) {
      device->logCallback(device->logUserData,
                          (LOG_PREFIX + msg + '\n').c_str());
    } else if (postAtLogLevel >= LOG_LEVEL_DEFAULT) {
      std::cout << LOG_PREFIX << msg << std::endl;
    }
  }

}  // namespace openvkl
