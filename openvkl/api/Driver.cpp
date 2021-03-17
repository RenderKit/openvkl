// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Driver.h"
#include <sstream>
#include "../common/objectFactory.h"
#include "ispc_util_ispc.h"
#include "rkcommon/tasking/tasking_system_init.h"
#include "rkcommon/utility/StringManip.h"
#include "rkcommon/utility/getEnvVar.h"

#ifdef NDEBUG
#define LOG_LEVEL_DEFAULT VKL_LOG_INFO
#else
#define LOG_LEVEL_DEFAULT VKL_LOG_DEBUG
#endif

namespace openvkl {
  namespace api {

    // helper functions
    template <std::ostream &OS>
    static inline void installLogCallback(Driver &driver)
    {
      driver.logUserData = nullptr;
      driver.logCallback = [](void * /*usrData*/, const char *msg) {
        OS << msg;
      };
    }

    template <std::ostream &OS>
    static inline void installErrorCallback(Driver &driver)
    {
      driver.errorUserData = nullptr;
      driver.errorCallback =
          [](void * /*usrData*/, VKLError e, const char *msg) {
            OS << "OPENVKL ERROR [" << e << "]: " << msg << std::endl;
          };
    }

    // Driver definitions
    memory::IntrusivePtr<Driver> Driver::current;

    VKLLogLevel Driver::logLevel = LOG_LEVEL_DEFAULT;

    Driver::Driver()
    {
      // setup default logging functions; after the driver is instantiated, they
      // may be overridden via vklDriverSet*Callback().
      installLogCallback<std::cout>(*this);
      installErrorCallback<std::cerr>(*this);
    }

    Driver *Driver::createDriver(const std::string &driverName)
    {
      const std::string ispcDriverName("ispc");

      // special case for ISPC driver selection
      if (driverName.find(ispcDriverName) != std::string::npos) {
        int requestedDriverWidth = 0;

        const int nativeMaxIspcWidth = ispc::get_programCount();

        if (driverName == ispcDriverName) {
          // the generic "ispc" driver was selected

          // update requested driver width if the user set the env var
          auto OPENVKL_ISPC_DRIVER_DEFAULT_WIDTH =
              utility::getEnvVar<int>("OPENVKL_ISPC_DRIVER_DEFAULT_WIDTH");
          requestedDriverWidth = OPENVKL_ISPC_DRIVER_DEFAULT_WIDTH.value_or(0);

          if (requestedDriverWidth) {
            LogMessageStream(VKL_LOG_DEBUG)
                << "application requested ISPC driver width "
                << requestedDriverWidth
                << " via OPENVKL_ISPC_DRIVER_DEFAULT_WIDTH";
          } else {
            // the user did not set the env var, use the runtime native
            // (maximum) ISPC vector width
            requestedDriverWidth = nativeMaxIspcWidth;

            LogMessageStream(VKL_LOG_DEBUG)
                << "will use ISPC driver native maximum width "
                << requestedDriverWidth;
          }

        } else if (driverName.find(ispcDriverName + "_") != std::string::npos &&
                   driverName.size() > ispcDriverName.size() + 1) {
          // the user chose a specific width for the ISPC driver, e.g.
          // ispc_[4,8,16]. verify that driver is legal on this system.
          std::string specifiedWidthStr =
              driverName.substr(driverName.find("_") + 1, driverName.size());

          try {
            requestedDriverWidth = std::stoi(specifiedWidthStr);

            LogMessageStream(VKL_LOG_DEBUG)
                << "application requested ISPC driver width "
                << requestedDriverWidth << "via driver name " << driverName;
          } catch (const std::invalid_argument &ia) {
            LogMessageStream(VKL_LOG_ERROR)
                << "could not parse requested ISPC driver width for name: "
                << driverName;
          }
        }

        // verify legality of the determined width
        if (requestedDriverWidth <= 0 ||
            requestedDriverWidth > nativeMaxIspcWidth) {
          std::stringstream ss;
          ss << "driver " << driverName
             << " cannot run on the system (native SIMD width: "
             << nativeMaxIspcWidth
             << ", requested SIMD width: " << requestedDriverWidth << ")";

          throw std::runtime_error(ss.str().c_str());
        }

        std::stringstream ss;
        ss << ispcDriverName << "_" << requestedDriverWidth;

        return objectFactory<Driver, VKL_DRIVER>(ss.str());
      }

      return objectFactory<Driver, VKL_DRIVER>(driverName);
    }

    void Driver::commit()
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
          LogMessageStream(VKL_LOG_ERROR)
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

    bool Driver::isCommitted()
    {
      return committed;
    }

    bool driverIsSet()
    {
      return Driver::current.ptr != nullptr;
    }

    Driver &currentDriver()
    {
      return *Driver::current;
    }

  }  // namespace api
}  // namespace openvkl
