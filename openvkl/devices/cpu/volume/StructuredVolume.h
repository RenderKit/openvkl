// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../common/temporal_data.h"
#include "GridAccelerator_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "Volume.h"
#include "openvkl/VKLFilter.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    struct StructuredVolume : public Volume<W>
    {
      ~StructuredVolume();

      virtual void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      unsigned int getNumAttributes() const override;

      range1f getValueRange() const override;

      VKLFilter getFilter() const
      {
        return filter;
      }

      VKLFilter getGradientFilter() const
      {
        return gradientFilter;
      }

     protected:
      void buildAccelerator();

      range1f valueRange{empty};

      // parameters set in commit()
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      std::vector<Ref<const Data>> attributesData;
      int temporallyStructuredNumTimesteps;
      Ref<const Data> temporallyUnstructuredIndices;
      Ref<const DataT<float>> temporallyUnstructuredTimes;
      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};
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
      dimensions  = this->template getParam<vec3i>("dimensions");
      gridOrigin  = this->template getParam<vec3f>("gridOrigin", vec3f(0.f));
      gridSpacing = this->template getParam<vec3f>("gridSpacing", vec3f(1.f));

      attributesData.clear();

      if (this->template hasParamDataT<Data *>("data")) {
        // multiple attributes provided through VKLData array
        Ref<const DataT<Data *>> data =
            this->template getParamDataT<Data *>("data");

        for (const auto &d : *data) {
          attributesData.push_back(d);
        }
      } else if (this->template getParam<Data *>("data", nullptr)) {
        // single attribute provided through single VKLData object
        attributesData.push_back(this->template getParam<Data *>("data"));
      } else {
        throw std::runtime_error(this->toString() +
                                 ": missing required 'data' parameter");
      }

      // motion blur data provided by user through 2 VKLData arrays
      temporallyStructuredNumTimesteps =
          this->template getParam<int>("temporallyStructuredNumTimesteps", 0);

      temporallyUnstructuredIndices = this->template getParam<Data *>(
          "temporallyUnstructuredIndices", nullptr);
      temporallyUnstructuredTimes = this->template getParamDataT<float>(
          "temporallyUnstructuredTimes", nullptr);

      filter = (VKLFilter)this->template getParam<int>("filter", filter);
      gradientFilter =
          (VKLFilter)this->template getParam<int>("gradientFilter", filter);

      // validate type of each provided attribute; size validated depending on
      // temporal configuration
      const std::vector<VKLDataType> supportedDataTypes{
          VKL_UCHAR, VKL_SHORT, VKL_USHORT, VKL_HALF, VKL_FLOAT, VKL_DOUBLE};

      for (int i = 0; i < attributesData.size(); i++) {
        if (std::find(supportedDataTypes.begin(),
                      supportedDataTypes.end(),
                      attributesData[i]->dataType) ==
            supportedDataTypes.end()) {
          throw std::runtime_error(
              this->toString() + ": unsupported data element type (attribute " +
              std::to_string(i) + ") for 'data' parameter");
        }
      }

      const uint64_t expectedNumVoxels = this->dimensions.long_product();

      VKLTemporalFormat temporalFormat = VKL_TEMPORAL_FORMAT_CONSTANT;
      if (temporallyStructuredNumTimesteps > 0)
        temporalFormat = VKL_TEMPORAL_FORMAT_STRUCTURED;
      else if (temporallyUnstructuredIndices)
        temporalFormat = VKL_TEMPORAL_FORMAT_UNSTRUCTURED;

      const uint64_t expectedNumDataElements =
          verifyTemporalData(this->device.ptr,
                             expectedNumVoxels,
                             temporalFormat,
                             temporallyStructuredNumTimesteps,
                             temporallyUnstructuredIndices.ptr,
                             temporallyUnstructuredTimes.ptr);

      for (int i = 0; i < attributesData.size(); ++i) {
        if (attributesData[i]->numItems != expectedNumDataElements) {
          runtimeError("attribute ",
                       i,
                       " has ",
                       attributesData[i]->numItems,
                       " elements, but expected ",
                       expectedNumDataElements);
        }
      }
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
    inline unsigned int StructuredVolume<W>::getNumAttributes() const
    {
      return attributesData.size();
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

  }  // namespace cpu_device
}  // namespace openvkl
