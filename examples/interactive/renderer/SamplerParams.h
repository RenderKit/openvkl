// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/openvkl.h>
#include <openvkl/vdb.h>
#include <list>
#include <string>
#include <vector>
#include <stdexcept>

namespace openvkl {
  namespace examples {

    struct VolumeParams;

    inline std::string filterToString(VKLFilter filter)
    {
      switch (filter) {
      case VKL_FILTER_NEAREST:
        return "nearest";
      case VKL_FILTER_LINEAR:
        return "trilinear";
      case VKL_FILTER_CUBIC:
        return "tricubic";
      default:
        break;
      }
      return "";
    }

    inline VKLFilter stringToFilter(const std::string &filterArg)
    {
      if (filterArg == "trilinear")
        return VKL_FILTER_LINEAR;
      else if (filterArg == "tricubic")
        return VKL_FILTER_CUBIC;
      else if (filterArg == "nearest")
        return VKL_FILTER_NEAREST;
      else
        throw std::runtime_error("unsupported filter specified: " + filterArg);
      return VKL_FILTER_LINEAR;
    }


    struct SamplerParams
    {
      int maxSamplingDepth = VKL_VDB_NUM_LEVELS - 1;
      VKLFilter filter{VKL_FILTER_LINEAR};
      VKLFilter gradientFilter{VKL_FILTER_LINEAR};

      void parseCommandLine(std::list<std::string> &args,
          const VolumeParams &volumeParams);
      static void usage();
      void updateSampler(VKLSampler sampler) const;

      static const std::vector<VKLFilter> &supportedFilters(
          const std::string &volumeType)
      {
        if (volumeType == "structuredRegular"
         || volumeType == "structuredSpherical"
         || volumeType == "vdb")
        {
          static const std::vector<VKLFilter> types = {
            VKL_FILTER_NEAREST,
            VKL_FILTER_LINEAR,
            VKL_FILTER_CUBIC
          };
          return types;
        }

        static const std::vector<VKLFilter> types;
        return types;
      }

    private:
      void validate(const VolumeParams &volumeParams) const;
    };

  }  // namespace examples
}  // namespace openvkl
