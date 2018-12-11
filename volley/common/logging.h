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

#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include "VLYCommon.h"

namespace volley {

  typedef enum : uint32_t
  {
    VLY_LOG_DEBUG   = 100,
    VLY_LOG_INFO    = 200,
    VLY_LOG_WARNING = 300,
    VLY_LOG_ERROR   = 400
  } VLYLogLevel;

  VOLLEY_CORE_INTERFACE VLYLogLevel logLevel();

  VOLLEY_CORE_INTERFACE void postLogMessage(
      const std::string &msg, VLYLogLevel postAtLogLevel = VLY_LOG_DEBUG);

  // stream class to facilitate usage as e.g.:
  // postLogMessage(VLY_LOG_ERROR) << "my error" << std::endl;
  struct LogMessageStream : public std::stringstream
  {
    LogMessageStream(VLYLogLevel postAtLogLevel = VLY_LOG_DEBUG);
    LogMessageStream(LogMessageStream &&other);
    ~LogMessageStream();

   private:
    VLYLogLevel logLevel{VLY_LOG_DEBUG};
  };

  inline LogMessageStream::LogMessageStream(VLYLogLevel postAtLogLevel)
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

  VOLLEY_CORE_INTERFACE LogMessageStream
  postLogMessage(VLYLogLevel postAtLogLevel = VLY_LOG_DEBUG);

}  // namespace volley
