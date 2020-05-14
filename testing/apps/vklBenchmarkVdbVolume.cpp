// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <random>
#include <sstream>
#include "../common/simd.h"
#include "benchmark/benchmark.h"
#include "openvkl_testing.h"
#include "ospcommon/utility/random.h"

using openvkl::vfloatn;
using openvkl::vintn;
using openvkl::vvec3fn;
using namespace openvkl::testing;
using namespace ospcommon::utility;
using openvkl::testing::WaveletVdbVolume;

void initializeOpenVKL()
{
  vklLoadModule("ispc_driver");

  VKLDriver driver = vklNewDriver("ispc");
  vklCommitDriver(driver);
  vklSetCurrentDriver(driver);
}

/*
 * Traits classes for API programming models.
 * These allow differentiating between benchmark implementations.
 */
namespace programming_model {

  struct Scalar
  {
  };

  template <int W>
  struct Vector
  {
  };

  template <unsigned int N>
  struct Stream
  {
  };

}  // namespace programming_model

/*
 * Coordinate generators for point sampling APIs.
 */

namespace coordinate_generator {

  struct Random
  {
    inline static constexpr const char *name()
    {
      return "Random";
    }

    explicit Random(vkl_box3f const &bbox)
    {
      std::random_device rd;
      distX = ospcommon::make_unique<pcg32_biased_float_distribution>(
          rd(), 0, bbox.lower.x, bbox.upper.x);
      distY = ospcommon::make_unique<pcg32_biased_float_distribution>(
          rd(), 0, bbox.lower.y, bbox.upper.y);
      distZ = ospcommon::make_unique<pcg32_biased_float_distribution>(
          rd(), 0, bbox.lower.z, bbox.upper.z);
    }

    inline vkl_vec3f getNextSamplePos()
    {
      return vkl_vec3f{(*distX)(), (*distY)(), (*distZ)()};
    }

    std::unique_ptr<pcg32_biased_float_distribution> distX;
    std::unique_ptr<pcg32_biased_float_distribution> distY;
    std::unique_ptr<pcg32_biased_float_distribution> distZ;
  };

  struct Fixed
  {
    inline static constexpr const char *name()
    {
      return "Fixed";
    }

    explicit Fixed(vkl_box3f const &bbox) {}

    inline vkl_vec3f getNextSamplePos()
    {
      return vkl_vec3f{0.1701f, 0.1701f, 0.1701f};
    }
  };

}  // namespace coordinate_generator

namespace api {

  /*
   * Benchmarks for vklComputeSample.
   */
  template <class ProgrammingModel,
            class VolumeWrapper,
            class CoordinateGenerator>
  struct VklComputeSample;

  template <class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Scalar,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalar" << CoordinateGenerator::name() << "Sample"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      for (auto _ : state) {
        auto objectCoordinates = gen.getNextSamplePos();
        benchmark::DoNotOptimize(
            vklComputeSample(sampler, (const vkl_vec3f *)&objectCoordinates));
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  namespace impl {
    template <int W, class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample;

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<4, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<4> &valid,
                              VKLSampler sampler,
                              const vvec3fn<4> &coord,
                              vfloatn<4> &samples)
      {
        vklComputeSample4(reinterpret_cast<const int *>(&valid),
                          sampler,
                          reinterpret_cast<const vkl_vvec3f4 *>(&coord),
                          reinterpret_cast<float *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<8, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<8> &valid,
                              VKLSampler sampler,
                              const vvec3fn<8> &coord,
                              vfloatn<8> &samples)
      {
        vklComputeSample8(reinterpret_cast<const int *>(&valid),
                          sampler,
                          reinterpret_cast<const vkl_vvec3f8 *>(&coord),
                          reinterpret_cast<float *>(&samples));
      }
    };

