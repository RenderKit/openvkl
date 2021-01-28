// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "GridAccelerator_ispc.h"
#include "SharedStructuredVolume_ispc.h"
#include "Volume.h"
#include "openvkl/VKLFilter.h"
#include "rkcommon/tasking/parallel_for.h"

namespace openvkl {
  namespace ispc_driver {

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
          VKL_UCHAR, VKL_SHORT, VKL_USHORT, VKL_FLOAT, VKL_DOUBLE};

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

      // validate temporal configuration and attribute data sizes

      if (temporallyStructuredNumTimesteps) {
        // temporally structured

        if (!(temporallyStructuredNumTimesteps > 1)) {
          throw std::runtime_error(
              "temporallyStructuredNumTimesteps must be > 1");
        }

        size_t expectedNumDataItems = size_t(temporallyStructuredNumTimesteps) *
                                      this->dimensions.long_product();

        for (int i = 0; i < attributesData.size(); i++) {
          if (attributesData[i]->numItems != expectedNumDataItems) {
            throw std::runtime_error("temporally structured attribute " +
                                     std::to_string(i) +
                                     " has improperly sized data");
          }
        }

        if (temporallyUnstructuredIndices &&
            temporallyUnstructuredIndices->size() > 0) {
          throw std::runtime_error(
              "temporally structured volume should not have "
              "temporallyUnstructuredIndices provided");
        }

        if (temporallyUnstructuredTimes &&
            temporallyUnstructuredTimes->size() > 0) {
          throw std::runtime_error(
              "temporally structured volume should not have "
              "temporallyUnstructuredTimes provided");
        }

      } else if (temporallyUnstructuredIndices &&
                 temporallyUnstructuredIndices->size() > 0) {
        // temporally unstructured

        bool require64BitIndices = attributesData[0]->numItems >=
                                   size_t(std::numeric_limits<uint32_t>::max());

        if (require64BitIndices &&
            temporallyUnstructuredIndices->dataType != VKL_ULONG) {
          throw std::runtime_error(
              "temporallyUnstructuredIndices must be VKL_ULONG due to "
              "attribute data size");
        }

        if (!require64BitIndices &&
            temporallyUnstructuredIndices->dataType == VKL_ULONG) {
          postLogMessage(VKL_LOG_WARNING)
              << "WARNING: temporallyUnstructuredIndices is VKL_ULONG when "
                 "VKL_UINT is sufficient and may be more performant";
        }

        if (temporallyUnstructuredIndices->dataType != VKL_UINT &&
            temporallyUnstructuredIndices->dataType != VKL_ULONG) {
          throw std::runtime_error(
              "temporallyUnstructuredIndices must be VKL_UINT or VKL_ULONG");
        }

        if (temporallyUnstructuredIndices->size() !=
            this->dimensions.long_product() + 1) {
          throw std::runtime_error(
              "temporally unstructured volume has improperly sized "
              "temporallyUnstructuredIndices");
        }

        size_t expectedNumDataItems;

        if (temporallyUnstructuredIndices->dataType == VKL_UINT) {
          expectedNumDataItems =
              size_t(temporallyUnstructuredIndices->template as<
                     uint32_t>()[temporallyUnstructuredIndices->size() - 1]);
        } else if (temporallyUnstructuredIndices->dataType == VKL_ULONG) {
          expectedNumDataItems =
              size_t(temporallyUnstructuredIndices->template as<
                     uint64_t>()[temporallyUnstructuredIndices->size() - 1]);
        }

        for (int i = 0; i < attributesData.size(); i++) {
          if (attributesData[i]->numItems != expectedNumDataItems) {
            throw std::runtime_error("temporally unstructured attribute " +
                                     std::to_string(i) +
                                     " has improperly sized data");
          }
        }

        if (!temporallyUnstructuredTimes ||
            temporallyUnstructuredTimes->size() != expectedNumDataItems) {
          throw std::runtime_error(
              "temporally unstructured volume has improperly sized "
              "temporallyUnstructuredTimes");
        }

        for (size_t i = 0; i < temporallyUnstructuredIndices->size() - 1; i++) {
          size_t timeBeginIndex;
          size_t timeEndIndex;

          if (temporallyUnstructuredIndices->dataType == VKL_UINT) {
            timeBeginIndex =
                temporallyUnstructuredIndices->template as<uint32_t>()[i];
            timeEndIndex =
                temporallyUnstructuredIndices->template as<uint32_t>()[i + 1] -
                1;
          } else if (temporallyUnstructuredIndices->dataType == VKL_ULONG) {
            timeBeginIndex =
                temporallyUnstructuredIndices->template as<uint64_t>()[i];
            timeEndIndex =
                temporallyUnstructuredIndices->template as<uint64_t>()[i + 1] -
                1;
          }

          if ((*temporallyUnstructuredTimes)[timeBeginIndex] < 0.f ||
              (*temporallyUnstructuredTimes)[timeBeginIndex] > 1.f ||
              (*temporallyUnstructuredTimes)[timeEndIndex] < 0.f ||
              (*temporallyUnstructuredTimes)[timeEndIndex] > 1.f) {
            throw std::runtime_error(
                "temporallyUnstructuredTimes values must be bounded by 0.0 and "
                "1.0 for every voxel");
          }

          for (size_t j = timeBeginIndex; j < timeEndIndex; j++) {
            if (!((*temporallyUnstructuredTimes)[j] <
                  (*temporallyUnstructuredTimes)[j + 1])) {
              throw std::runtime_error(
                  "temporallyUnstructuredTimes values must be strictly "
                  "monotonically increasing for every voxel");
            }
          }
        }
      }

      else {
        // no time configuration

        for (int i = 0; i < attributesData.size(); i++) {
          if (attributesData[i]->size() != this->dimensions.long_product()) {
            throw std::runtime_error("incorrect data size (attribute " +
                                     std::to_string(i) +
                                     ") for provided volume dimensions");
          }
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

  }  // namespace ispc_driver
}  // namespace openvkl
