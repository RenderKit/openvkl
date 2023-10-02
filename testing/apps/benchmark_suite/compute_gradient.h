// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "utility.h"

/*
 * Benchmark wrappers for vklComputeGradient* functions.
 */

namespace api {

  using openvkl::vintn;
  using openvkl::vvec3fn;

  template <class ProgrammingModel,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeGradient;

  template <class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeGradient<programming_model::Scalar,
                            VolumeWrapper,
                            CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalar" << CoordinateGenerator::name() << "Gradient";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));
      vkl_vec3f objectCoordinates{0.f, 0.f, 0.f};

      BENCHMARK_WARMUP_AND_RUN(({
        gen.template getNextN<1>(&objectCoordinates);
        benchmark::DoNotOptimize(
            vklComputeGradient(&sampler, &objectCoordinates));
      }));

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  namespace impl {

    template <int W, class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeGradient;

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeGradient<4, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<4> &valid,
                              VKLSampler sampler,
                              const vvec3fn<4> &coord,
                              vvec3fn<4> &samples)
      {
        vklComputeGradient4(reinterpret_cast<const int *>(&valid),
                            &sampler,
                            reinterpret_cast<const vkl_vvec3f4 *>(&coord),
                            reinterpret_cast<vkl_vvec3f4 *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeGradient<8, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<8> &valid,
                              VKLSampler sampler,
                              const vvec3fn<8> &coord,
                              vvec3fn<8> &samples)
      {
        vklComputeGradient8(reinterpret_cast<const int *>(&valid),
                            &sampler,
                            reinterpret_cast<const vkl_vvec3f8 *>(&coord),
                            reinterpret_cast<vkl_vvec3f8 *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeGradient<16, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<16> &valid,
                              VKLSampler sampler,
                              const vvec3fn<16> &coord,
                              vvec3fn<16> &samples)
      {
        vklComputeGradient16(reinterpret_cast<const int *>(&valid),
                             &sampler,
                             reinterpret_cast<const vkl_vvec3f16 *>(&coord),
                             reinterpret_cast<vkl_vvec3f16 *>(&samples));
      }
    };

  }  // namespace impl

  template <int W, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeGradient<programming_model::Vector<W>,
                            VolumeWrapper,
                            CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "vector" << CoordinateGenerator::name() << "Gradient"
         << "<" << W;
      if (!VolumeWrapper::name().empty())
        os << ", " << VolumeWrapper::name();
      os << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      vintn<W> valid;
      vvec3fn<W> objectCoordinates;
      vvec3fn<W> samples;

      for (int i = 0; i < W; i++) {
        valid[i] = 1;
      }

      BENCHMARK_WARMUP_AND_RUN(({
        gen.template getNextV<W>(&objectCoordinates);
        impl::VklComputeGradient<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples);
      }));

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * W);
    }
  };

  template <unsigned int N, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeGradient<programming_model::Stream<N>,
                            VolumeWrapper,
                            CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "stream" << CoordinateGenerator::name() << "Gradient"
         << "<" << N;
      if (!VolumeWrapper::name().empty())
        os << ", " << VolumeWrapper::name();
      os << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<vkl_vec3f> samples(N);

      BENCHMARK_WARMUP_AND_RUN(({
        gen.template getNextN<N>(objectCoordinates.data());
        vklComputeGradientN(
            &sampler, N, objectCoordinates.data(), samples.data());
      }));

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * N);
    }
  };

}  // namespace api

/*
 * Register benchmarks related to vklComputeGradient*
 */
template <class VolumeWrapper, class CoordinateGenerator>
inline void registerComputeGradient()
{
  using programming_model::Scalar;
  using programming_model::Stream;
  using programming_model::Vector;

  registerBenchmark<
      api::VklComputeGradient<Scalar, VolumeWrapper, CoordinateGenerator>>();

  registerBenchmark<
      api::VklComputeGradient<Vector<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeGradient<Vector<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Vector<16>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();

  registerBenchmark<
      api::VklComputeGradient<Stream<1>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeGradient<Stream<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<
      api::VklComputeGradient<Stream<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Stream<16>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Stream<32>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Stream<64>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Stream<128>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();
  registerBenchmark<api::VklComputeGradient<Stream<256>,
                                            VolumeWrapper,
                                            CoordinateGenerator>>();
}
