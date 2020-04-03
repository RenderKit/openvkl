// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "GridAccelerator_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "Volume.h"
#include "ospcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct StructuredVolume : public Volume<W>
    {
      ~StructuredVolume();

      virtual void commit() override;

      void computeSample(const vvec3fn<1> &objectCoordinates,
                         vfloatn<1> &samples) const override;

      void computeSampleV(const vintn<W> &valid,
                          const vvec3fn<W> &objectCoordinates,
                          vfloatn<W> &samples) const override;

      void computeGradientV(const vintn<W> &valid,
                            const vvec3fn<W> &objectCoordinates,
                            vvec3fn<W> &gradients) const override;

      box3f getBoundingBox() const override;

      range1f getValueRange() const override;

     protected:
      void buildAccelerator();

      range1f valueRange{empty};

      // parameters set in commit()
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      Data *voxelData{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    StructuredVolume<W>::~StructuredVolume()
    {
      if (this->ispcEquivalent) {
        CALL_ISPC(SharedStructuredVolume_Destructor, this->ispcEquivalent);
      }
    }

    template <int W>
    inline void StructuredVolume<W>::commit()
    {
      dimensions  = this->template getParam<vec3i>("dimensions", vec3i(128));
      gridOrigin  = this->template getParam<vec3f>("gridOrigin", vec3f(0.f));
      gridSpacing = this->template getParam<vec3f>("gridSpacing", vec3f(1.f));

      voxelData = (Data *)this->template getParam<ManagedObject::VKL_PTR>(
          "data", nullptr);

      if (!voxelData) {
        throw std::runtime_error("no data set on volume");
      }

      if (voxelData->size() != this->dimensions.long_product()) {
        throw std::runtime_error(
            "incorrect data size for provided volume dimensions");
      }
    }

    template <int W>
    inline void StructuredVolume<W>::computeSample(
        const vvec3fn<1> &objectCoordinates, vfloatn<1> &samples) const
    {
      CALL_ISPC(SharedStructuredVolume_sample_uniform_export,
                this->ispcEquivalent,
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void StructuredVolume<W>::computeSampleV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vfloatn<W> &samples) const
    {
      CALL_ISPC(SharedStructuredVolume_sample_export,
                static_cast<const int *>(valid),
                this->ispcEquivalent,
                &objectCoordinates,
                &samples);
    }

    template <int W>
    inline void StructuredVolume<W>::computeGradientV(
        const vintn<W> &valid,
        const vvec3fn<W> &objectCoordinates,
        vvec3fn<W> &gradients) const
    {
      CALL_ISPC(SharedStructuredVolume_gradient_export,
                static_cast<const int *>(valid),
                this->ispcEquivalent,
                &objectCoordinates,
                &gradients);
    }

    template <int W>
    inline box3f StructuredVolume<W>::getBoundingBox() const
    {
      ispc::box3f bb = CALL_ISPC(SharedStructuredVolume_getBoundingBox,
                                 this->ispcEquivalent);

      return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                   vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
    }

    template <int W>
    inline range1f StructuredVolume<W>::getValueRange() const
    {
      return valueRange;
    }

    template <int W>
    inline void StructuredVolume<W>::buildAccelerator()
    {
      void *accelerator = CALL_ISPC(SharedStructuredVolume_createAccelerator,
                                    this->ispcEquivalent);

      vec3i bricksPerDimension;
      bricksPerDimension.x =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_x, accelerator);
      bricksPerDimension.y =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_y, accelerator);
      bricksPerDimension.z =
          CALL_ISPC(GridAccelerator_getBricksPerDimension_z, accelerator);

      const int numTasks =
          bricksPerDimension.x * bricksPerDimension.y * bricksPerDimension.z;
      tasking::parallel_for(numTasks, [&](int taskIndex) {
        CALL_ISPC(GridAccelerator_build, accelerator, taskIndex);
      });

      CALL_ISPC(GridAccelerator_computeValueRange,
                accelerator,
                valueRange.lower,
                valueRange.upper);
    }

  }  // namespace ispc_driver
}  // namespace openvkl
