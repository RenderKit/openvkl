// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "logging.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include "../api/Driver.h"

#define LOG_PREFIX "[openvkl] "

namespace openvkl {

  VKLLogLevel logLevel()
  {
    static VKLLogLevel logLevel = VKL_LOG_INFO;

    static bool checkedEnvironment = false;

    if (!checkedEnvironment) {
      checkedEnvironment = true;

      const char *envLogLevel = getenv("VKL_LOG_LEVEL");

      if (!envLogLevel) {
        return logLevel;
      }

      std::string envLogLevelString(envLogLevel);

      if (!envLogLevelString.empty()) {
        std::transform(envLogLevelString.begin(),
                       envLogLevelString.end(),
                       envLogLevelString.begin(),
                       [](unsigned char c) { return std::tolower(c); });

        if (envLogLevelString == "debug") {
          logLevel = VKL_LOG_DEBUG;
        } else if (envLogLevelString == "info") {
          logLevel = VKL_LOG_INFO;
        } else if (envLogLevelString == "warning") {
          logLevel = VKL_LOG_WARNING;
        } else if (envLogLevelString == "error") {
          logLevel = VKL_LOG_ERROR;
        } else {
          postLogMessage(
              "unknown VKL_LOG_LEVEL env value; must be DEBUG, INFO, WARNING, "
              "or ERROR",
              VKL_LOG_ERROR);
        }
      }
    }

    return logLevel;
  }

  LogMessageStream postLogMessage(VKLLogLevel postAtLogLevel)
  {
    return LogMessageStream(postAtLogLevel);
  }

  void postLogMessage(const std::string &msg, VKLLogLevel postAtLogLevel)
  {
    if (postAtLogLevel >= logLevel()) {
      if (api::driverIsSet()) {
        api::Driver::current->messageFunction((LOG_PREFIX + msg + '\n').c_str());
      } else {
        std::cout << LOG_PREFIX << msg << std::endl;
      }
    }
  }

}  // namespace openvkl
