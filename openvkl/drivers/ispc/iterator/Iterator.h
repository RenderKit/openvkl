// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <type_traits>
#include "../common/align.h"
#include "../common/simd.h"
#include "../value_selector/ValueSelector.h"
#include "openvkl/openvkl.h"

using namespace rkcommon;

#define paste(A, B) __paste(A, B)
#define __paste(A, B) A##B

#define __varying_ispc_type(TypeName) \
  ispc::paste(paste(v, VKL_TARGET_WIDTH), _varying_##TypeName)

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume;

    /*
     * Base class for all iterators.
     */
    template <int W>
    struct Iterator
    {
      /*
       * Disallow all kinds of copying to be on the safe side.
       */
      Iterator(const Iterator &) = delete;
      Iterator &operator=(const Iterator &) = delete;
      Iterator(Iterator &&)                 = delete;
      Iterator &operator=(Iterator &&) = delete;

      explicit Iterator(const Volume<W> *volume) : volume{volume} {}

      // WORKAROUND ICC 15: This destructor must be public!
      virtual ~Iterator() = default;

     protected:
      const Volume<W> *volume;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Interval iterator //////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct IntervalIterator : public Iterator<W>
    {
      using Iterator<W>::Iterator;

      /*
       * Uniform path.
       * Implementing this can have substantial performance benefits, however,
       * note that we fall back to the varying code path by default.
       */
      virtual void initializeIntervalU(const vvec3fn<1> &origin,
                                       const vvec3fn<1> &direction,
                                       const vrange1fn<1> &tRange,
                                       const ValueSelector<W> *valueSelector)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
        vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
        vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

        initializeIntervalV(
            validW, originW, directionW, tRangeW, valueSelector);
      }

      virtual void iterateIntervalU(vVKLIntervalN<1> &interval,
                                    vintn<1> &result)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vVKLIntervalN<W> intervalW;

        vintn<W> resultW;

        iterateIntervalV(validW, intervalW, resultW);

        interval.tRange.lower[0]     = intervalW.tRange.lower[0];
        interval.tRange.upper[0]     = intervalW.tRange.upper[0];
        interval.valueRange.lower[0] = intervalW.valueRange.lower[0];
        interval.valueRange.upper[0] = intervalW.valueRange.upper[0];
        interval.nominalDeltaT[0]    = intervalW.nominalDeltaT[0];

        result[0] = resultW[0];
      }

      /*
       * Varying code path.
       */
      virtual void initializeIntervalV(
          const vintn<W> &valid,
          const vvec3fn<W> &origin,
          const vvec3fn<W> &direction,
          const vrange1fn<W> &tRange,
          const ValueSelector<W> *valueSelector) = 0;

      virtual void iterateIntervalV(const vintn<W> &valid,
                                    vVKLIntervalN<W> &interval,
                                    vintn<W> &result) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Hit iterator ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    struct HitIterator : public Iterator<W>
    {
      using Iterator<W>::Iterator;

      /*
       * Uniform path.
       * Implementing this can have substantial performance benefits, however,
       * note that we fall back to the varying code path by default.
       */

      virtual void initializeHitU(const vvec3fn<1> &origin,
                                  const vvec3fn<1> &direction,
                                  const vrange1fn<1> &tRange,
                                  const ValueSelector<W> *valueSelector)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
        vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
        vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

        initializeHitV(validW, originW, directionW, tRangeW, valueSelector);
      }

      virtual void iterateHitU(vVKLHitN<1> &hit, vintn<1> &result)
      {
        vintn<W> validW;
        for (int i = 0; i < W; i++)
          validW[i] = i == 0 ? -1 : 0;

        vVKLHitN<W> hitW;
        vintn<W> resultW;

        iterateHitV(validW, hitW, resultW);

        hit.t[0]       = hitW.t[0];
        hit.sample[0]  = hitW.sample[0];
        hit.epsilon[0] = hitW.epsilon[0];
        result[0]      = resultW[0];
      }

      /*
       * Varying code path.
       */

      virtual void initializeHitV(const vintn<W> &valid,
                                  const vvec3fn<W> &origin,
                                  const vvec3fn<W> &direction,
                                  const vrange1fn<W> &tRange,
                                  const ValueSelector<W> *valueSelector) = 0;

      virtual void iterateHitV(const vintn<W> &valid,
                               vVKLHitN<W> &hit,
                               vintn<W> &result) = 0;
    };

    ///////////////////////////////////////////////////////////////////////////
    // Iterator factory ///////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    /*
     * Interface for iterator factories.
     * Note that the uniform interfaces attempt to fall back to the varying
     * implementation to reduce implementation burden. However, volumes should
     * consider implementing both varying and uniform code paths for maximum
     * performance.
     */
    template <int W, template <int> class IteratorT>
    struct IteratorFactory
    {
      virtual ~IteratorFactory() = default;

      /*
       * Construct a new varying iterator into the provided buffer.
       */
      virtual IteratorT<W> *constructV(const Volume<W> *volume,
                                       void *buffer) const = 0;

      /*
       * Retrieve the size, in bytes, required for a buffer to be able to
       * store a varying iterator.
       *
       * This includes all padding that may be needed for alignment, and both
       * the storeage required for the iterator itself and its internal state.
       */
      virtual size_t sizeV() const = 0;

      /*
       * Construct a new uniform iterator into the provided buffer.
       */
      virtual IteratorT<W> *constructU(const Volume<W> *volume,
                                       void *buffer) const = 0;

      /*
       * Retrieve the size, in bytes, required for a buffer to be able to
       * store a uniform iterator.
       *
       * This includes all padding that may be needed for alignment, and both
       * the storeage required for the iterator itself and its internal state.
       */
      virtual size_t sizeU() const = 0;
    };

    /*
     * Concrete iterator factory. Derive from this to implement a volume
     * iterator factory.
     */
    template <int W,
              template <int>
              class IteratorBaseT,
              template <int>
              class IteratorT>
    struct ConcreteIteratorFactory : public IteratorFactory<W, IteratorBaseT>
    {
      static_assert(std::is_base_of<IteratorBaseT<W>, IteratorT<W>>::value,
                    "ConcreteIteratorFactory used with incompatible types.");

      IteratorBaseT<W> *constructV(const Volume<W> *volume,
                                   void *buffer) const override final
      {
        return new (align<IteratorT<W>>(buffer)) IteratorT<W>(volume);
      }

      size_t sizeV() const override final
      {
        return alignedSize<IteratorT<W>>();
      }

      IteratorBaseT<W> *constructU(const Volume<W> *volume,
                                   void *buffer) const override final
      {
        return new (align<IteratorT<W>>(buffer)) IteratorT<W>(volume);
      }

      size_t sizeU() const override final
      {
        return alignedSize<IteratorT<W>>();
      }
    };

  }  // namespace ispc_driver
}  // namespace openvkl
