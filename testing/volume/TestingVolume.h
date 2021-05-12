// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// openvkl
#include "openvkl/openvkl.h"
// rkcommon
#include "rkcommon/math/range.h"
#include "rkcommon/math/vec.h"
// half
#include "../external/half.hpp"

using namespace rkcommon::math;

namespace openvkl {
  namespace testing {

    struct TemporalConfig
    {
      enum Type
      {
        Constant,
        Structured,
        Unstructured,
      };

      Type type{Constant};
      std::vector<float> sampleTime;

      // Threshold for temporal compression on temporally unstructured volumes.
      // Lossy if > 0, but will remove duplicate samples at 0.
      bool useTemporalCompression = false;
      float temporalCompressionThreshold = 0;

      TemporalConfig() = default;

      TemporalConfig(Type type, size_t numSamples)
          : type(type), sampleTime(equidistantTime(numSamples))
      {
        assert(type == Constant || numSamples > 0);
      }

      explicit TemporalConfig(const std::vector<float> &sampleTime)
          : type(Unstructured), sampleTime(sampleTime)
      {
        assert(!sampleTime.empty());
      }

      bool isCompatible(const TemporalConfig &other) const
      {
        return (type == other.type) &&
               (sampleTime.size() == other.sampleTime.size());
      }

      bool hasTime() const
      {
        return type != Constant;
      }

      size_t getNumSamples() const
      {
        return type == Constant ? 1 : sampleTime.size();
      }

     private:
      static std::vector<float> equidistantTime(size_t numSamples)
      {
        std::vector<float> st(numSamples);
        // Initialize to {} for numSamples 0, {0} for numSamples 1,
        // and a regular grid between 0 and 1 for numSamples > 1.
        const float dt =
            1.f / static_cast<float>(std::max<size_t>(numSamples, 2) - 1);
        for (size_t i = 0; i < numSamples; ++i)
          st[i] = i * dt;
        return st;
      }
    };


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
      } else if (std::is_same<T, half_float::half>::value) {
        return VKL_HALF;
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
      case VKL_HALF:
        return sizeof(half_float::half);
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
      else if (dataType == VKL_HALF)
        return computeValueRange<half_float::half>(data, numValues);
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
      VKLVolume getVKLVolume(VKLDevice device);

      // returns an application-side computed value range, for comparison with
      // vklGetValueRange() results
      virtual range1f getComputedValueRange() const = 0;

     protected:
      virtual void generateVKLVolume(VKLDevice device) = 0;

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

    inline VKLVolume TestingVolume::getVKLVolume(VKLDevice device)
    {
      if (!volume) {
        generateVKLVolume(device);
      }

      return volume;
    }

  }  // namespace testing
}  // namespace openvkl