    template <class VolumeWrapper, class CoordinateGenerator>
    struct VklComputeSample<16, VolumeWrapper, CoordinateGenerator>
    {
      static inline void call(const vintn<16> &valid,
                              VKLSampler sampler,
                              const vvec3fn<16> &coord,
                              vfloatn<16> &samples)
      {
        vklComputeSample16(reinterpret_cast<const int *>(&valid),
                           sampler,
                           reinterpret_cast<const vkl_vvec3f16 *>(&coord),
                           reinterpret_cast<float *>(&samples));
      }
    };
  }  // namespace impl

  template <int W, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Vector<W>,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "vector" << CoordinateGenerator::name() << "Sample"
         << "<" << W << "," << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      vintn<W> valid;
      vvec3fn<W> objectCoordinates;
      vfloatn<W> samples;

      for (int i = 0; i < W; i++) {
        valid[i] = 1;
      }

      for (auto _ : state) {
        for (int i = 0; i < W; i++) {
          const auto pos         = gen.getNextSamplePos();
          objectCoordinates.x[i] = pos.x;
          objectCoordinates.y[i] = pos.y;
          objectCoordinates.z[i] = pos.z;
        }
        impl::VklComputeSample<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples);
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * W);
    }
  };

  template <unsigned int N, class VolumeWrapper, class CoordinateGenerator>
  struct VklComputeSample<programming_model::Stream<N>,
                          VolumeWrapper,
                          CoordinateGenerator>
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "stream" << CoordinateGenerator::name() << "Sample"
         << "<" << N << "," << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<float> samples(N);

      for (auto _ : state) {
        for (int i = 0; i < N; i++) {
          objectCoordinates[i] = gen.getNextSamplePos();
        }
        vklComputeSampleN(sampler, N, objectCoordinates.data(), samples.data());
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * N);
    }
  };

  /*
   * Benchmarks for vklComputeGradient.
   */
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
      os << "scalar" << CoordinateGenerator::name() << "Gradient"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      for (auto _ : state) {
        auto objectCoordinates = gen.getNextSamplePos();
        benchmark::DoNotOptimize(
            vklComputeGradient(sampler, (const vkl_vec3f *)&objectCoordinates));
      }

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
                            sampler,
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
                            sampler,
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
                             sampler,
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
         << "<" << W << "," << VolumeWrapper::name() << ">";
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

      for (auto _ : state) {
        for (int i = 0; i < W; i++) {
          const auto pos         = gen.getNextSamplePos();
          objectCoordinates.x[i] = pos.x;
          objectCoordinates.y[i] = pos.y;
          objectCoordinates.z[i] = pos.z;
        }
        impl::VklComputeGradient<W, VolumeWrapper, CoordinateGenerator>::call(
            valid, sampler, objectCoordinates, samples);
      }

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
         << "<" << N << "," << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      VolumeWrapper wrapper;
      VKLSampler sampler = wrapper.getSampler();
      CoordinateGenerator gen(vklGetBoundingBox(wrapper.getVolume()));

      std::vector<vkl_vec3f> objectCoordinates(N);
      std::vector<vkl_vec3f> samples(N);

      for (auto _ : state) {
        for (int i = 0; i < N; i++) {
          objectCoordinates[i] = gen.getNextSamplePos();
        }
        vklComputeGradientN(
            sampler, N, objectCoordinates.data(), samples.data());
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations() * N);
    }
  };

  /*
   * Iterator apis.
   */
  template <class VolumeWrapper>
  struct IntervalIteratorConstruction
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorConstruction"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLIntervalIterator iterator;
      static vkl_vec3f origin;

      if (state.thread_index == 0) {
        wrapper = ospcommon::make_unique<VolumeWrapper>();

        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};
      }

      vkl_vec3f direction{0.f, 0.f, 1.f};
      vkl_range1f tRange{0.f, 1000.f};

      for (auto _ : state) {
        VKLIntervalIterator iterator;
        vklInitIntervalIterator(
            &iterator, vklVolume, &origin, &direction, &tRange, nullptr);

        benchmark::DoNotOptimize(iterator);
      }

      // global teardown only in first thread
      if (state.thread_index == 0) {
        wrapper.reset();
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  template <class VolumeWrapper>
  struct IntervalIteratorIterateFirst
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorIterateFirst"
         << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLIntervalIterator iterator;

      if (state.thread_index == 0) {
        wrapper = ospcommon::make_unique<VolumeWrapper>();

        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());

        vkl_vec3f origin{distX(eng), distY(eng), -1.f};
        vkl_vec3f direction{0.f, 0.f, 1.f};
        vkl_range1f tRange{0.f, 1000.f};

        vklInitIntervalIterator(
            &iterator, vklVolume, &origin, &direction, &tRange, nullptr);
      }

      VKLInterval interval;

      for (auto _ : state) {
        VKLIntervalIterator iteratorTemp = iterator;

        bool success = vklIterateInterval(&iteratorTemp, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }

      // global teardown only in first thread
      if (state.thread_index == 0) {
        wrapper.reset();
      }

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

}  // namespace api

