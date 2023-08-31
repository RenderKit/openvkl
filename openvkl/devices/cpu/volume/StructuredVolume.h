// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../common/Data.h"
#include "../common/export_util.h"
#include "../common/math.h"
#include "../common/temporal_data_verification.h"
#include "Volume.h"
#include "openvkl/VKLFilter.h"
#include "rkcommon/tasking/parallel_for.h"

#include "GridAccelerator.h"
#include "GridAcceleratorShared.h"
#include "StructuredVolumeShared.h"
#include "openvkl/devices/common/StructShared.h"

namespace openvkl {
  namespace cpu_device {

    void destructAttributesStorage(ispc::SharedStructuredVolume *self);

    void constructAttributesStorage(ispc::SharedStructuredVolume *self,
                                    const uint32 numAttributes);
    void SharedStructuredVolume_Destructor(void *_self);

    void SharedStructuredVolume_Constructor(void *_self);

    bool SharedStructuredVolume_set(
        void *_self,
        const uint32 numAttributes,
        const ispc::Data1D **attributesData,
        const uint32 temporallyStructuredNumTimesteps,
        const ispc::Data1D *temporallyUnstructuredIndices,
        const ispc::Data1D *temporallyUnstructuredTimes,
        const vec3i &dimensions,
        const ispc::SharedStructuredVolumeGridType gridType,
        const vec3f &gridOrigin,
        const vec3f &gridSpacing,
        const ispc::VKLFilter filter);

    template <int W>
    struct StructuredVolume
        : public AddStructShared<Volume<W>, ispc::SharedStructuredVolume>
    {
      StructuredVolume(Device *device)
          : AddStructShared<Volume<W>, ispc::SharedStructuredVolume>(device){};
      ~StructuredVolume();

      virtual void commit() override;

      Sampler<W> *newSampler() override;

      box3f getBoundingBox() const override;

      unsigned int getNumAttributes() const override;

      range1f getValueRange(unsigned int attributeIndex) const override;

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

      // during acceleration structure builds, we need host-accessible data.
      // these functions ensure we have host-accessible data during build, but
      // revert to existing (e.g. device-only) data afterward.
      void preHostBuild();
      void postHostBuild();

      // temporary state associated with [pre,post]HostBuild()
      std::vector<rkcommon::memory::Ref<const Data>> host_attributesData;
      std::unique_ptr<BufferShared<ispc::Data1D>> host_deviceAttributesData;

      // parameters set in commit()
      vec3i dimensions;
      vec3f gridOrigin;
      vec3f gridSpacing;
      std::vector<rkcommon::memory::Ref<const Data>> attributesData;
      std::unique_ptr<BufferShared<ispc::Data1D>> deviceAttributesData;
      int temporallyStructuredNumTimesteps;
      rkcommon::memory::Ref<const Data> temporallyUnstructuredIndices;
      rkcommon::memory::Ref<const DataT<float>> temporallyUnstructuredTimes;
      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};
      rkcommon::memory::Ref<const DataT<float>> background;

