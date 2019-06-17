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

#include "Driver.h"
#include <sstream>
#include "../common/objectFactory.h"
#include "ispc_util_ispc.h"

namespace openvkl {
  namespace api {

    // helper functions
    template <typename OSTREAM_T>
    static inline void installMessageFunction(Driver &driver, OSTREAM_T &stream)
    {
      driver.messageFunction = [&](const char *msg) { stream << msg; };
    }

    template <typename OSTREAM_T>
    static inline void installErrorMessageFunction(Driver &driver,
                                                   OSTREAM_T &stream)
    {
      driver.errorFunction = [&](VKLError e, const char *msg) {
        stream << "OPENVKL ERROR [" << e << "]: " << msg << std::endl;
      };
    }

    template <typename OSTREAM_T>
    static inline void installTraceFunction(Driver &driver, OSTREAM_T &stream)
    {
      driver.traceFunction = [&](const char *msg) { stream << msg; };
    }

    // Driver definitions
    std::shared_ptr<Driver> Driver::current;

    Driver *Driver::createDriver(const char *driverName)
    {
      // special case for ISPC driver selection based on runtime native ISPC
      // vector width
      const char *ispcDriverName = "ispc_driver";

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
      // setup default logging functions
      installMessageFunction(*this, std::cout);
      installErrorMessageFunction(*this, std::cerr);

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
