// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Device.h"
#include <sstream>
#include "../common/objectFactory.h"
#include "ispc_util_ispc.h"
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/StringManip.h"
#include "rkcommon/utility/getEnvVar.h"

namespace openvkl {
  namespace api {

    // helper functions
    template <std::ostream &OS>
    static inline void installLogCallback(Device &device)
    {
      device.logUserData = nullptr;
      device.logCallback = [](void * /*usrData*/, const char *msg) {
        OS << msg;
      };
    }

    template <std::ostream &OS>
    static inline void installErrorCallback(Device &device)
    {
      device.errorUserData = nullptr;
      device.errorCallback =
          [](void * /*usrData*/, VKLError e, const char *msg) {
            OS << "OPENVKL ERROR [" << e << "]: " << msg << std::endl;
          };
    }

    // Device definitions

    Device::Device()
    {
      // setup default logging functions; after the device is instantiated, they
      // may be overridden via vklDeviceSet*Callback().
      installLogCallback<std::cout>(*this);
      installErrorCallback<std::cerr>(*this);
    }

    Device *Device::createDevice(const std::string &deviceName)
    {
      const std::string cpuDeviceName("cpu");

      // special case for CPU device selection
      if (deviceName.find(cpuDeviceName) != std::string::npos) {
        int requestedDeviceWidth = 0;

        const int nativeMaxIspcWidth = ispc::get_programCount();

        if (deviceName == cpuDeviceName) {
          // the generic "cpu" device was selected

          // update requested device width if the user set the env var
          auto OPENVKL_CPU_DEVICE_DEFAULT_WIDTH =
              utility::getEnvVar<int>("OPENVKL_CPU_DEVICE_DEFAULT_WIDTH");
          requestedDeviceWidth = OPENVKL_CPU_DEVICE_DEFAULT_WIDTH.value_or(0);

          if (requestedDeviceWidth) {
            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "application requested CPU device width "
                << requestedDeviceWidth
                << " via OPENVKL_CPU_DEVICE_DEFAULT_WIDTH";
          } else {
            // the user did not set the env var, use the runtime native
            // (maximum) ISPC vector width
            requestedDeviceWidth = nativeMaxIspcWidth;

            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "will use ISPC device native maximum width "
                << requestedDeviceWidth;
          }

        } else if (deviceName.find(cpuDeviceName + "_") != std::string::npos &&
                   deviceName.size() > cpuDeviceName.size() + 1) {
          // the user chose a specific width for the CPU device, e.g.
          // cpu_[4,8,16]. verify that device is legal on this system.
          std::string specifiedWidthStr =
              deviceName.substr(deviceName.find("_") + 1, deviceName.size());

          try {
            requestedDeviceWidth = std::stoi(specifiedWidthStr);

            LogMessageStream(nullptr, VKL_LOG_DEBUG)
                << "application requested ISPC device width "
                << requestedDeviceWidth << "via device name " << deviceName;
          } catch (const std::invalid_argument &ia) {
            LogMessageStream(nullptr, VKL_LOG_ERROR)
                << "could not parse requested ISPC device width for name: "
                << deviceName;
          }
        }

        // verify legality of the determined width
        if (requestedDeviceWidth <= 0 ||
            requestedDeviceWidth > nativeMaxIspcWidth) {
          std::stringstream ss;
          ss << "device " << deviceName
             << " cannot run on the system (native SIMD width: "
             << nativeMaxIspcWidth
             << ", requested SIMD width: " << requestedDeviceWidth << ")";

          throw std::runtime_error(ss.str().c_str());
        }

        std::stringstream ss;
        ss << cpuDeviceName << "_" << requestedDeviceWidth;

        return objectFactory<Device, VKL_DEVICE>(nullptr, ss.str());
      }

      return objectFactory<Device, VKL_DEVICE>(nullptr, deviceName);
    }

    void Device::commit()
    {
      // log level
      logLevel = VKLLogLevel(getParam<int>("logLevel", LOG_LEVEL_DEFAULT));

      // environment variable takes precedence
      auto OPENVKL_LOG_LEVEL = utility::lowerCase(
          utility::getEnvVar<std::string>("OPENVKL_LOG_LEVEL")
              .value_or(std::string()));

      if (!OPENVKL_LOG_LEVEL.empty()) {
        if (OPENVKL_LOG_LEVEL == "debug") {
          logLevel = VKL_LOG_DEBUG;
        } else if (OPENVKL_LOG_LEVEL == "info") {
          logLevel = VKL_LOG_INFO;
        } else if (OPENVKL_LOG_LEVEL == "warning") {
          logLevel = VKL_LOG_WARNING;
        } else if (OPENVKL_LOG_LEVEL == "error") {
          logLevel = VKL_LOG_ERROR;
        } else if (OPENVKL_LOG_LEVEL == "none") {
          logLevel = VKL_LOG_NONE;
        } else {
          LogMessageStream(this, VKL_LOG_ERROR)
              << "unknown OPENVKL_LOG_LEVEL env value; must be debug, info, "
                 "warning, error or none";
        }
      }

      // log output
      auto OPENVKL_LOG_OUTPUT =
          utility::getEnvVar<std::string>("OPENVKL_LOG_OUTPUT");

      auto dst =
          OPENVKL_LOG_OUTPUT.value_or(getParam<std::string>("logOutput", ""));

      if (dst == "cout")
        installLogCallback<std::cout>(*this);
      else if (dst == "cerr")
        installLogCallback<std::cerr>(*this);
      else if (dst == "none") {
        logCallback = [](void *, const char *) {};
        logUserData = nullptr;
      }

      // error output
      auto OPENVKL_ERROR_OUTPUT =
          utility::getEnvVar<std::string>("OPENVKL_ERROR_OUTPUT");

      dst = OPENVKL_ERROR_OUTPUT.value_or(
          getParam<std::string>("errorOutput", ""));

      if (dst == "cout")
        installErrorCallback<std::cout>(*this);
      else if (dst == "cerr")
        installErrorCallback<std::cerr>(*this);
      else if (dst == "none") {
        errorCallback = [](void *, VKLError, const char *) {};
        errorUserData = nullptr;
      }

      // threads
      auto OPENVKL_THREADS = utility::getEnvVar<int>("OPENVKL_THREADS");
      int numThreads =
          OPENVKL_THREADS.value_or(getParam<int>("numThreads", -1));

      // flush to zero / denormals are zero
      auto OPENVKL_FLUSH_DENORMALS =
          utility::getEnvVar<int>("OPENVKL_FLUSH_DENORMALS");
      bool flushDenormals =
          OPENVKL_FLUSH_DENORMALS.value_or(getParam<int>("flushDenormals", 1));

      tasking::initTaskingSystem(numThreads, flushDenormals);

      committed = true;
    }

    bool Device::isCommitted()
    {
      return committed;
    }

  }  // namespace api
}  // namespace openvkl