     private:
      ISPCRTMemoryView m_accelerator = nullptr;
      std::unique_ptr<BufferShared<range1f>> valueRanges;
      std::unique_ptr<BufferShared<box1f>> cellValueRanges;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    StructuredVolume<W>::~StructuredVolume()
    {
      attributesData.clear();
      if (this->SharedStructInitialized) {
        SharedStructuredVolume_Destructor(this->getSh());
      }
      if (m_accelerator)
        BufferSharedDelete(m_accelerator);
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
        rkcommon::memory::Ref<const DataT<Data *>> data =
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
      size_t nA                    = attributesData.size();
      this->getSh()->numAttributes = nA;
      deviceAttributesData =
          make_buffer_shared_unique<ispc::Data1D>(this->getDevice(), nA);
      this->getSh()->attributesData = deviceAttributesData->sharedPtr();
      for (size_t i = 0; i < nA; i++) {
        this->getSh()->attributesData[i] = *(ispc(attributesData[i]));
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

      background = this->template getParamDataT<float>(
          "background", attributesData.size(), VKL_BACKGROUND_UNDEFINED);

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
      ispc::box3f bb;
      CALL_ISPC(SharedStructuredVolume_getBoundingBox, this->getSh(), bb);

      return box3f(vec3f(bb.lower.x, bb.lower.y, bb.lower.z),
                   vec3f(bb.upper.x, bb.upper.y, bb.upper.z));
    }

    template <int W>
    inline unsigned int StructuredVolume<W>::getNumAttributes() const
    {
      return attributesData.size();
    }

    template <int W>
    inline range1f StructuredVolume<W>::getValueRange(
        unsigned int attributeIndex) const
    {
      throwOnIllegalAttributeIndex(this, attributeIndex);
      return valueRanges->sharedPtr()[attributeIndex];
    }

    template <int W>
    inline void StructuredVolume<W>::buildAccelerator()
    {
      preHostBuild();

      if (m_accelerator)
        BufferSharedDelete(m_accelerator);
      m_accelerator =
          BufferSharedCreate(this->getDevice(), sizeof(ispc::GridAccelerator));
      auto ga =
          static_cast<ispc::GridAccelerator *>(ispcrtHostPtr(m_accelerator));

      // cells per dimension after padding out the volume dimensions to the
      // nearest cell
      vec3i cellsPerDimension =
          vec3i((this->getSh()->dimensions.x + CELL_WIDTH - 1) / CELL_WIDTH,
                (this->getSh()->dimensions.y + CELL_WIDTH - 1) / CELL_WIDTH,
                (this->getSh()->dimensions.z + CELL_WIDTH - 1) / CELL_WIDTH);

      // bricks per dimension after padding out the cell dimensions to the
      // nearest brick
      ga->bricksPerDimension =
          (cellsPerDimension + BRICK_WIDTH - 1) / BRICK_WIDTH;

      ga->cellCount = ga->bricksPerDimension.x * ga->bricksPerDimension.y *
                      ga->bricksPerDimension.z * BRICK_CELL_COUNT;

      cellValueRanges = make_buffer_shared_unique<box1f>(
          this->getDevice(), ga->cellCount * this->getSh()->numAttributes);
      ga->cellValueRanges =
          (ga->cellCount > 0) ? cellValueRanges->sharedPtr() : nullptr;

      ga->volume                 = this->getSh();
      this->getSh()->accelerator = ga;

      void *accelerator = ga;
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

      valueRanges = make_buffer_shared_unique<range1f>(this->getDevice(),
                                                       getNumAttributes());

      for (unsigned int a = 0; a < getNumAttributes(); a++) {
        CALL_ISPC(GridAccelerator_computeValueRange,
                  accelerator,
                  a,
                  (valueRanges->sharedPtr() + a)->lower,
                  (valueRanges->sharedPtr() + a)->upper);
      }

      postHostBuild();
    }

    template <int W>
    inline void StructuredVolume<W>::preHostBuild()
    {
      host_deviceAttributesData = make_buffer_shared_unique<ispc::Data1D>(
          this->getDevice(), attributesData.size());

      for (auto &data : attributesData) {
        host_attributesData.push_back(data->hostAccessible());
      }

      this->getSh()->attributesData = host_deviceAttributesData->sharedPtr();
      for (size_t i = 0; i < attributesData.size(); i++) {
        this->getSh()->attributesData[i] = *(ispc(host_attributesData[i]));
      }
    }

    template <int W>
    inline void StructuredVolume<W>::postHostBuild()
    {
      this->getSh()->attributesData = deviceAttributesData->sharedPtr();
      for (size_t i = 0; i < attributesData.size(); i++) {
        this->getSh()->attributesData[i] = *(ispc(attributesData[i]));
      }

      host_attributesData.clear();
      host_deviceAttributesData.reset();
    }

  }  // namespace cpu_device
}  // namespace openvkl
