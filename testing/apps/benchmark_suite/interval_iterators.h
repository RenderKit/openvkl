// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#pragma once

#include <vector>
#include "utility.h"

/*
 * Benchmark wrappers for interval iterator functions.
 */

namespace api {

  template <class VolumeWrapper>
  struct IntervalIteratorConstruction
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorConstruction";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLSampler vklSampler;
      static VKLIntervalIteratorContext intervalContext;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static const float time{0.f};
      static size_t intervalIteratorSize{0};
      static std::vector<char> buffers;

      // This is safe: Google benchmark guarantees that threads do not
      // start running until the start of the loop below.
      if (state.thread_index() == 0) {
        wrapper              = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);
        vklSampler           = vklNewSampler(vklVolume);
        vklCommit(vklSampler);
        intervalContext = vklNewIntervalIteratorContext(vklSampler);
        vklCommit(intervalContext);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(&intervalContext);
        buffers.resize(intervalIteratorSize * state.threads());
      }

      BENCHMARK_WARMUP_AND_RUN(({
        void *buffer =
            buffers.data() + intervalIteratorSize * state.thread_index();
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            &intervalContext, &origin, &direction, &tRange, time, buffer);

        benchmark::DoNotOptimize(iterator);
      }));

      if (state.thread_index() == 0)
        wrapper.reset();

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
      os << "scalarIntervalIteratorIterateFirst";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLSampler vklSampler;
      static VKLIntervalIteratorContext intervalContext;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static const float time{0.f};
      static size_t intervalIteratorSize{0};
      static std::vector<char> buffers;

      if (state.thread_index() == 0) {
        wrapper              = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);
        vklSampler           = vklNewSampler(vklVolume);
        vklCommit(vklSampler);
        intervalContext = vklNewIntervalIteratorContext(vklSampler);
        vklCommit(intervalContext);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());

        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};
        vkl_vec3f direction{0.f, 0.f, 1.f};
        vkl_range1f tRange{0.f, 1000.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(&intervalContext);
        buffers.resize(intervalIteratorSize * state.threads());
      }

      VKLInterval interval;
      std::vector<char> buffer;

      BENCHMARK_WARMUP_AND_RUN(({
        void *buffer =
            buffers.data() + intervalIteratorSize * state.thread_index();
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            &intervalContext, &origin, &direction, &tRange, time, buffer);

        bool success = vklIterateInterval(iterator, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }));

      // global teardown only in first thread
      if (state.thread_index() == 0)
        wrapper.reset();

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

  template <class VolumeWrapper>
  struct IntervalIteratorIterateSecond
  {
    static const std::string name()
    {
      std::ostringstream os;
      os << "scalarIntervalIteratorIterateSecond";
      if (!VolumeWrapper::name().empty())
        os << "<" << VolumeWrapper::name() << ">";
      return os.str();
    }

    static inline void run(benchmark::State &state)
    {
      static std::unique_ptr<VolumeWrapper> wrapper;
      static VKLVolume vklVolume;
      static VKLSampler vklSampler;
      static VKLIntervalIteratorContext intervalContext;
      static vkl_vec3f origin;
      static const vkl_vec3f direction{0.f, 0.f, 1.f};
      static const vkl_range1f tRange{0.f, 1000.f};
      static const float time{0.f};
      static size_t intervalIteratorSize{0};
      static std::vector<char> buffers;

      if (state.thread_index() == 0) {
        wrapper              = rkcommon::make_unique<VolumeWrapper>();
        vklVolume            = wrapper->getVolume();
        const vkl_box3f bbox = vklGetBoundingBox(vklVolume);
        vklSampler           = vklNewSampler(vklVolume);
        vklCommit(vklSampler);
        intervalContext = vklNewIntervalIteratorContext(vklSampler);
        vklCommit(intervalContext);

        std::uniform_real_distribution<float> distX(bbox.lower.x, bbox.upper.x);
        std::uniform_real_distribution<float> distY(bbox.lower.y, bbox.upper.y);

        std::random_device rd;
        std::mt19937 eng(rd());
        origin = vkl_vec3f{distX(eng), distY(eng), -1.f};

        intervalIteratorSize = vklGetIntervalIteratorSize(&intervalContext);
        buffers.resize(intervalIteratorSize * state.threads());
      }

      VKLInterval interval;
      BENCHMARK_WARMUP_AND_RUN(({
        void *buffer =
            buffers.data() + intervalIteratorSize * state.thread_index();
        VKLIntervalIterator iterator = vklInitIntervalIterator(
            &intervalContext, &origin, &direction, &tRange, time, buffer);

        vklIterateInterval(iterator, &interval);

        bool success = vklIterateInterval(iterator, &interval);

        if (!success) {
          throw std::runtime_error("vklIterateInterval() returned false");
        }

        benchmark::DoNotOptimize(interval);
      }));

      if (state.thread_index() == 0)
        wrapper.reset();

      // enables rates in report output
      state.SetItemsProcessed(state.iterations());
    }
  };

}  // namespace api

/*
 * Register interval iterator tests.
 */
template <class VolumeWrapper>
inline void registerIntervalIterators()
{
  using Construction = api::IntervalIteratorConstruction<VolumeWrapper>;
  registerBenchmark<Construction>()->UseRealTime();

  using IterateFirst = api::IntervalIteratorIterateFirst<VolumeWrapper>;
  registerBenchmark<IterateFirst>()->UseRealTime();

  using IterateSecond = api::IntervalIteratorIterateSecond<VolumeWrapper>;
  registerBenchmark<IterateSecond>()->UseRealTime();
}
