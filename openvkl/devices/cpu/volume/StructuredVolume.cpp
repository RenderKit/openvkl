// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "StructuredVolume.h"

#include "StructuredSampler.h"

namespace openvkl {
  namespace cpu_device {
  ///////////////////////////////////////////////////////////////////////////////
  // Helper functions for handling multiple attributes //////////////////////////
  ///////////////////////////////////////////////////////////////////////////////

  void destructComputeFunctions(ispc::SharedStructuredVolume * self)
  {
    if (self->computeVoxelRange) {
      delete[] self->computeVoxelRange;
      self->computeVoxelRange = nullptr;
    }

    if (self->computeSamplesInner_varying) {
      delete[] self->computeSamplesInner_varying;
      self->computeSamplesInner_varying = nullptr;
    }

    if (self->computeSamplesInner_uniform) {
      delete[] self->computeSamplesInner_uniform;
      self->computeSamplesInner_uniform = nullptr;
    }
  }

  void constructComputeFunctions(ispc::SharedStructuredVolume * self,
                                 const  uint32 numAttributes)
  {
    self->computeVoxelRange =
         new  ispc::ComputeVoxelRangeFunc[numAttributes];

    self->computeSamplesInner_varying =
         new  ispc::ComputeSampleInnerVaryingFunc[numAttributes];

    self->computeSamplesInner_uniform =
         new  ispc::ComputeSampleInnerUniformFunc[numAttributes];
  }

  void SharedStructuredVolume_Destructor(void * _self)
  {
     ispc::SharedStructuredVolume * self =
        ( ispc::SharedStructuredVolume * ) _self;

    destructComputeFunctions(self);
  }

  void SharedStructuredVolume_Constructor(
                            void * _self)
  {
     ispc::SharedStructuredVolume * self =
        ( ispc::SharedStructuredVolume * ) _self;

    memset(self, 0, sizeof( ispc::SharedStructuredVolume));
  }

  bool hasStructuredTimeData(
      const ispc::SharedStructuredVolume * self)
  {
    return self->temporallyStructuredNumTimesteps > 1;
  }

  bool hasUnstructuredTimeData(
      const ispc::SharedStructuredVolume * self)
  {
    return (self->temporallyUnstructuredIndices.addr != nullptr) &&
           (self->temporallyUnstructuredIndices.numItems > 0);
  }

