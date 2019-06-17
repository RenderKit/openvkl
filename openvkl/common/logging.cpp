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
#include "../api/Driver.h"

#define LOG_PREFIX "[openvkl] "

namespace openvkl {

  VKLLogLevel logLevel()
  {
    return VKL_LOG_DEBUG;
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
