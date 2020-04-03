// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <openvkl/vdb.h>
#include <memory>
#include "../StructuredVolume.h"
#include "../common/Data.h"
#include "VdbGrid.h"
#include "VdbIterator.h"
#include "VdbVolume_ispc.h"
#include "ospcommon/memory/RefCount.h"

using namespace ospcommon::memory;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct VdbVolume : public Volume<W>
    {
      VdbVolume(const VdbVolume &) = delete;
      VdbVolume &operator=(const VdbVolume &) = delete;

      VdbVolume();
      VdbVolume(VdbVolume &&other);
      VdbVolume &operator=(VdbVolume &&other);
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
       * Sample the volume at the given coordinates.
       */
      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      /*
       * Scalar specialization.
       */
      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples) const override;

      /*
       * Compute the volume gradient at the given coordinates.
       * NOT IMPLEMENTED.
       */
      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override
      {
        THROW_NOT_IMPLEMENTED;
      }

      /*
       * Obtain the volume bounding box.
       */
      box3f getBoundingBox() const override
      {
        return bounds;
      }

      /*
       * Get the minimum and maximum value in this volume.
       */
      range1f getValueRange() const override
      {
        return valueRange;
      }

      const VdbGrid *getGrid() const
      {
        return grid;
      }

      VKLObserver newObserver(const char *type) override;

      void initIntervalIteratorV(
          const vintn<W> &valid,
          vVKLIntervalIteratorN<W> &iterator,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) override;

      void iterateIntervalV(const vintn<W> &valid,
                            vVKLIntervalIteratorN<W> &iterator,
                            vVKLIntervalN<W> &interval,
                            vintn<W> &result) override;

      void initHitIteratorV(const vintn<W> &valid,
                            vVKLHitIteratorN<W> &iterator,
                            const vvec3fn<W> &origin,
                            const vvec3fn<W> &direction,
                            const vrange1fn<W> &tRange,
                            const ValueSelector<W> *valueSelector) override;

      void iterateHitV(const vintn<W> &valid,
                       vVKLHitIteratorN<W> &iterator,
                       vVKLHitN<W> &hit,
                       vintn<W> &result) override;

     private:
      void cleanup();

     private:
      box3f bounds;
      std::string name;
      range1f valueRange;
      Ref<Data> dataData;
      VdbGrid *grid{nullptr};
      size_t bytesAllocated{0};
    };

  }  // namespace ispc_driver
}  // namespace openvkl
