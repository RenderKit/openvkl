// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include "VKLCommon.h"
#include "openvkl/VKLLogLevel.h"

namespace openvkl {

  OPENVKL_CORE_INTERFACE VKLLogLevel logLevel();

  OPENVKL_CORE_INTERFACE void postLogMessage(
      const std::string &msg, VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);

  // stream class to facilitate usage as e.g.:
  // postLogMessage(VKL_LOG_ERROR) << "my error" << std::endl;
  struct LogMessageStream : public std::stringstream
  {
    LogMessageStream(VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);
    LogMessageStream(LogMessageStream &&other);
    ~LogMessageStream();

   private:
    VKLLogLevel logLevel{VKL_LOG_DEBUG};
  };

  inline LogMessageStream::LogMessageStream(VKLLogLevel postAtLogLevel)
      : logLevel(postAtLogLevel)
  {
  }

  inline LogMessageStream::~LogMessageStream()
  {
    auto msg = str();
    if (!msg.empty())
      postLogMessage(msg, logLevel);
  }

  inline LogMessageStream::LogMessageStream(LogMessageStream &&other)
  {
    this->str(other.str());
  }

  OPENVKL_CORE_INTERFACE LogMessageStream
  postLogMessage(VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);

}  // namespace openvkl
