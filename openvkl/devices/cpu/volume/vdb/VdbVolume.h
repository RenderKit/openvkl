// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/vdb.h>
#include <memory>
#include "../../common/Allocator.h"
#include "../../observer/ObserverRegistry.h"
#include "../Volume.h"
#include "../common/Data.h"
#include "VdbGrid.h"
#include "VdbIterator.h"
#include "VdbVolume_ispc.h"
#include "rkcommon/containers/aligned_allocator.h"
#include "rkcommon/memory/RefCount.h"

using namespace rkcommon::memory;

namespace openvkl {
  namespace cpu_device {

    template <typename T>
    using AlignedVector16 =
        std::vector<T, rkcommon::containers::aligned_allocator<T, 16>>;

    template <int W>
    struct VdbVolume : public Volume<W>
    {
      VdbVolume(const VdbVolume &) = delete;
      VdbVolume &operator=(const VdbVolume &) = delete;
      VdbVolume(VdbVolume &&other)            = delete;
      VdbVolume &operator=(VdbVolume &&other) = delete;

      VdbVolume();
      ~VdbVolume() override;

      /*
       * Return "openvkl::VdbVolume".
       */
      std::string toString() const override;

      /*
       * Commit the volume after setup, but before rendering.
       * Will build the main tree structure from all leaves
       * provided as parameters.
       */
      void commit() override;

      /*
       * Obtain the volume bounding box.
       */
      box3f getBoundingBox() const override
      {
        return bounds;
      }

      /*
       * Get the number of attributes in this volume.
       */
      unsigned int getNumAttributes() const override
      {
        return grid ? grid->numAttributes : 0;
      }

      /*
       * Get the minimum and maximum value in this volume.
       */
      range1f getValueRange(unsigned int attributeIndex) const override
      {
        throwOnIllegalAttributeIndex(this, attributeIndex);
        return valueRanges[attributeIndex];
      }

      const VdbGrid *getGrid() const
      {
        return grid;
      }

      Observer<W> *newObserver(const char *type) override;
      Sampler<W> *newSampler() override;

      VKLFilter getFilter() const
      {
        return filter;
      }

      VKLFilter getGradientFilter() const
      {
        return gradientFilter;
      }

      uint32_t getMaxSamplingDepth() const
      {
        return maxSamplingDepth;
      }

      uint32_t getMaxIteratorDepth() const
      {
        return maxIteratorDepth;
      }

     private:
      void cleanup();

     private:
      box3f bounds;
      std::string name;
      std::vector<range1f> valueRanges;
      Ref<const DataT<Data *>> leafData;
      Ref<const DataT<uint32_t>> leafFormat;
      Ref<const DataT<uint32_t>> leafTemporalFormat;
      Ref<const DataT<int>> leafStructuredTimesteps;
      Ref<const DataT<Data *>> leafUnstructuredIndices;
      Ref<const DataT<Data *>> leafUnstructuredTimes;
      VdbGrid *grid{nullptr};
      Allocator allocator;

      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};
      uint32_t maxSamplingDepth{VKL_VDB_NUM_LEVELS - 1};
      uint32_t maxIteratorDepth{VKL_VDB_NUM_LEVELS - 2};
    };

  }  // namespace cpu_device
}  // namespace openvkl
