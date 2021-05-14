// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <stack>
#include <vector>

namespace openvkl {
  namespace utility {
    namespace temporal_compression {

      /*
       * Compress the given temporal data in place.
       *
       * This is a lossy compression controlled by the compression parameter.
       * It is an implementation of the method outlined in
       *
       * Douglas and Peucker,
       * "Algorithms for the Reduction of the Number of Points Required to
       * Represent a Digitized Line or its Caricature", Cartographica: The
       * International Journal for Geographic Information and Geovisualization,
       * 1973
       *
       * Returns the number of samples after compression.
       *
       * compression is some positive number. The higher the number, the more
       * the function is compressed.
       */
      template <typename ValueT>
      inline size_t douglas_peucker(size_t numSamples,
                                    ValueT *samples,
                                    float *times,
                                    float compression)
      {
        std::vector<ValueT> compressedSamples;
        compressedSamples.reserve(numSamples);

        std::vector<float> compressedTimes;
        compressedTimes.reserve(numSamples);

        std::stack<std::pair<size_t, size_t>> segments;
        segments.emplace(0, numSamples - 1);
        while (!segments.empty()) {
          const size_t first = segments.top().first;
          const size_t last  = segments.top().second;
          segments.pop();

          const float sFirst = static_cast<float>(samples[first]);
          if (first + 1 < last) {
            const float dv         = static_cast<float>(samples[last]) - sFirst;
            const float dt         = times[last] - times[first];
            const float m          = dv / dt;
            const float threshold  = compression * std::max(dv, 1e-3f);
            size_t maxErrorIdx     = 0;
            float maxError         = 0.f;
            for (size_t i = first + 1; i < last; ++i) {
              const bool zero      = (samples[i] == ValueT(0));
              const bool zeroChain = zero && (samples[i - 1] == ValueT(0)) &&
                                     (samples[i + 1] == ValueT(0));
              float error = std::numeric_limits<float>::infinity();
              // Never remove zero vertices unless part of a chain of zeros
              // to avoid introducing material.
              if (!zero || zeroChain) {
                // Vertical distance between the current point and the line.
                const float t = times[i] - times[first];
                error =
                    std::abs(sFirst + t * m - static_cast<float>(samples[i]));
              }
              if (error > maxError) {
                maxErrorIdx = i;
                maxError    = error;
              }
            }
            // This is a straight line. Skip intermediate points and move on to
            // the next segment.
            if (maxError <= threshold) {
              compressedSamples.push_back(samples[first]);
              compressedTimes.push_back(times[first]);
            } else {
              // This is not a straight line, so examine the two parts
              // separately. Note: push second segment first, this is a stack.
              segments.emplace(maxErrorIdx, last);
              segments.emplace(first, maxErrorIdx);
            }
          } else {
            // Accept single segments directly.
            compressedSamples.push_back(samples[first]);
            compressedTimes.push_back(times[first]);
          }
        }
        // Keep the last point as we never had the chance to add it.
        compressedSamples.push_back(samples[numSamples - 1]);
        compressedTimes.push_back(times[numSamples - 1]);

        assert(compressedSamples.size() <= numSamples);
        assert(compressedTimes.size() == compressedSamples.size());

        std::copy(compressedSamples.begin(), compressedSamples.end(), samples);
        std::copy(compressedTimes.begin(), compressedTimes.end(), times);
        return compressedSamples.size();
      }

    }  // namespace temporal_compression
  }    // namespace utility
}  // namespace openvkl
