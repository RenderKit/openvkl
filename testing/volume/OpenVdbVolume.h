// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#if OPENVKL_VDB_UTIL_OPENVDB_ENABLED

#include "TestingVolume.h"
#include "openvkl/vdb_util/OpenVdbGrid.h"
#include "ospcommon/tasking/AsyncTask.h"
#include "ospcommon/utility/CodeTimer.h"

using namespace ospcommon;

namespace openvkl {
  namespace testing {

    struct OpenVdbFloatVolume : public TestingVolume
    {
      using Grid = openvkl::vdb_util::OpenVdbFloatGrid;

      OpenVdbFloatVolume(const OpenVdbFloatVolume &other) = delete;
      OpenVdbFloatVolume &operator=(const OpenVdbFloatVolume &other) = delete;
      OpenVdbFloatVolume(OpenVdbFloatVolume &&other)                 = default;
      OpenVdbFloatVolume &operator=(OpenVdbFloatVolume &&other) = default;

      OpenVdbFloatVolume(const std::string &filename,
                         const std::string &field,
                         VKLFilter filter)
          : grid(filename, field, true), filter(filter)
      {
        volume = grid.createVolume(filter);
        if (grid.numDeferred() > 0)
          leafAccessObserver = vklNewObserver(volume, "LeafNodeAccess");
      }

      ~OpenVdbFloatVolume()
      {
        if (asyncLoader) {
          if (asyncLoader->valid())
            asyncLoader->wait();
          const auto result = asyncLoader->get();
          asyncLoader.reset();
          if (result.volume)
            vklRelease(result.volume);
        }

        if (leafAccessObserver)
          vklRelease(leafAccessObserver);
      }

      bool updateVolume()
      {
        bool changed = false;
        if (!asyncLoader && grid.numDeferred() > 0) {
          asyncLoader.reset(
              new ospcommon::tasking::AsyncTask<AsyncResult>([=]() {
                // Load remaining leaves, but use the usage buffer as guidance.
                AsyncResult result;
                result.volume = nullptr;

                ospcommon::utility::CodeTimer loadTimer;
                loadTimer.start();

                grid.loadDeferred(leafAccessObserver);
                // Committing takes longer the more leaves there are, so commit
                // less frequently, but with more nodes, the more data is in
                // core.
                grid.loadDeferred(lastLoadMS);

                loadTimer.stop();
                result.loadMS = loadTimer.milliseconds();

                ospcommon::utility::CodeTimer commitTimer;
                commitTimer.start();
                result.volume = grid.createVolume(filter);
                commitTimer.stop();
                result.commitMS = commitTimer.milliseconds();

                return result;
              }));
        } else if (asyncLoader && asyncLoader->finished()) {
          AsyncResult result = asyncLoader->get();
          asyncLoader.reset();
          if (result.volume) {
            changed = true;
            if (leafAccessObserver) {
              vklRelease(leafAccessObserver);
              leafAccessObserver = nullptr;
            }
            vklRelease(volume);

            volume = result.volume;
            if (grid.numDeferred() > 0)
              leafAccessObserver = vklNewObserver(volume, "LeafNodeAccess");

            std::cout << "Done loading leaf data."
                      << " Load: " << result.loadMS << "ms"
                      << ", Commit: " << result.commitMS << "ms."
                      << " " << (grid.numNodes() - grid.numDeferred()) << " of "
                      << grid.numNodes() << " leaves are in core, "
                      << grid.numDeferred() << " remain out of core."
                      << std::endl;

            lastLoadMS = result.loadMS + result.commitMS;
          }
        }
        return changed;
      }

      range1f getComputedValueRange() const override
      {
        return range1f(0.f, 1.f);
      }

     protected:
      struct AsyncResult
      {
        VKLVolume volume{nullptr};
        uint64_t loadMS{0};    // The time it took to load leaves.
        uint64_t commitMS{0};  // The time it took to commit.
      };

      void generateVKLVolume() override {}

     private:
      Grid grid;
      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLObserver leafAccessObserver{nullptr};
      uint64_t lastLoadMS{30};
      std::unique_ptr<ospcommon::tasking::AsyncTask<AsyncResult>> asyncLoader;
    };

  }  // namespace testing
}  // namespace openvkl

#else  // OPENVKL_VDB_UTIL_OPENVDB_ENABLED

#include "TestingVolume.h"
#include "openvkl/vdb.h"

namespace openvkl {
  namespace testing {

    struct OpenVdbFloatVolume : public testing::TestingVolume
    {
      OpenVdbFloatVolume(const std::string &filename,
                         const std::string &field,
                         VKLFilter filter = VKL_FILTER_TRILINEAR,
                         bool forceInCore = false)
      {
        throw std::runtime_error(
            "You must compile with OpenVDB to use TestingVdbVolume");
      }

      bool updateVolume()
      {
        return false;
      }
      range1f getComputedValueRange() const override
      {
        return range1f(0.f, 0.f);
      }

     protected:
      void generateVKLVolume() override {}
    };

  }  // namespace testing
}  // namespace openvkl

#endif  // OPENVKL_VDB_UTIL_OPENVDB_ENABLED

