// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SamplerParams.h"
#include "CommandLine.h"
#include "VolumeParams.h"

namespace openvkl {
  namespace examples {

    void SamplerParams::parseCommandLine(std::list<std::string> &args,
                                         const VolumeParams &volumeParams)
    {
      for (auto it = args.begin(); it != args.end();) {
        const std::string arg = *it;

        if (arg == "-filter") {
          const std::string filterArg = cmd::consume_1<std::string>(args, it);
          filter                      = stringToFilter(filterArg);
          gradientFilter              = filter;
        } else {
          ++it;
        }
      }

      validate(volumeParams);
    }

    void SamplerParams::validate(const VolumeParams &volumeParams) const
    {
      const std::vector<VKLFilter> &supFilters =
          supportedFilters(volumeParams.volumeType);
      if (!supFilters.empty()) {
        if (std::find(supFilters.begin(), supFilters.end(), filter) ==
            supFilters.end()) {
          throw std::runtime_error("invalid filter for volume type " +
                                   volumeParams.volumeType);
        }
      }
    }

    void SamplerParams::usage()
    {
      std::cerr << "\t-filter FILTER (vdb, structuredRegular and "
                   "structuredSpherical)\n";
    }

    void SamplerParams::updateSampler(VKLSampler sampler) const
    {
      vklSetInt2(sampler, "filter", filter);
      vklSetInt2(sampler, "gradientFilter", gradientFilter);
      vklSetInt2(sampler, "maxSamplingDepth", maxSamplingDepth);
      vklCommit2(sampler);
    }

  }  // namespace examples
}  // namespace openvkl
