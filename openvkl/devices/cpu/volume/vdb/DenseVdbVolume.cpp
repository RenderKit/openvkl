// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenseVdbVolume.h"
#include "../../common/runtime_error.h"
#include "../../common/temporal_data_verification.h"
#include "rkcommon/utility/multidim_index_sequence.h"

namespace openvkl {
  namespace cpu_device {

    template <int W>
    void DenseVdbVolume<W>::commit()
    {
      parseStructuredVolumeParameters();

      this->dense               = true;
      this->denseDimensions     = dimensions;
      this->denseData           = attributesData;
      this->denseTemporalFormat = temporalFormat;
      this->denseTemporallyStructuredNumTimesteps =
          temporallyStructuredNumTimesteps;
      this->denseTemporallyUnstructuredIndices = temporallyUnstructuredIndices;
      this->denseTemporallyUnstructuredTimes   = temporallyUnstructuredTimes;

      // for now, always assume vertex-centered representation to match legacy
      // structuredRegular data interpretation
      this->constantCellData = false;

      VdbVolume<W>::commit();
    }

    template <int W>
    void DenseVdbVolume<W>::initIndexSpaceTransforms()
    {
      AffineSpace3f i2o(one);

      // support indexToObject transformation (instead of gridOrigin,
      // gridSpacing) if provided
      if (this->template hasParamT<AffineSpace3f>("indexToObject") ||
          this->template hasParamDataT<float>("indexToObject")) {
        i2o = getParamAffineSpace3f(this, "indexToObject");
      } else {
        i2o.l = LinearSpace3f(vec3f(gridSpacing.x, 0.f, 0.f),
                              vec3f(0.f, gridSpacing.y, 0.f),
                              vec3f(0.f, 0.f, gridSpacing.z));
        i2o.p = vec3f(gridOrigin.x, gridOrigin.y, gridOrigin.z);
      }

      writeTransform(i2o, this->grid->indexToObject);

      AffineSpace3f o2i;
      o2i.l = i2o.l.inverse();
      o2i.p = -(o2i.l * i2o.p);
      writeTransform(o2i, this->grid->objectToIndex);
    }

    template <int W>
    void DenseVdbVolume<W>::initLeafNodeData()
    {
      this->numLeaves = ((dimensions + VKL_VDB_RES_LEAF - 1) / VKL_VDB_RES_LEAF)
                            .long_product();

      if (this->numLeaves == 0) {
        runtimeError("Vdb volumes must have at least one leaf node.");
      }

      this->leafLevel = new DataT<uint32_t>(
          this->numLeaves, static_cast<uint32_t>(VKL_VDB_NUM_LEVELS - 1));
      this->leafLevel->refDec();

      // generate leafOrigins data
      const multidim_index_sequence<3> mis{
          vec3i((dimensions + VKL_VDB_RES_LEAF - 1) / VKL_VDB_RES_LEAF)};

      std::vector<vec3i> leafOrigins;

      for (const auto &ijk : mis) {
        leafOrigins.push_back(ijk * VKL_VDB_RES_LEAF);
      }

      assert(leafOrigins.size() == this->numLeaves);

      // and leafOrigin data array
      auto leafOriginTemp = new Data(leafOrigins.size(),
                                     VKL_VEC3I,
                                     leafOrigins.data(),
                                     VKL_DATA_DEFAULT,
                                     sizeof(vec3i));
      this->leafOrigin    = &leafOriginTemp->as<vec3i>();
      leafOriginTemp->refDec();

      // finally, leafFormat and leafTemporalFormat. these go mostly unused in
      // the VDB dense path for traversal / sampling, but are encoded in the
      // leaf voxel data for the acceleration structure, which is still used in
      // iteration and other places.
      this->leafFormat = new DataT<uint32_t>(
          this->numLeaves, static_cast<uint32_t>(VKL_FORMAT_DENSE_ZYX));
      this->leafFormat->refDec();

      this->leafTemporalFormat = new DataT<uint32_t>(
          this->numLeaves, static_cast<uint32_t>(temporalFormat));
      this->leafTemporalFormat->refDec();
    }

    template <int W>
    void DenseVdbVolume<W>::parseStructuredVolumeParameters()
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

      temporalFormat = VKL_TEMPORAL_FORMAT_CONSTANT;
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

    VKL_REGISTER_VOLUME(DenseVdbVolume<VKL_TARGET_WIDTH>,
                        CONCAT1(internal_structuredRegular_, VKL_TARGET_WIDTH))

  }  // namespace cpu_device
}  // namespace openvkl
