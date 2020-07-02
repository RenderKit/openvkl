// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// openvkl
#include "openvkl/openvkl.h"
// rkcommon
#include "rkcommon/math/range.h"
#include "rkcommon/math/vec.h"

using namespace rkcommon::math;

namespace openvkl {
  namespace testing {

    ///////////////////////////////////////////////////////////////////////////
    // Helper functions ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename T>
    inline VKLDataType getVKLDataType()
    {
      if (std::is_same<T, unsigned char>::value) {
        return VKL_UCHAR;
      } else if (std::is_same<T, short>::value) {
        return VKL_SHORT;
      } else if (std::is_same<T, unsigned short>::value) {
        return VKL_USHORT;
      } else if (std::is_same<T, float>::value) {
        return VKL_FLOAT;
      } else if (std::is_same<T, double>::value) {
        return VKL_DOUBLE;
      } else {
        return VKL_UNKNOWN;
      }
    }

    inline size_t sizeOfVKLDataType(VKLDataType dataType)
    {
      switch (dataType) {
      case VKL_UCHAR:
        return sizeof(unsigned char);
      case VKL_SHORT:
        return sizeof(short);
      case VKL_USHORT:
        return sizeof(unsigned short);
      case VKL_FLOAT:
        return sizeof(float);
      case VKL_DOUBLE:
        return sizeof(double);
      case VKL_UNKNOWN:
        break;
      default:
        break;
      };

      throw std::runtime_error("cannot compute size of unknown VKLDataType");
    }

    template <typename T>
    inline range1f computeValueRange(const void *data, size_t numValues)
    {
      const T *valuesTyped = (const T *)data;

      auto minmax = std::minmax_element(valuesTyped, valuesTyped + numValues);

      return range1f(*minmax.first, *minmax.second);
    }

    inline range1f computeValueRange(VKLDataType dataType,
                                     const void *data,
                                     size_t numValues)
    {
      if (dataType == VKL_UCHAR)
        return computeValueRange<unsigned char>(data, numValues);
      else if (dataType == VKL_SHORT)
        return computeValueRange<short>(data, numValues);
      else if (dataType == VKL_USHORT)
        return computeValueRange<unsigned short>(data, numValues);
      else if (dataType == VKL_FLOAT)
        return computeValueRange<float>(data, numValues);
      else if (dataType == VKL_DOUBLE)
        return computeValueRange<double>(data, numValues);
      else
        throw std::runtime_error(
            "computeValueRange() called with unsupported data type");
    }

    ///////////////////////////////////////////////////////////////////////////
    // TestingVolume //////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    struct TestingVolume
    {
      TestingVolume() = default;
      virtual ~TestingVolume();

      void release();
      VKLVolume getVKLVolume();

      // returns an application-side computed value range, for comparison with
      // vklGetValueRange() results
      virtual range1f getComputedValueRange() const = 0;

     protected:
      virtual void generateVKLVolume() = 0;

      VKLVolume volume{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline TestingVolume::~TestingVolume()
    {
      release();
    }

    inline void TestingVolume::release()
    {
      if (volume) {
        vklRelease(volume);
        volume = nullptr;
      }
    }

    inline VKLVolume TestingVolume::getVKLVolume()
    {
      if (!volume) {
        generateVKLVolume();
      }

      return volume;
    }

  }  // namespace testing
}  // namespace openvkl
