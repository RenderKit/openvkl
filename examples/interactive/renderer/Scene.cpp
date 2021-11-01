// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Scene.h"
#include "CommandLine.h"
#include "DensityPathTracer.h"
#include "HitIteratorRenderer.h"
#include "IntervalIteratorDebug.h"
#include "RayMarchIteratorRenderer.h"
#include "Renderer.h"

#include <algorithm>
#include <iostream>

namespace openvkl {
  namespace examples {

    Volume::Volume() = default;

    Volume::Volume(Volume &&other)
    {
      using std::swap;
      swap(volumeParams, other.volumeParams);
      swap(samplerParams, other.samplerParams);
      swap(testingVolume, other.testingVolume);
      swap(vklSampler, other.vklSampler);
    }

    Volume &Volume::operator=(Volume &&other)
    {
      if (this != &other) {
        using std::swap;
        swap(volumeParams, other.volumeParams);
        swap(samplerParams, other.samplerParams);
        swap(testingVolume, other.testingVolume);
        swap(vklSampler, other.vklSampler);
      }
      return *this;
    }

    Volume::~Volume()
    {
      if (vklSampler) {
        vklRelease(vklSampler);
        vklSampler = nullptr;
      }
      vklVolume = nullptr;
    }

    void Volume::parseCommandLine(std::list<std::string> &args)
    {
      volumeParams.parseCommandLine(args);
      samplerParams.parseCommandLine(args, volumeParams);
    }

    void Volume::usage() const
    {
      volumeParams.usage();
      samplerParams.usage();
    }

    void Volume::updateVKLObjects()
    {
      // VolumeParams::createVolume may throw with invalid params. We try
      // to be robust to that, and avoid updating the volume in this case.
      try {
        if (!vklVolume || volumeNeedsUpdate) {
          auto newVolume = volumeParams.createVolume();

          if (vklSampler) {
            vklRelease(vklSampler);
            vklSampler = nullptr;
          }

          testingVolume = std::move(newVolume);
          vklVolume     = testingVolume->getVKLVolume(getOpenVKLDevice());
          reinterpret_cast<vkl_box3f &>(volumeBounds) =
              vklGetBoundingBox(vklVolume);
          numAttributes = vklGetNumAttributes(vklVolume);
        }

        if (!vklSampler) {
          vklSampler         = vklNewSampler(vklVolume);
          samplerNeedsUpdate = true;
        }

        if (samplerNeedsUpdate) {
          samplerParams.updateSampler(vklSampler);
        }
      } catch (const std::exception &e) {
        if (testingVolume) {
          std::cerr << e.what() << std::endl;
        } else {
          // If we have no volume at all, we terminate. This will be caught
          // in vklExamples::main().
          throw;
        }
      }

      // If the update failed, do not try to update again.
      volumeNeedsUpdate  = false;
      samplerNeedsUpdate = false;
    }

    // -------------------------------------------------------------------------
    const std::vector<std::string> &Scene::supportedRendererTypes() const
    {
      static const std::vector<std::string> types = {
          "density_pathtracer",
          "density_pathtracer_ispc",
          "hit_iterator",
          "hit_iterator_ispc",
          "ray_march_iterator",
          "ray_march_iterator_ispc",
          "interval_iterator_debug",
          "interval_iterator_debug_ispc"};
      return types;
    }

    bool Scene::validateRendererType(const std::string &type) const
    {
      const auto &types = supportedRendererTypes();
      return (std::find(types.begin(), types.end(), type) != types.end());
    }

    std::unique_ptr<Renderer> Scene::createRenderer(const std::string &type)
    {
      std::unique_ptr<Renderer> ptr;

      if (type == "density_pathtracer") {
        ptr = rkcommon::make_unique<DensityPathTracer>(*this);
      } else if (type == "density_pathtracer_ispc") {
        ptr = rkcommon::make_unique<DensityPathTracerIspc>(*this);
      } else if (type == "hit_iterator") {
        ptr = rkcommon::make_unique<HitIteratorRenderer>(*this);
      } else if (type == "hit_iterator_ispc") {
        ptr = rkcommon::make_unique<HitIteratorRendererIspc>(*this);
      } else if (type == "interval_iterator_debug") {
        ptr = rkcommon::make_unique<IntervalIteratorDebug>(*this);
      } else if (type == "interval_iterator_debug_ispc") {
        ptr = rkcommon::make_unique<IntervalIteratorDebugIspc>(*this);
      } else if (type == "ray_march_iterator") {
        ptr = rkcommon::make_unique<RayMarchIteratorRenderer>(*this);
      } else if (type == "ray_march_iterator_ispc") {
        ptr = rkcommon::make_unique<RayMarchIteratorRendererIspc>(*this);
      }

      return ptr;
    }

    bool Scene::parseCommandLine(const std::list<std::string> &_args)
    {
      std::list<std::string> args = _args;

      bool terminate   = false;
      bool printUsage  = false;
      bool synchronous = false;

      if (args.empty()) {
        std::runtime_error(
            "invalid argument list provided to Scene::parseCommandLine");
      }
      const std::string programName = args.front();
      args.pop_front();

      try {
        for (auto it = args.begin(); it != args.end() && !terminate;) {
          const std::string arg = *it;
          if (arg == "-h" || arg == "-help" || arg == "--help") {
            cmd::consume_0(args, it);
            terminate  = true;
            printUsage = true;
          } else if (arg == "-batch") {
            cmd::consume_0(args, it);
            interactive = false;
          } else if (arg == "-spp") {
            batchModeSpp = cmd::consume_1<unsigned>(args, it);
          } else if (arg == "-disable-vsync") {
            cmd::consume_0(args, it);
            disableVSync = true;
          } else if (arg == "-renderer") {
            const std::string type = cmd::consume_1<std::string>(args, it);
            if (type == "all") {
              rendererTypes = supportedRendererTypes();
            } else if (validateRendererType(type)) {
              rendererTypes.push_back(type);
            } else {
              std::cerr << "unknown renderer type: " << type << std::endl;
              terminate = true;
            }
          } else if (arg == "-sync") {
            cmd::consume_0(args, it);
            synchronous = true;
          } else {
            ++it;
          }
        }

        if (!terminate) {
          volume.parseCommandLine(args);
          rendererParams->parseCommandLine(args);
        }
      } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        terminate = true;
      }

      for (const auto &arg : args) {
        std::cerr << "unknown argument: " << arg << std::endl;
        terminate = true;
      }

      if (printUsage) {
        std::cerr << "usage: " << programName
                  << "\n"
                     "\t-h, -help\n"
                     "\t-batch\n"
                     "\t-spp SPP (only in -batch mode)\n"
                     "\t-sync\n"
                     "\t-disable-vsync\n"
                     "\t-renderer ";

        const auto &types = supportedRendererTypes();
        for (auto it = types.begin(); it != types.end(); ++it) {
          if (it != types.begin()) {
            std::cerr << "| ";
          }
          std::cerr << *it;
        }
        std::cerr << "\n";

        volume.usage();
        rendererParams->usage();

        std::cerr << std::flush;
        terminate = true;
      }

      if (!interactive) {
        synchronous = true;
      }

      scheduler = Scheduler(synchronous);

      return !terminate;
    }

  }  // namespace examples
}  // namespace openvkl
