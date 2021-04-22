// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../api/Device.h"
#include "../common/Data.h"
#include "../common/logging.h"
#include "runtime_error.h"

namespace openvkl {
  namespace cpu_device {
    inline uint64_t verifyTemporallyStructuredData(
        uint64_t expectedNumVoxels, int temporallyStructuredNumTimesteps)
    {
      if (temporallyStructuredNumTimesteps <= 0) {
        runtimeError(
            "Temporal format is VKL_TEMPORAL_FORMAT_STRUCTURED "
            "but temporallyStructuredNumTimesteps is not greater than 0.");
      }

      return expectedNumVoxels * temporallyStructuredNumTimesteps;
    }

    inline uint64_t verifyTemporallyUnstructuredData(
        Device *device,
        uint64_t expectedNumVoxels,
        const Data *temporallyUnstructuredIndices,
        const Data *temporallyUnstructuredTimes)
    {
      if (!temporallyUnstructuredIndices) {
        runtimeError(
            "temporal format is VKL_TEMPORAL_FORMAT_UNSTRUCTURED "
            "but temporallyUnstructuredIndices is not specified.");
      }

      if (!temporallyUnstructuredTimes) {
        runtimeError(
            "temporal format is VKL_TEMPORAL_FORMAT_UNSTRUCTURED "
            "but temporallyUnstructuredTimes is not specified.");
      }

      if (temporallyUnstructuredIndices->size() < (expectedNumVoxels + 1)) {
        runtimeError("temporallyUnstructuredIndices is too small.");
      }

      const bool require64BitIndices =
          expectedNumVoxels >= uint64_t(std::numeric_limits<uint32_t>::max());

      if (require64BitIndices &&
          temporallyUnstructuredIndices->dataType != VKL_ULONG) {
        throw std::runtime_error(
            "temporallyUnstructuredIndices must be VKL_ULONG due to "
            "attribute data size");
      }

      if (!require64BitIndices &&
          temporallyUnstructuredIndices->dataType == VKL_ULONG) {
        LogMessageStream(device, VKL_LOG_WARNING)
            << "WARNING: temporallyUnstructuredIndices is VKL_ULONG when "
               "VKL_UINT is sufficient and may be more performant";
      }

      if (temporallyUnstructuredIndices->dataType != VKL_UINT &&
          temporallyUnstructuredIndices->dataType != VKL_ULONG) {
        runtimeError(
            "temporallyUnstructuredIndices must have type VKL_UINT or "
            "VKL_ULONG");
      }

      if (temporallyUnstructuredTimes->dataType != VKL_FLOAT) {
        runtimeError("temporallyUnstructuredIndices must have type VKL_FLOAT");
      }

      uint64_t indexBegin = 0;
      uint64_t indexEnd   = 0;
      for (uint64_t i = 0; i < expectedNumVoxels; ++i) {
        if (temporallyUnstructuredIndices->dataType == VKL_UINT) {
          indexBegin =
              temporallyUnstructuredIndices->template as<uint32_t>()[i];
          indexEnd =
              temporallyUnstructuredIndices->template as<uint32_t>()[i + 1];
        } else if (temporallyUnstructuredIndices->dataType == VKL_ULONG) {
          indexBegin =
              temporallyUnstructuredIndices->template as<uint64_t>()[i];
          indexEnd =
              temporallyUnstructuredIndices->template as<uint64_t>()[i + 1];
        }

        if (!(indexBegin < indexEnd)) {
          runtimeError(
              "termporallyUnstructuredIndices must increase strictly "
              "monotonically.");
        }

        if (!(indexEnd <= temporallyUnstructuredTimes->size())) {
          runtimeError(
              "Values in temporallyUnstructuredIndices are out of "
              "bounds in temporallyUnstructredTimes.");
        }

        for (uint64_t ti = indexBegin + 1; ti < indexEnd; ++ti) {
          if (temporallyUnstructuredTimes->template as<float>()[ti - 1] >=
              temporallyUnstructuredTimes->template as<float>()[ti]) {
            runtimeError(
                "temporallyUnstructuredTimes must increase strictly "
                " monotonically for every voxel.");
          }
        }

        if (temporallyUnstructuredTimes->template as<float>()[indexBegin] <
                0.f ||
            temporallyUnstructuredTimes->template as<float>()[indexBegin] >
                1.f ||
            temporallyUnstructuredTimes->template as<float>()[indexEnd - 1] <
                0.f ||
            temporallyUnstructuredTimes->template as<float>()[indexEnd - 1] >
                1.f) {
          runtimeError(
              "Values in temporallyUnstructuredTimes must be bounded "
              "by 0.0 and 1.0 for every voxel.");
        }
      }

      return indexEnd;
    }

    /*
     * Returns the expected number of voxels in each attribute array
     * based on the given temporal format.
     */
    inline uint64_t verifyTemporalData(
        Device *device,
        uint64_t expectedNumVoxels,
        VKLTemporalFormat temporalFormat,
        const int temporallyStructuredNumTimesteps,
        const Data *temporallyUnstructuredIndices,
        const Data *temporallyUnstructuredTimes)
    {
      switch (temporalFormat) {
      case VKL_TEMPORAL_FORMAT_CONSTANT: {
        if (temporallyStructuredNumTimesteps != 0) {
          runtimeError(
              "Temporally constant volumes should not have "
              "temporallyStructuredNumTimesteps provided");
        }
        if (temporallyUnstructuredIndices || temporallyUnstructuredTimes) {
          runtimeError(
              "Temporally constant volumes should not have "
              "temporallyUnstructuredIndices or temporallyUnstructuredTimes "
              "provided");
        }
        return expectedNumVoxels;
      }
      case VKL_TEMPORAL_FORMAT_STRUCTURED: {
        if (temporallyUnstructuredIndices || temporallyUnstructuredTimes) {
          runtimeError(
              "Temporally structured volumes should not have "
              "temporallyUnstructuredIndices or temporallyUnstructuredTimes "
              "provided");
        }
        return verifyTemporallyStructuredData(expectedNumVoxels,
                                              temporallyStructuredNumTimesteps);
      }
      case VKL_TEMPORAL_FORMAT_UNSTRUCTURED: {
        if (temporallyStructuredNumTimesteps != 0) {
          runtimeError(
              "Temporally unstructured volumes should not have "
              "temporallyStructuredNumTimesteps provided");
        }
        return verifyTemporallyUnstructuredData(device, 
                                                expectedNumVoxels,
                                                temporallyUnstructuredIndices,
                                                temporallyUnstructuredTimes);
      }
      default:
        runtimeError("Invalid temporal format specified.");
      }

      return 0;
    }

  }  // namespace cpu_device
}  // namespace openvkl
