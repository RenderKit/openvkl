// Copyright 2019 Intel Corporation
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
#include "VdbVolumeShared.h"
#include "openvkl/devices/common/StructShared.h"

using namespace rkcommon::memory;

namespace openvkl {
  namespace cpu_device {

    template <typename T>
    using AlignedVector16 =
        std::vector<T, rkcommon::containers::aligned_allocator<T, 16>>;

    ///////////////////////////////////////////////////////////////////////////
    // Helpers ////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    // These may be used in VdbVolume and derived classes

    /*
     * Load an affine 4x3 matrix from the given object.
     */
    inline AffineSpace3f getParamAffineSpace3f(ManagedObject *obj,
                                               const char *name)
    {
      AffineSpace3f a(one);

      if (obj->hasParamT<AffineSpace3f>(name)) {
        a = obj->getParam<AffineSpace3f>(name);
      } else {
        Ref<const DataT<float>> dataIndexToObject =
            obj->template getParamDataT<float>(name, nullptr);

        if (dataIndexToObject && dataIndexToObject->size() >= 12) {
          const DataT<float> &i2w = *dataIndexToObject;
          a.l                     = LinearSpace3f(vec3f(i2w[0], i2w[1], i2w[2]),
                              vec3f(i2w[3], i2w[4], i2w[5]),
                              vec3f(i2w[6], i2w[7], i2w[8]));
          a.p                     = vec3f(i2w[9], i2w[10], i2w[11]);
        }
      }
      return a;
    }

    /*
     * Store the given transformation in a format that our ISPC implementation
     * can work with.
     */
    inline void writeTransform(const AffineSpace3f &a, float *buffer)
    {
      assert(buffer);
      buffer[0]  = a.l.row0().x;
      buffer[1]  = a.l.row0().y;
      buffer[2]  = a.l.row0().z;
      buffer[3]  = a.l.row1().x;
      buffer[4]  = a.l.row1().y;
      buffer[5]  = a.l.row1().z;
      buffer[6]  = a.l.row2().x;
      buffer[7]  = a.l.row2().y;
      buffer[8]  = a.l.row2().z;
      buffer[9]  = a.p.x;
      buffer[10] = a.p.y;
      buffer[11] = a.p.z;
    }

    ///////////////////////////////////////////////////////////////////////////
    // VdbVolume //////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct VdbVolume : public AddStructShared<Volume<W>, ispc::VdbVolume>
    {
      VdbVolume(const VdbVolume &) = delete;
      VdbVolume &operator=(const VdbVolume &) = delete;
      VdbVolume(VdbVolume &&other)            = delete;
      VdbVolume &operator=(VdbVolume &&other) = delete;

      VdbVolume(Device *device);

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

     protected:
      virtual void initIndexSpaceTransforms();
      virtual void initLeafNodeData();

     private:
      void cleanup();

     protected:
      box3f bounds;
      std::vector<range1f> valueRanges;

      // populated in initLeafNodeData()
      size_t numLeaves;
      Ref<const DataT<uint32_t>> leafLevel;
      Ref<const DataT<vec3i>> leafOrigin;
      Ref<const DataT<uint32_t>> leafFormat;
      Ref<const DataT<uint32_t>> leafTemporalFormat;

      // populated in initLeafNodeData(), only for sparse (non-dense) volumes
      Ref<const DataT<Data *>> leafData;
      Ref<const DataT<int>> leafStructuredTimesteps;
      Ref<const DataT<Data *>> leafUnstructuredIndices;
      Ref<const DataT<Data *>> leafUnstructuredTimes;

      // optional: re-packed dense and tile node data in single contiguous
      // arrays (per attribute) for improved performance, only for sparse
      // volumes
      Ref<const DataT<Data *>> nodesPackedDense;
      Ref<const DataT<Data *>> nodesPackedTile;

      // populated for dense volumes only on commit
      bool dense{false};
      vec3i denseDimensions;
      vec3i denseIndexOrigin;
      std::vector<Ref<const Data>> denseData;
      VKLTemporalFormat denseTemporalFormat;
      int denseTemporallyStructuredNumTimesteps;
      Ref<const Data> denseTemporallyUnstructuredIndices;
      Ref<const DataT<float>> denseTemporallyUnstructuredTimes;

      VdbGrid *grid{nullptr};
      Allocator allocator{this->getDevice()};

      // Data can either be interpreted as constant cell data, or
      // vertex-centered data. Note that the vertex-centered interpretation is
      // only legal for the dense configuration.
      bool constantCellData{true};

      VKLFilter filter{VKL_FILTER_TRILINEAR};
      VKLFilter gradientFilter{VKL_FILTER_TRILINEAR};
      uint32_t maxSamplingDepth{VKL_VDB_NUM_LEVELS - 1};

      Ref<const DataT<float>> background;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    VdbVolume<W>::VdbVolume(Device *device) : AddStructShared<Volume<W>, ispc::VdbVolume>(device)
    {
      CALL_ISPC(VdbVolume_Constructor, this->getSh());
      this->SharedStructInitialized = true;
    }

    template <int W>
    VdbVolume<W>::~VdbVolume()
    {
      cleanup();
      CALL_ISPC(VdbVolume_destroy, this->getSh());
      this->SharedStructInitialized = false;
    }

  }  // namespace cpu_device
}  // namespace openvkl