/*
 * Convenience wrapper that registers a benchmark with Google Benchmark.
 */
template <class Api>
inline benchmark::internal::Benchmark *registerBenchmark()
{
  return benchmark::RegisterBenchmark(Api::name().c_str(), Api::run);
}

/*
 * Register the given type of point query benchmark.
 */
template <template <class, class, class> class Api,
          class VolumeWrapper,
          class CoordinateGenerator>
inline void registerPointQueryBenchmarks()
{
  using programming_model::Scalar;
  using programming_model::Stream;
  using programming_model::Vector;
  registerBenchmark<Api<Scalar, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Vector<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Vector<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Vector<16>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<1>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<2>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<4>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<8>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<16>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<32>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<64>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<128>, VolumeWrapper, CoordinateGenerator>>();
  registerBenchmark<Api<Stream<256>, VolumeWrapper, CoordinateGenerator>>();
}

/*
 * Register interval iterator benchmarks.
 */
template <class Api>
inline void registerIntervalIteratorBenchmarks()
{
  registerBenchmark<Api>()->UseRealTime();
  registerBenchmark<Api>()->Threads(2)->UseRealTime();
  registerBenchmark<Api>()->Threads(4)->UseRealTime();
  registerBenchmark<Api>()->Threads(6)->UseRealTime();
  registerBenchmark<Api>()->Threads(12)->UseRealTime();
  registerBenchmark<Api>()->Threads(36)->UseRealTime();
  registerBenchmark<Api>()->Threads(72)->UseRealTime();
}

/*
 * Register all benchmarks for the given volume type.
 */
template <class VolumeWrapper>
inline void registerVolumeBenchmarks()
{
  using api::IntervalIteratorConstruction;
  using api::IntervalIteratorIterateFirst;
  using api::VklComputeGradient;
  using api::VklComputeSample;

  registerPointQueryBenchmarks<VklComputeSample,
                               VolumeWrapper,
                               coordinate_generator::Fixed>();
  registerPointQueryBenchmarks<VklComputeSample,
                               VolumeWrapper,
                               coordinate_generator::Random>();

  registerPointQueryBenchmarks<VklComputeGradient,
                               VolumeWrapper,
                               coordinate_generator::Fixed>();
  registerPointQueryBenchmarks<VklComputeGradient,
                               VolumeWrapper,
                               coordinate_generator::Random>();

  registerIntervalIteratorBenchmarks<
      IntervalIteratorConstruction<VolumeWrapper>>();
  registerIntervalIteratorBenchmarks<
      IntervalIteratorIterateFirst<VolumeWrapper>>();
}

/*
 * VDB volume wrapper.
 */

template <VKLFilter filter>
constexpr const char *toString();

template <>
inline constexpr const char *toString<VKL_FILTER_NEAREST>()
{
  return "VKL_FILTER_NEAREST";
}

template <>
inline constexpr const char *toString<VKL_FILTER_TRILINEAR>()
{
  return "VKL_FILTER_TRILINEAR";
}

template <VKLFilter filter>
struct Vdb
{
  static std::string name()
  {
    return toString<filter>();
  }

  Vdb()
  {
    volume =
        ospcommon::make_unique<WaveletVdbVolume>(128, vec3f(0.f), vec3f(1.f));

    vklVolume  = volume->getVKLVolume();
    vklSampler = vklNewSampler(vklVolume);
    vklSetInt(vklSampler, "filter", filter);
    vklSetInt(vklSampler, "gradientFilter", filter);
    vklCommit(vklSampler);
  }

  ~Vdb()
  {
    vklRelease(vklSampler);
    vklRelease(vklVolume);
    volume.reset();
  }

  inline VKLVolume getVolume() const
  {
    return vklVolume;
  }

  inline VKLSampler getSampler() const
  {
    return vklSampler;
  }

  std::unique_ptr<WaveletVdbVolume> volume;
  VKLVolume vklVolume{nullptr};
  VKLSampler vklSampler{nullptr};
};

// based on BENCHMARK_MAIN() macro from benchmark.h
int main(int argc, char **argv)
{
  initializeOpenVKL();

  registerVolumeBenchmarks<Vdb<VKL_FILTER_NEAREST>>();
  registerVolumeBenchmarks<Vdb<VKL_FILTER_TRILINEAR>>();

  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;

  ::benchmark::RunSpecifiedBenchmarks();

  vklShutdown();

  return 0;
}
