// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ProceduralStructuredVolume.h"
#include "procedural_functions.h"

using namespace rkcommon;

namespace openvkl {
  namespace testing {

    template <typename VOXEL_TYPE =
                  VoidType /* should be void (we have static_assert to
                          prevent such instantiation), but isn't due
                          to Windows Visual Studio compiler bug */
              ,
              VOXEL_TYPE samplingFunction(const vec3f &, float) =
                  samplingNotImplemented,
              vec3f gradientFunction(const vec3f &, float) = gradientNotImplemented>
    struct ProceduralStructuredRegularVolume
        : public ProceduralStructuredVolume<VOXEL_TYPE,
                                            samplingFunction,
                                            gradientFunction>
    {
      ProceduralStructuredRegularVolume(
          const vec3i &dimensions,
          const vec3f &gridOrigin,
          const vec3f &gridSpacing,
          const TemporalConfig &temporalConfig   = TemporalConfig(),
          VKLDataCreationFlags dataCreationFlags = VKL_DATA_DEFAULT,
          size_t byteStride                      = 0);

      vec3f transformLocalToObjectCoordinates(
          const vec3f &localCoordinates) const override;

      static void generateGridParameters(const vec3i &dimensions,
                                         const float boundingBoxSize,
                                         vec3f &gridOrigin,
                                         vec3f &gridSpacing);
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                             samplingFunction,
                                             gradientFunction>::
        ProceduralStructuredRegularVolume(
            const vec3i &dimensions,
            const vec3f &gridOrigin,
            const vec3f &gridSpacing,
            const TemporalConfig &temporalConfig,
            VKLDataCreationFlags dataCreationFlags,
            size_t byteStride)
        : ProceduralStructuredVolume<VOXEL_TYPE,
                                     samplingFunction,
                                     gradientFunction>("structuredRegular",
                                                       dimensions,
                                                       gridOrigin,
                                                       gridSpacing,
                                                       temporalConfig,
                                                       dataCreationFlags,
                                                       byteStride)
    {
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline vec3f ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                                   samplingFunction,
                                                   gradientFunction>::
        transformLocalToObjectCoordinates(const vec3f &localCoordinates) const
    {
      return this->gridOrigin + localCoordinates * this->gridSpacing;
    }

    template <typename VOXEL_TYPE,
              VOXEL_TYPE samplingFunction(const vec3f &, float),
              vec3f gradientFunction(const vec3f &, float)>
    inline void ProceduralStructuredRegularVolume<
        VOXEL_TYPE,
        samplingFunction,
        gradientFunction>::generateGridParameters(const vec3i &dimensions,
                                                  const float boundingBoxSize,
                                                  vec3f &gridOrigin,
                                                  vec3f &gridSpacing)
    {
      // generate grid parameters for a bounding box centered at (0,0,0) with a
      // maximum length boundingBoxSize

      const float minGridSpacing =
          reduce_min(boundingBoxSize / (dimensions - 1));

      gridOrigin  = -0.5f * (dimensions - 1) * minGridSpacing;
      gridSpacing = vec3f(minGridSpacing);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Procedural volume types ////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename VOXEL_TYPE>
    using WaveletStructuredRegularVolume =
        ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                          getWaveletValue<VOXEL_TYPE>,
                                          getWaveletGradient>;

    template <typename VOXEL_TYPE>
    using XYZStructuredRegularVolume =
        ProceduralStructuredRegularVolume<VOXEL_TYPE,
                                          getXYZValue<VOXEL_TYPE>,
                                          getXYZGradient>;

    using XProceduralVolume =
        ProceduralStructuredRegularVolume<float, getXValue, getXGradient>;

    using YProceduralVolume =
        ProceduralStructuredRegularVolume<float, getYValue, getYGradient>;

    using ZProceduralVolume =
        ProceduralStructuredRegularVolume<float, getZValue, getZGradient>;

    // required due to Windows Visual Studio compiler bugs, which prevent us
    // from writing e.g. WaveletStructuredRegularVolume<float>
    using WaveletStructuredRegularVolumeUChar =
        ProceduralStructuredRegularVolume<unsigned char,
                                          getWaveletValue<unsigned char>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeShort =
        ProceduralStructuredRegularVolume<short,
                                          getWaveletValue<short>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeUShort =
        ProceduralStructuredRegularVolume<unsigned short,
                                          getWaveletValue<unsigned short>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeHalf =
        ProceduralStructuredRegularVolume<half_float::half,
                                          getWaveletValue<half_float::half>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeFloat =
        ProceduralStructuredRegularVolume<float,
                                          getWaveletValue<float>,
                                          getWaveletGradient>;
    using WaveletStructuredRegularVolumeDouble =
        ProceduralStructuredRegularVolume<double,
                                          getWaveletValue<double>,
                                          getWaveletGradient>;

    using XYZStructuredRegularVolumeUChar =
        ProceduralStructuredRegularVolume<unsigned char,
                                          getXYZValue<unsigned char>,
                                          getXYZGradient>;
    using XYZStructuredRegularVolumeShort =
        ProceduralStructuredRegularVolume<short,
                                          getXYZValue<short>,
                                          getXYZGradient>;
    using XYZStructuredRegularVolumeUShort =
        ProceduralStructuredRegularVolume<unsigned short,
                                          getXYZValue<unsigned short>,
                                          getXYZGradient>;
    using XYZStructuredRegularVolumeHalf =
        ProceduralStructuredRegularVolume<half_float::half,
                                          getXYZValue<half_float::half>,
                                          getXYZGradient>;
    using XYZStructuredRegularVolumeFloat =
        ProceduralStructuredRegularVolume<float,
                                          getXYZValue<float>,
                                          getXYZGradient>;
    using XYZStructuredRegularVolumeDouble =
        ProceduralStructuredRegularVolume<double,
                                          getXYZValue<double>,
                                          getXYZGradient>;

    using SphereStructuredRegularVolumeUChar =
        ProceduralStructuredRegularVolume<unsigned char,
                                          getRotatingSphereValue<unsigned char>,
                                          getRotatingSphereGradient>;
    using SphereStructuredRegularVolumeShort =
        ProceduralStructuredRegularVolume<short,
                                          getRotatingSphereValue<short>,
                                          getRotatingSphereGradient>;
    using SphereStructuredRegularVolumeUShort =
        ProceduralStructuredRegularVolume<unsigned short,
                                          getRotatingSphereValue<unsigned short>,
                                          getRotatingSphereGradient>;
    using SphereStructuredRegularVolumeHalf =
        ProceduralStructuredRegularVolume<half_float::half,
                                          getRotatingSphereValue<half_float::half>,
                                          getRotatingSphereGradient>;
    using SphereStructuredRegularVolumeFloat =
        ProceduralStructuredRegularVolume<float,
                                          getRotatingSphereValue<float>,
                                          getRotatingSphereGradient>;
    using SphereStructuredRegularVolumeDouble =
        ProceduralStructuredRegularVolume<double,
                                          getRotatingSphereValue<double>,
                                          getRotatingSphereGradient>;

    // using type traits to work around Visual Studio compiler templating bugs
    template <typename VOXEL_TYPE>
    struct ProceduralStructuredRegularVolumes
    {
      using Wavelet = void;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<unsigned char>
    {
      using Wavelet =
          ProceduralStructuredRegularVolume<unsigned char,
                                            getWaveletValue<unsigned char>,
                                            getWaveletGradient>;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<short>
    {
      using Wavelet = ProceduralStructuredRegularVolume<short,
                                                        getWaveletValue<short>,
                                                        getWaveletGradient>;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<unsigned short>
    {
      using Wavelet =
          ProceduralStructuredRegularVolume<unsigned short,
                                            getWaveletValue<unsigned short>,
                                            getWaveletGradient>;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<half_float::half>
    {
      using Wavelet =
          ProceduralStructuredRegularVolume<half_float::half,
                                            getWaveletValue<half_float::half>,
                                            getWaveletGradient>;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<float>
    {
      using Wavelet = ProceduralStructuredRegularVolume<float,
                                                        getWaveletValue<float>,
                                                        getWaveletGradient>;
    };

    template <>
    struct ProceduralStructuredRegularVolumes<double>
    {
      using Wavelet = ProceduralStructuredRegularVolume<double,
                                                        getWaveletValue<double>,
                                                        getWaveletGradient>;
    };

  }  // namespace testing
}  // namespace openvkl
