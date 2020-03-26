// ======================================================================== //
// Copyright 2019-2020 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../common/ManagedObject.h"
#include "../common/objectFactory.h"
#include "../iterator/DefaultIterator.h"
#include "../value_selector/ValueSelector.h"
#include "openvkl/openvkl.h"
#include "ospcommon/math/box.h"

#define THROW_NOT_IMPLEMENTED                          \
  throw std::runtime_error(std::string(__FUNCTION__) + \
                           " not implemented in this volume!")

using namespace ospcommon;

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    struct Volume : public ManagedObject
    {
      Volume()                   = default;
      virtual ~Volume() override = default;

      static Volume *createInstance(const std::string &type);

      // Volumes may implement their own interval and hit iterators based on
      // their internal acceleration structures. If not provided, a default
      // iterator implementation will be used instead. The default iterator
      // implementation may not be performant.
      //
      // Both scalar (uniform) and wide (varying) interfaces for iterators are
      // available. These are suffixed with U and V respectively, e.g.
      // initIntervalIteratorU() and initIntervalIteratorV(). If uniform
      // implementations are not overridden, inputs will be upconverted and the
      // varying implementations will be used. If varying implementations are
      // not implemented, the default iterator implementation will be used.
      // Thus, if a volume implements its own iterators, it should implement the
      // varying interfaces first, as the uniform interfaces will automatically
      // make use of it.
      //
      // The iterator objects can be casted to volume-specific iterator types,
      // and may maintain internal state as desired, e.g. for current state
      // within an acceleration structure.

      // Initialize a new interval iterator for the given input ray(s)
      // (specified by origin, direction and tRange) and optional valueSelector
      // indicating volume sample values of interest. If no valueSelector is
      // provided, all intervals intersecting the volume should be (iteratively)
      // returned by iterateInterval[U,V]()
      virtual void initIntervalIteratorU(vVKLIntervalIteratorN<1> &iterator,
                                         const vvec3fn<1> &origin,
                                         const vvec3fn<1> &direction,
                                         const vrange1fn<1> &tRange,
                                         const ValueSelector<W> *valueSelector);

      // The valid mask indicates which lanes are active.
      virtual void initIntervalIteratorV(const vintn<W> &valid,
                                         vVKLIntervalIteratorN<W> &iterator,
                                         const vvec3fn<W> &origin,
                                         const vvec3fn<W> &direction,
                                         const vrange1fn<W> &tRange,
                                         const ValueSelector<W> *valueSelector);

      // Iterate once for the given iterator and return the next interval (if
      // any) satisfying the iterator's valueSelector in interval. Result (0 or
      // 1) indicates if a new interval was found.
      virtual void iterateIntervalU(vVKLIntervalIteratorN<1> &iterator,
                                    vVKLIntervalN<1> &interval,
                                    vintn<1> &result);

      virtual void iterateIntervalV(const vintn<W> &valid,
                                    vVKLIntervalIteratorN<W> &iterator,
                                    vVKLIntervalN<W> &interval,
                                    vintn<W> &result);

      // Initialize a new hit iterator for the given input ray(s) (specified by
      // origin, direction and tRange) and optional valueSelector indicating
      // volume sample values of interest. If no valueSelector is provided, or
      // the value selector contains now values, no hits should be returned by
      // iterateHit[U,V]().
      virtual void initHitIteratorU(vVKLHitIteratorN<1> &iterator,
                                    const vvec3fn<1> &origin,
                                    const vvec3fn<1> &direction,
                                    const vrange1fn<1> &tRange,
                                    const ValueSelector<W> *valueSelector);

      // The valid mask indicates which lanes are active.
      virtual void initHitIteratorV(const vintn<W> &valid,
                                    vVKLHitIteratorN<W> &iterator,
                                    const vvec3fn<W> &origin,
                                    const vvec3fn<W> &direction,
                                    const vrange1fn<W> &tRange,
                                    const ValueSelector<W> *valueSelector);

      // Iterate once for the given iterator and return the next hit (if any)
      // satisfying the iterator's valueSelector in hit. Result (0 or 1)
      // indicates if a new hit was found.
      virtual void iterateHitU(vVKLHitIteratorN<1> &iterator,
                               vVKLHitN<1> &hit,
                               vintn<1> &result);

      virtual void iterateHitV(const vintn<W> &valid,
                               vVKLHitIteratorN<W> &iterator,
                               vVKLHitN<W> &hit,
                               vintn<W> &result);

      virtual ValueSelector<W> *newValueSelector();

      // volumes can optionally define a scalar sampling method; if not
      // defined then the default implementation will use computeSampleV()
      virtual void computeSample(const vvec3fn<1> &objectCoordinates,
                                 vfloatn<1> &samples) const;

      virtual void computeSampleV(const vintn<W> &valid,
                                  const vvec3fn<W> &objectCoordinates,
                                  vfloatn<W> &samples) const = 0;

      virtual void computeGradientV(const vintn<W> &valid,
                                    const vvec3fn<W> &objectCoordinates,
                                    vvec3fn<W> &gradients) const;

      virtual box3f getBoundingBox() const = 0;

      virtual range1f getValueRange() const = 0;

      void *getISPCEquivalent() const;

      virtual VKLObserver newObserver(const char *type)
      {
        return nullptr;
      }

     protected:
      void *ispcEquivalent{nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    template <int W>
    inline Volume<W> *Volume<W>::createInstance(const std::string &type)
    {
      return createInstanceHelper<Volume<W>, VKL_VOLUME>(type);
    }

    template <int W>
    inline void Volume<W>::initIntervalIteratorU(
        vVKLIntervalIteratorN<1> &iterator,
        const vvec3fn<1> &origin,
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

      vVKLIntervalIteratorN<W> *iteratorW =
          static_cast<vVKLIntervalIteratorN<W> *>(iterator);

      initIntervalIteratorV(
          validW, *iteratorW, originW, directionW, tRangeW, valueSelector);
    }

    template <int W>
    inline void Volume<W>::initIntervalIteratorV(
        const vintn<W> &valid,
        vVKLIntervalIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLIntervalIterator<DefaultIterator<W>>(
          iterator, valid, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void Volume<W>::iterateIntervalU(vVKLIntervalIteratorN<1> &iterator,
                                            vVKLIntervalN<1> &interval,
                                            vintn<1> &result)
    {
      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i == 0 ? -1 : 0;

      vVKLIntervalN<W> intervalW;

      vintn<W> resultW;

      vVKLIntervalIteratorN<W> *iteratorW =
          static_cast<vVKLIntervalIteratorN<W> *>(iterator);

      iterateIntervalV(validW, *iteratorW, intervalW, resultW);

      interval.tRange.lower[0]     = intervalW.tRange.lower[0];
      interval.tRange.upper[0]     = intervalW.tRange.upper[0];
      interval.valueRange.lower[0] = intervalW.valueRange.lower[0];
      interval.valueRange.upper[0] = intervalW.valueRange.upper[0];
      interval.nominalDeltaT[0]    = intervalW.nominalDeltaT[0];

      result[0] = resultW[0];
    }

    template <int W>
    inline void Volume<W>::iterateIntervalV(const vintn<W> &valid,
                                            vVKLIntervalIteratorN<W> &iterator,
                                            vVKLIntervalN<W> &interval,
                                            vintn<W> &result)
    {
      DefaultIterator<W> *i =
          fromVKLIntervalIterator<DefaultIterator<W>>(&iterator);

      i->iterateInterval(valid, result);

      interval =
          *reinterpret_cast<const vVKLIntervalN<W> *>(i->getCurrentInterval());
    }

    template <int W>
    inline void Volume<W>::initHitIteratorU(
        vVKLHitIteratorN<1> &iterator,
        const vvec3fn<1> &origin,
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

      vVKLHitIteratorN<W> *iteratorW =
          static_cast<vVKLHitIteratorN<W> *>(iterator);

      initHitIteratorV(
          validW, *iteratorW, originW, directionW, tRangeW, valueSelector);
    }

    template <int W>
    inline void Volume<W>::initHitIteratorV(
        const vintn<W> &valid,
        vVKLHitIteratorN<W> &iterator,
        const vvec3fn<W> &origin,
        const vvec3fn<W> &direction,
        const vrange1fn<W> &tRange,
        const ValueSelector<W> *valueSelector)
    {
      initVKLHitIterator<DefaultIterator<W>>(
          iterator, valid, this, origin, direction, tRange, valueSelector);
    }

    template <int W>
    inline void Volume<W>::iterateHitU(vVKLHitIteratorN<1> &iterator,
                                       vVKLHitN<1> &hit,
                                       vintn<1> &result)
    {
      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i == 0 ? -1 : 0;

      vVKLHitN<W> hitW;

      vintn<W> resultW;

      vVKLHitIteratorN<W> *iteratorW =
          static_cast<vVKLHitIteratorN<W> *>(iterator);

      iterateHitV(validW, *iteratorW, hitW, resultW);

      hit.t[0]      = hitW.t[0];
      hit.sample[0] = hitW.sample[0];

      result[0] = resultW[0];
    }

    template <int W>
    inline void Volume<W>::iterateHitV(const vintn<W> &valid,
                                       vVKLHitIteratorN<W> &iterator,
                                       vVKLHitN<W> &hit,
                                       vintn<W> &result)
    {
      DefaultIterator<W> *i = fromVKLHitIterator<DefaultIterator<W>>(&iterator);

      i->iterateHit(valid, result);

      hit = *reinterpret_cast<const vVKLHitN<W> *>(i->getCurrentHit());
    }

    template <int W>
    inline ValueSelector<W> *Volume<W>::newValueSelector()
    {
      return new ValueSelector<W>(this);
    }

    template <int W>
    inline void Volume<W>::computeSample(const vvec3fn<1> &objectCoordinates,
                                         vfloatn<1> &sample) const
    {
      // gracefully degrade to use computeSampleV(); see
      // ISPCDriver<W>::computeSampleAnyWidth()

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i == 0 ? 1 : 0;

      ocW.fill_inactive_lanes(validW);

      vfloatn<W> samplesW;

      computeSampleV(validW, ocW, samplesW);

      sample[0] = samplesW[0];
    }

    template <int W>
    inline void Volume<W>::computeGradientV(const vintn<W> &valid,
                                            const vvec3fn<W> &objectCoordinates,
                                            vvec3fn<W> &gradients) const
    {
      THROW_NOT_IMPLEMENTED;
    }

    template <int W>
    inline void *Volume<W>::getISPCEquivalent() const
    {
      return ispcEquivalent;
    }

#define VKL_REGISTER_VOLUME(InternalClass, external_name) \
  VKL_REGISTER_OBJECT(                                    \
      ::openvkl::ManagedObject, volume, InternalClass, external_name)

  }  // namespace ispc_driver
}  // namespace openvkl
