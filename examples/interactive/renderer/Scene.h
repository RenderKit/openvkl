// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
#include "RendererParams.h"
#include "SamplerParams.h"
#include "Versioned.h"
#include "VolumeParams.h"

#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"

#include <cstdint>
#include <memory>

namespace openvkl {
  namespace examples {

    using namespace rkcommon::math;

    class Renderer;

    struct Volume
    {
     public:
      Volume();
      Volume(const Volume &)            = delete;
      Volume &operator=(const Volume &) = delete;
      Volume(Volume &&other);
      Volume &operator=(Volume &&other);
      ~Volume();

     public:
      // Renderers may only access the volume and sampler. Parameters etc.
      // are fully hidden.
      VKLVolume getVolume() const
      {
        return *vklVolume;
      }

      const VKLVolume *getVolumePtr() const
      {
        return vklVolume.get();
      }

      const box3f &getBounds() const
      {
        return volumeBounds;
      }

      unsigned int getNumAttributes() const
      {
        return numAttributes;
      }

      VKLSampler getSampler() const
      {
        return *vklSampler;
      }

      const VKLSampler *getSamplerPtr() const
      {
        return vklSampler.get();
      }

      VolumeParams &getVolumeParams()
      {
        return volumeParams;
      }

      void setVolumeDirty()
      {
        volumeNeedsUpdate = true;
      }

      bool volumeIsDirty() const
      {
        return volumeNeedsUpdate;
      }

      SamplerParams &getSamplerParams()
      {
        return samplerParams;
      }

      void setSamplerDirty()
      {
        samplerNeedsUpdate = true;
      }

      void printInfo() const
      {
        std::cout << "volumeType:     " << volumeParams.volumeType << std::endl;
        std::cout << "voxelType:     "
                  << voxelTypeToString(volumeParams.voxelType) << std::endl;
        std::cout << "gridDimensions: " << volumeParams.dimensions << std::endl;
        std::cout << "gridOrigin:     " << volumeParams.gridOrigin << std::endl;
        std::cout << "gridSpacing:    " << volumeParams.gridSpacing
                  << std::endl;
        std::cout << "source:         " << volumeParams.source << std::endl;

        std::cout << "boundingBox:    "
                  << "(" << volumeBounds.lower.x << ", " << volumeBounds.lower.y
                  << ", " << volumeBounds.lower.z << ") -> ("
                  << volumeBounds.upper.x << ", " << volumeBounds.upper.y
                  << ", " << volumeBounds.upper.z << ")" << std::endl;
      }

     public:
      // Command line parsing. This should only be called once on startup.
      void parseCommandLine(std::list<std::string> &args);
      void usage() const;

      // Update the VKL volume and sampler objects.
      // This should only be called from the main thread, and only if all
      // workers have been stopped.
      void updateVKLObjects();

     private:
      VolumeParams volumeParams;
      SamplerParams samplerParams;
      bool volumeNeedsUpdate{true};
      bool samplerNeedsUpdate{true};

      std::unique_ptr<testing::TestingVolume> testingVolume;
      std::unique_ptr<VKLVolume> vklVolume;
      box3f volumeBounds;
      unsigned int numAttributes{0};
      std::unique_ptr<VKLSampler> vklSampler;
    };

    struct Scene
    {
      Scheduler scheduler;
      Volume volume;
      Versioned<RendererParams> rendererParams;
      Versioned<ArcballCamera> camera;

      // General application parameters. These cannot be changed
      // through the GUI.
      bool disableVSync{false};
      bool interactive{true};
      bool printStats{false};
      unsigned batchModeSpp{1};
      std::vector<std::string> rendererTypes;

      // Returns false if the application should terminate.
      bool parseCommandLine(const std::list<std::string> &args);

      const std::vector<std::string> &supportedRendererTypes() const;
      bool validateRendererType(const std::string &type) const;
      std::unique_ptr<Renderer> createRenderer(const std::string &type);
    };

  }  // namespace examples
}  // namespace openvkl
