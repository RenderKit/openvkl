// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include "VKLCommon.h"
#include "openvkl/VKLLogLevel.h"

namespace openvkl {

  OPENVKL_CORE_INTERFACE VKLLogLevel logLevel();

  OPENVKL_CORE_INTERFACE void postLogMessage(
      Device *device,
      const std::string &msg,
      VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);

  // stream class to facilitate usage as e.g.:
  // postLogMessage(VKL_LOG_ERROR) << "my error" << std::endl;
  struct LogMessageStream : public std::stringstream
  {
    LogMessageStream(Device *device,
                     VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);
    LogMessageStream(LogMessageStream &&other);
    ~LogMessageStream();

   private:
    Device *device{nullptr};
    VKLLogLevel logLevel{VKL_LOG_DEBUG};
  };

  inline LogMessageStream::LogMessageStream(Device *device,
                                            VKLLogLevel postAtLogLevel)
      : device(device), logLevel(postAtLogLevel)
  {
  }

  inline LogMessageStream::~LogMessageStream()
  {
    auto msg = str();
    if (!msg.empty())
      postLogMessage(device, msg, logLevel);
  }

  inline LogMessageStream::LogMessageStream(LogMessageStream &&other)
  {
    this->str(other.str());
  }

  OPENVKL_CORE_INTERFACE LogMessageStream
  postLogMessage(Device *device, VKLLogLevel postAtLogLevel = VKL_LOG_DEBUG);

}  // namespace openvkl
