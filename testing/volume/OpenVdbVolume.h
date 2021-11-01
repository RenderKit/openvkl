// Copyright 2020-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#if defined(OPENVKL_UTILITY_VDB_OPENVDB_ENABLED)

#include "TestingVolume.h"
#include "openvkl/utility/vdb/OpenVdbGrid.h"
#include "rkcommon/tasking/AsyncTask.h"
#include "rkcommon/utility/CodeTimer.h"

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    struct OpenVdbVolume : public TestingVolume
    {
      static OpenVdbVolume *loadVdbFile(VKLDevice device,
                                        const std::string &filename,
                                        const std::string &field,
                                        bool deferLeaves = false);

      virtual ~OpenVdbVolume() {}

      virtual bool updateVolume(VKLObserver leafAccessObserver) = 0;

      virtual VKLObserver newLeafAccessObserver(VKLSampler sampler) const = 0;
    };

    template <typename OpenVdbGridType>
    struct OpenVdbVolumeImpl : public OpenVdbVolume
    {
      using Grid = OpenVdbGridType;

      OpenVdbVolumeImpl(const OpenVdbVolumeImpl &other) = delete;
      OpenVdbVolumeImpl &operator=(const OpenVdbVolumeImpl &other) = delete;
      OpenVdbVolumeImpl(OpenVdbVolumeImpl &&other)                 = default;
      OpenVdbVolumeImpl &operator=(OpenVdbVolumeImpl &&other) = default;

      OpenVdbVolumeImpl(VKLDevice device,
                        const std::string &filename,
                        const std::string &field,
                        bool deferLeaves = false)
          : grid(device, filename, field, deferLeaves)
      {
      }

      ~OpenVdbVolumeImpl()
      {
        if (asyncLoader) {
          if (asyncLoader->valid())
            asyncLoader->wait();
          const auto result = asyncLoader->get();
          asyncLoader.reset();
          if (result.volume)
            vklRelease(result.volume);
        }
      }

      bool updateVolume(VKLObserver leafAccessObserver) override
      {
        bool changed = false;
        if (!asyncLoader && grid.numDeferred() > 0) {
          asyncLoader.reset(
              new rkcommon::tasking::AsyncTask<AsyncResult>([=]() {
                // Load remaining leaves, but use the usage buffer as guidance.
                AsyncResult result;
                result.volume = nullptr;

                rkcommon::utility::CodeTimer loadTimer;
                loadTimer.start();

                grid.loadDeferred(leafAccessObserver);
                // Committing takes longer the more leaves there are, so commit
                // less frequently, but with more nodes, the more data is in
                // core.
                grid.loadDeferred(lastLoadMS);

                loadTimer.stop();
                result.loadMS = loadTimer.milliseconds();

                rkcommon::utility::CodeTimer commitTimer;
                commitTimer.start();
                result.volume = grid.createVolume();
                commitTimer.stop();
                result.commitMS = commitTimer.milliseconds();

                return result;
              }));
        } else if (asyncLoader && asyncLoader->finished()) {
          AsyncResult result = asyncLoader->get();
          asyncLoader.reset();
          if (result.volume) {
            changed = true;
            vklRelease(volume);

            volume = result.volume;

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

      VKLObserver newLeafAccessObserver(VKLSampler sampler) const override
      {
        VKLObserver observer = nullptr;
        if (grid.numDeferred() > 0)
          observer = vklNewSamplerObserver(sampler, "LeafNodeAccess");
        return observer;
      }

     protected:
      struct AsyncResult
      {
        VKLVolume volume{nullptr};
        uint64_t loadMS{0};    // The time it took to load leaves.
        uint64_t commitMS{0};  // The time it took to commit.
      };

      void generateVKLVolume(VKLDevice device) override
      {
        if (device != grid.getVKLDevice()) {
          throw std::runtime_error(
              "specified device not compatible with grid device");
        }

        volume = grid.createVolume();
      }

     private:
      Grid grid;
      uint64_t lastLoadMS{30};
      std::unique_ptr<rkcommon::tasking::AsyncTask<AsyncResult>> asyncLoader;
    };

    using OpenVdbFloatVolume =
        OpenVdbVolumeImpl<openvkl::utility::vdb::OpenVdbFloatGrid>;
    using OpenVdbVec3sVolume =
        OpenVdbVolumeImpl<openvkl::utility::vdb::OpenVdbVec3sGrid>;

    inline OpenVdbVolume *OpenVdbVolume::loadVdbFile(
        VKLDevice device,
        const std::string &filename,
        const std::string &field,
        bool deferLeaves)
    {
      openvdb::initialize();

      openvdb::io::File file(filename.c_str());
      file.open();

      if (!file.hasGrid(field)) {
        std::ostringstream os;
        os << "invalid field '" << field << "'. valid field names are:";
        for (auto it = file.beginName(); it != file.endName(); ++it) {
          os << " '" << *it << "'";
        }
        throw std::runtime_error(os.str().c_str());
      }

      openvdb::GridBase::Ptr baseGrid = file.readGridMetadata(field);

      if (baseGrid->valueType() == "float") {
        return new OpenVdbFloatVolume(device, filename, field, deferLeaves);
      } else if (baseGrid->valueType() == "vec3s") {
        return new OpenVdbVec3sVolume(device, filename, field, deferLeaves);
      } else {
        throw std::runtime_error("unsupported OpenVDB grid type: " +
                                 baseGrid->valueType());
      }
    }

  }  // namespace testing
}  // namespace openvkl

#else  // defined(OPENVKL_UTILITY_VDB_OPENVDB_ENABLED)

#include "TestingVolume.h"
#include "openvkl/vdb.h"

namespace openvkl {
  namespace testing {

    struct OpenVdbVolume : public TestingVolume
    {
      static OpenVdbVolume *loadVdbFile(VKLDevice device,
                                        const std::string &filename,
                                        const std::string &field,
                                        bool deferLeaves = false)
      {
        throw std::runtime_error(
            "You must compile with OpenVDB to use OpenVdbVolume");
      }

      virtual ~OpenVdbVolume() {}

      virtual bool updateVolume(VKLObserver leafAccessObserver) = 0;

      virtual VKLObserver newLeafAccessObserver(VKLSampler sampler) const = 0;
    };

    struct OpenVdbVolumeImpl : public OpenVdbVolume
    {
      OpenVdbVolumeImpl(const std::string &filename,
                        const std::string &field,
                        bool deferLeaves = false)
      {
        throw std::runtime_error(
            "You must compile with OpenVDB to use OpenVdbVolumeImpl");
      }

      bool updateVolume(VKLObserver leafAccessObserver) override
      {
        return false;
      }

      range1f getComputedValueRange() const override
      {
        return range1f(0.f, 0.f);
      }

      VKLObserver newLeafAccessObserver(VKLSampler sampler) const override
      {
        return nullptr;
      }

     protected:
      void generateVKLVolume(VKLDevice device) override {}
    };

    using OpenVdbFloatVolume = OpenVdbVolumeImpl;
    using OpenVdbVec3sVolume = OpenVdbVolumeImpl;

  }  // namespace testing
}  // namespace openvkl

#endif  // defined(OPENVKL_UTILITY_VDB_OPENVDB_ENABLED)
