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

#include "Driver.h"
#include <sstream>
#include "../common/objectFactory.h"
#include "ispc_util_ispc.h"
#include "ospcommon/tasking/tasking_system_init.h"
#include "ospcommon/utility/getEnvVar.h"

namespace openvkl {
  namespace api {

    // helper functions
    template <typename OSTREAM_T>
    static inline void installLogFunction(Driver &driver, OSTREAM_T &stream)
    {
      driver.logFunction = [&](const char *msg) { stream << msg; };
    }

    template <typename OSTREAM_T>
    static inline void installErrorFunction(Driver &driver, OSTREAM_T &stream)
    {
      driver.errorFunction = [&](VKLError e, const char *msg) {
        stream << "OPENVKL ERROR [" << e << "]: " << msg << std::endl;
      };
    }

    // Driver definitions
    std::shared_ptr<Driver> Driver::current;

    Driver::Driver()
    {
      // setup default logging functions; after the driver is instantiated, they
      // may be overridden via vklDriverSet*Func().
      installLogFunction(*this, std::cout);
      installErrorFunction(*this, std::cerr);
    }

    Driver *Driver::createDriver(const char *driverName)
    {
      // special case for ISPC driver selection based on runtime native ISPC
      // vector width
      const char *ispcDriverName = "ispc";

      if (strcmp(driverName, ispcDriverName) == 0) {
        int nativeIspcWidth = ispc::get_programCount();

        std::stringstream ss;
        ss << ispcDriverName << "_" << nativeIspcWidth;

        return objectFactory<Driver, VKL_DRIVER>(ss.str().c_str());
      }

      return objectFactory<Driver, VKL_DRIVER>(driverName);
    }

    void Driver::commit()
    {
      auto OPENVKL_THREADS = utility::getEnvVar<int>("OPENVKL_THREADS");
      numThreads = OPENVKL_THREADS.value_or(getParam<int>("numThreads", -1));

      tasking::initTaskingSystem(numThreads);

      committed = true;
    }

    bool Driver::isCommitted()
    {
      return committed;
    }

    bool driverIsSet()
    {
      return Driver::current.get() != nullptr;
    }

    Driver &currentDriver()
    {
      return *Driver::current;
    }

  }  // namespace api
}  // namespace openvkl
