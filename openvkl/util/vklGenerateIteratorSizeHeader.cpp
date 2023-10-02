// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <openvkl/openvkl.h>
#include <openvkl/device/openvkl.h>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <tuple>

using std::size_t;

template <int W>
std::pair<size_t, size_t> getMaxIteratorSizeIspc()
{
  vklInit();

  std::ostringstream os;
  os << "cpu_" << W;
  VKLDevice device = vklNewDevice(os.str().c_str());

  size_t maxIntervalSize = 0;
  size_t maxHitSize      = 0;

  if (device) {
    vklCommitDevice(device);

    for (const char *volumeType : {"amr",
                                   "structuredRegular",
                                   "structuredSpherical",
                                   "particle",
                                   "unstructured",
                                   "vdb"}) {
      VKLVolume volume   = vklNewVolume(device, volumeType);
      VKLSampler sampler = vklNewSampler(volume);
      VKLIntervalIteratorContext intervalContext =
          vklNewIntervalIteratorContext(sampler);
      VKLHitIteratorContext hitContext = vklNewHitIteratorContext(sampler);

      maxIntervalSize = std::max<size_t>(
          maxIntervalSize, vklGetIntervalIteratorSize(&intervalContext));
      maxHitSize =
          std::max<size_t>(maxHitSize, vklGetHitIteratorSize(&hitContext));

      vklRelease(hitContext);
      vklRelease(intervalContext);
      vklRelease(sampler);
      vklRelease(volume);
    }
  }

  vklReleaseDevice(device);

  return {maxIntervalSize, maxHitSize};
}

int main()
{
  std::size_t maxIntervalSize4            = 0;
  std::size_t maxHitSize4                 = 0;
  std::tie(maxIntervalSize4, maxHitSize4) = getMaxIteratorSizeIspc<4>();

  std::size_t maxIntervalSize8            = 0;
  std::size_t maxHitSize8                 = 0;
  std::tie(maxIntervalSize8, maxHitSize8) = getMaxIteratorSizeIspc<8>();

  std::size_t maxIntervalSize16             = 0;
  std::size_t maxHitSize16                  = 0;
  std::tie(maxIntervalSize16, maxHitSize16) = getMaxIteratorSizeIspc<16>();

  std::cout
      << "// Copyright 2020 Intel Corporation\n"
      << "// SPDX-License-Identifier: Apache-2.0\n"
      << "\n"
      << "#pragma once\n"
      << "\n"
      << "// Maximum iterator size over all supported volume and device\n"
      << "// types, and for each target SIMD width.\n"
      << "#define VKL_MAX_INTERVAL_ITERATOR_SIZE_4 " << maxIntervalSize4 << "\n"
      << "#define VKL_MAX_INTERVAL_ITERATOR_SIZE_8 " << maxIntervalSize8 << "\n"
      << "#define VKL_MAX_INTERVAL_ITERATOR_SIZE_16 " << maxIntervalSize16
      << "\n"
      << "\n"
      << "#if defined(TARGET_WIDTH) && (TARGET_WIDTH == 4)\n"
      << "  #define VKL_MAX_INTERVAL_ITERATOR_SIZE "
         "VKL_MAX_INTERVAL_ITERATOR_SIZE_4\n"
      << "#elif defined(TARGET_WIDTH) && (TARGET_WIDTH == 8)\n"
      << "  #define VKL_MAX_INTERVAL_ITERATOR_SIZE "
         "VKL_MAX_INTERVAL_ITERATOR_SIZE_8\n"
      << "#else\n"
      << "  #define VKL_MAX_INTERVAL_ITERATOR_SIZE "
         "VKL_MAX_INTERVAL_ITERATOR_SIZE_16\n"
      << "#endif\n"
      << "#define VKL_MAX_HIT_ITERATOR_SIZE_4 " << maxHitSize4 << "\n"
      << "#define VKL_MAX_HIT_ITERATOR_SIZE_8 " << maxHitSize8 << "\n"
      << "#define VKL_MAX_HIT_ITERATOR_SIZE_16 " << maxHitSize16 << "\n"
      << "\n"
      << "#if defined(TARGET_WIDTH) && (TARGET_WIDTH == 4)\n"
      << "  #define VKL_MAX_HIT_ITERATOR_SIZE VKL_MAX_HIT_ITERATOR_SIZE_4\n"
      << "#elif defined(TARGET_WIDTH) && (TARGET_WIDTH == 8)\n"
      << "  #define VKL_MAX_HIT_ITERATOR_SIZE VKL_MAX_HIT_ITERATOR_SIZE_8\n"
      << "#else\n"
      << "  #define VKL_MAX_HIT_ITERATOR_SIZE VKL_MAX_HIT_ITERATOR_SIZE_16\n"
      << "#endif\n"
      << std::flush;

  return 0;
}