  void computeStructuredSphericalBoundingBox(
      const ispc::SharedStructuredVolume *self, ispc::box3f &boundingBox)
  {
    box1f rRange = {
        self->gridOrigin.x,
        self->gridOrigin.x + (self->dimensions.x - 1.f) * self->gridSpacing.x};

    box1f incRange = {
        self->gridOrigin.y,
        self->gridOrigin.y + (self->dimensions.y - 1.f) * self->gridSpacing.y};

    box1f azRange = {
        self->gridOrigin.z,
        self->gridOrigin.z + (self->dimensions.z - 1.f) * self->gridSpacing.z};

    // reverse ranges in case of negative gridSpacing values
    if (rRange.upper <= rRange.lower) {
      rRange = {rRange.upper, rRange.lower};
    }

    if (incRange.upper <= incRange.lower) {
      incRange = {incRange.upper, incRange.lower};
    }

    if (azRange.upper <= azRange.lower) {
      azRange = {azRange.upper, azRange.lower};
    }

    // critical values to test
  #define NUM_R_TEST_VALUES 2
  #define NUM_INCLINATION_TEST_VALUES 5
  #define NUM_AZIMUTH_TEST_VALUES 7

    const float rs[NUM_R_TEST_VALUES] = {rRange.lower, rRange.upper};

    // inclination grid is guaranteed in [0, PI]
    const float inclinations[NUM_INCLINATION_TEST_VALUES] = {
        0.f, 0.5f * M_PI, M_PI, incRange.lower, incRange.upper};

    // azimuth grid is guaranteed in [0, 2*PI]
    const float azimuths[NUM_AZIMUTH_TEST_VALUES] = {
        0.f, 0.5f * M_PI, M_PI, 1.5f * M_PI, 2.f * M_PI, azRange.lower, azRange.upper};

    box3f bb = {{inf,inf,inf}, {neg_inf,neg_inf,neg_inf}};
    // iterate over critical values and extend bounding box
    for (int i = 0; i < NUM_R_TEST_VALUES; i++) {
      for (int j = 0; j < NUM_INCLINATION_TEST_VALUES; j++) {
        for (int k = 0; k < NUM_AZIMUTH_TEST_VALUES; k++) {
          const float r   = rs[i];
          const float inc = inclinations[j];
          const float az  = azimuths[k];

          // skip values outside the grid
          if (inc < incRange.lower || inc > incRange.upper ||
              az < azRange.lower || az > azRange.upper) {
            continue;
          }

          float sinInc, cosInc;
          sinInc = sin(inc);
          cosInc = cos(inc);

          float sinAz, cosAz;
          sinAz = sin(az);
          cosAz = cos(az);

          vec3f objectCoordinates;
          objectCoordinates.x = r * sinInc * cosAz;
          objectCoordinates.y = r * sinInc * sinAz;
          objectCoordinates.z = r * cosInc;

          bb.extend(objectCoordinates);
        }
      }
    }
    boundingBox = {{bb.lower[0],bb.lower[1],bb.lower[2]},{bb.upper[0],bb.upper[1],bb.upper[2]}};
  }

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
      const ispc::VKLFilter filter)
  {
    ispc::SharedStructuredVolume *self = (ispc::SharedStructuredVolume *)_self;

    destructComputeFunctions(self);
    constructComputeFunctions(self, numAttributes);

    self->temporallyStructuredNumTimesteps = temporallyStructuredNumTimesteps;

    if (temporallyUnstructuredIndices) {
      self->temporallyUnstructuredIndices = *(temporallyUnstructuredIndices);
    } else {
      self->temporallyUnstructuredIndices.addr     = nullptr;
      self->temporallyUnstructuredIndices.numItems = 0;
    }
    if (temporallyUnstructuredTimes) {
      self->temporallyUnstructuredTimes = *(temporallyUnstructuredTimes);
    } else {
      self->temporallyUnstructuredTimes.addr     = nullptr;
      self->temporallyUnstructuredTimes.numItems = 0;
    }

    self->dimensions = ispc::vec3i{dimensions.x, dimensions.y, dimensions.z};
    self->gridType   = gridType;
    self->gridOrigin = ispc::vec3f{gridOrigin.x, gridOrigin.y, gridOrigin.z};
    self->gridSpacing =
        ispc::vec3f{gridSpacing.x, gridSpacing.y, gridSpacing.z};

    if (self->gridType == ispc::structured_regular) {
      vec3f gridUpper =
          gridOrigin +
          vec3f{dimensions.x - 1.f, dimensions.y - 1.f, dimensions.z - 1.f} *
              gridSpacing;
      self->boundingBox = {{gridOrigin.x, gridOrigin.y, gridOrigin.z},
                           {gridUpper.x, gridUpper.y, gridUpper.z}};

      CALL_ISPC(assignComputeGradientChecks, self);

    } else if (self->gridType == ispc::structured_spherical) {
      computeStructuredSphericalBoundingBox(self, self->boundingBox);

      CALL_ISPC(assignComputeGradientChecks, self);
    }

    vec3f lcubtmp = nextafter(dimensions - 1, vec3i{0, 0, 0});
    self->localCoordinatesUpperBound = {lcubtmp.x, lcubtmp.y, lcubtmp.z};

    self->voxelOfs_dx = 1;
    self->voxelOfs_dy = dimensions.x;
    self->voxelOfs_dz = dimensions.x * dimensions.y;

    self->filter = filter;

    for (uint32 i = 0; i < numAttributes; i++) {
      bool success = false;

      if (hasStructuredTimeData(self)) {
        success =
            CALL_ISPC(assignTemporallyStructuredSamplingFunctions, self, i);
      } else if (hasUnstructuredTimeData(self)) {
        success =
            CALL_ISPC(assignTemporallyUnstructuredSamplingFunctions, self, i);
      } else {
        success = CALL_ISPC(assignTemporallyConstantSamplingFunctions, self, i);
      }

      if (!success) {
        printf(
            "#vkl:shared_structured_volume: error assigning sampling "
            "functions\n");
        return false;
      }
    }

    return true;
  }

    template <int W>
    Sampler<W> *StructuredVolume<W>::newSampler()
    {
      return new StructuredRegularSampler<W>(this->getDevice(), *this);
    }

    template struct StructuredVolume<VKL_TARGET_WIDTH>;

  }  // namespace cpu_device
}  // namespace openvkl
