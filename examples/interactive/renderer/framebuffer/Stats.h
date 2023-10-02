// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <chrono>

#include "openvkl_testing.h"

namespace openvkl {
  namespace examples {
    struct Stats
    {
     public:
      using Clock = std::chrono::high_resolution_clock;

      Clock::duration frameTime{0};
      Clock::duration renderTime{0};
      Clock::duration tonemapTime{0};
      Clock::duration copyTime{0};

      void printToStdout() const
      {
        const std::string thematicBreak = "-------------------------";

        std::cout << std::endl << std::endl << thematicBreak << std::endl;

        printOneValue("frameTime", frameTime);
        printOneValue("renderTime", renderTime);
        printOneValue("tonemapTime", tonemapTime);
        printOneValue("copyTime", copyTime);

        std::cout << thematicBreak << std::endl << std::endl;
      }

#ifdef OPENVKL_TESTING_GPU
      static Clock::duration convert(const sycl::event &syclEvent)
      {
        auto end =
            syclEvent
                .get_profiling_info<sycl::info::event_profiling::command_end>();
        auto start = syclEvent.get_profiling_info<
            sycl::info::event_profiling::command_start>();

        return std::chrono::high_resolution_clock::duration{end - start};
      }
#endif
     private:
      void printOneValue(const std::string &name,
                         const Clock::duration &value) const
      {
        using MuS = std::chrono::microseconds;
        std::cout << std::setw(12) << name << " [ms]: "
                  << std::chrono::duration_cast<MuS>(value).count() / 1000.0
                  << std::endl;
      }
    };
  }  // namespace examples
}  // namespace openvkl
