// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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

#include "ISPCDriver.h"
#include "../samples_mask/SamplesMask.h"
#include "../volume/Volume.h"
#include "../common/Data.h"
#include "ispc_util_ispc.h"

namespace openvkl {
  namespace ispc_driver {

    template <int W>
    bool ISPCDriver<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    int ISPCDriver<W>::getNativeSIMDWidth()
    {
      return ispc::get_programCount();
    }

    template <int W>
    void ISPCDriver<W>::commit()
    {
      Driver::commit();
    }

    template <int W>
    void ISPCDriver<W>::commit(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void ISPCDriver<W>::release(VKLObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLData ISPCDriver<W>::newData(size_t numItems,
                                   VKLDataType dataType,
                                   const void *source,
                                   VKLDataCreationFlags dataCreationFlags)
    {
      Data *data = new Data(numItems, dataType, source, dataCreationFlags);
      return (VKLData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Iterator ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

#define __define_newRayIteratorN(WIDTH)                         \
  template <int W>                                              \
  vVKLRayIteratorN<WIDTH> ISPCDriver<W>::newRayIterator##WIDTH( \
      const int *valid,                                         \
      VKLVolume volume,                                         \
      const vvec3fn<WIDTH> &origin,                             \
      const vvec3fn<WIDTH> &direction,                          \
      const vrange1fn<WIDTH> &tRange,                           \
      VKLSamplesMask samplesMask)                               \
  {                                                             \
    return newRayIteratorAnyWidth<WIDTH>(                       \
        valid, volume, origin, direction, tRange, samplesMask); \
  }

    __define_newRayIteratorN(1);
    __define_newRayIteratorN(4);
    __define_newRayIteratorN(8);
    __define_newRayIteratorN(16);

#undef __define_newRayIteratorN

#define __define_iterateIntervalN(WIDTH)                                     \
  template <int W>                                                           \
  void ISPCDriver<W>::iterateInterval##WIDTH(                                \
      const int *valid,                                                      \
      vVKLRayIteratorN<WIDTH> &rayIterator,                                  \
      vVKLRayIntervalN<WIDTH> &rayInterval,                                  \
      vintn<WIDTH> &result)                                                  \
  {                                                                          \
    iterateIntervalAnyWidth<WIDTH>(valid, rayIterator, rayInterval, result); \
  }

    __define_iterateIntervalN(1);
    __define_iterateIntervalN(4);
    __define_iterateIntervalN(8);
    __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

#define __define_iterateSurfaceN(WIDTH)                                    \
  template <int W>                                                         \
  void ISPCDriver<W>::iterateSurface##WIDTH(                               \
      const int *valid,                                                    \
      vVKLRayIteratorN<WIDTH> &rayIterator,                                \
      vVKLSurfaceHitN<WIDTH> &surfaceHit,                                  \
      vintn<WIDTH> &result)                                                \
  {                                                                        \
    iterateSurfaceAnyWidth<WIDTH>(valid, rayIterator, surfaceHit, result); \
  }

    __define_iterateSurfaceN(1);
    __define_iterateSurfaceN(4);
    __define_iterateSurfaceN(8);
    __define_iterateSurfaceN(16);

#undef __define_iterateSurfaceN

    ///////////////////////////////////////////////////////////////////////////
    // Module /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLError ISPCDriver<W>::loadModule(const char *moduleName)
    {
      return openvkl::loadLocalModule(moduleName);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void ISPCDriver<W>::set1f(VKLObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::set1i(VKLObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::setVec3f(VKLObject object,
                                 const char *name,
                                 const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setVec3i(VKLObject object,
                                 const char *name,
                                 const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setObject(VKLObject object,
                                  const char *name,
                                  VKLObject setObject)
    {
      ManagedObject *target = (ManagedObject *)object;
      ManagedObject *value  = (ManagedObject *)setObject;
      target->setParam(name, value);
    }

    template <int W>
    void ISPCDriver<W>::setString(VKLObject object,
                                  const char *name,
                                  const std::string &s)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, s);
    }

    template <int W>
    void ISPCDriver<W>::setVoidPtr(VKLObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    /////////////////////////////////////////////////////////////////////////
    // Samples mask /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLSamplesMask ISPCDriver<W>::newSamplesMask(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VKLSamplesMask)volumeObject.newSamplesMask();
    }

    template <int W>
    void ISPCDriver<W>::samplesMaskSetRanges(
        VKLSamplesMask samplesMask,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setRanges(ranges);
    }

    template <int W>
    void ISPCDriver<W>::samplesMaskSetValues(
        VKLSamplesMask samplesMask,
        const utility::ArrayView<const float> &values)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VKLVolume ISPCDriver<W>::newVolume(const char *type)
    {
      std::stringstream ss;
      ss << type << "_" << W;
      return (VKLVolume)Volume<W>::createInstance(ss.str());
    }

#define __define_computeSampleN(WIDTH)                                       \
  template <int W>                                                           \
  void ISPCDriver<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                      \
      VKLVolume volume,                                                      \
      const vvec3fn<WIDTH> &objectCoordinates,                               \
      vfloatn<WIDTH> &samples)                                               \
  {                                                                          \
    computeSampleAnyWidth<WIDTH>(valid, volume, objectCoordinates, samples); \
  }

    __define_computeSampleN(1);
    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    template <int W>
    vec3f ISPCDriver<W>::computeGradient(VKLVolume volume,
                                         const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.computeGradient(objectCoordinates);
    }

    template <int W>
    box3f ISPCDriver<W>::getBoundingBox(VKLVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), vVKLRayIteratorN<OW>>::type
    ISPCDriver<W>::newRayIteratorAnyWidth(const int *valid,
                                          VKLVolume volume,
                                          const vvec3fn<OW> &origin,
                                          const vvec3fn<OW> &direction,
                                          const vrange1fn<OW> &tRange,
                                          VKLSamplesMask samplesMask)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      vVKLRayIteratorN<W> rayIteratorW = volumeObject.newRayIteratorV(
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const SamplesMask *>(samplesMask));

      vVKLRayIteratorN<1> rayIterator1 =
          static_cast<vVKLRayIteratorN<1>>(rayIteratorW);

      return rayIterator1;
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), vVKLRayIteratorN<OW>>::type
    ISPCDriver<W>::newRayIteratorAnyWidth(const int *valid,
                                          VKLVolume volume,
                                          const vvec3fn<OW> &origin,
                                          const vvec3fn<OW> &direction,
                                          const vrange1fn<OW> &tRange,
                                          VKLSamplesMask samplesMask)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      return volumeObject.newRayIteratorV(
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const SamplesMask *>(samplesMask));
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), vVKLRayIteratorN<OW>>::type
    ISPCDriver<W>::newRayIteratorAnyWidth(const int *valid,
                                          VKLVolume volume,
                                          const vvec3fn<OW> &origin,
                                          const vvec3fn<OW> &direction,
                                          const vrange1fn<OW> &tRange,
                                          VKLSamplesMask samplesMask)
    {
      throw std::runtime_error(
          "ray iterators cannot be created for widths different than the "
          "native runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLRayIteratorN<OW> &rayIterator1,
                                           vVKLRayIntervalN<OW> &rayInterval,
                                           vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(rayIterator1.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLRayIntervalN<W> rayIntervalW;

      vintn<W> resultW;

      vVKLRayIteratorN<W> rayIteratorW =
          static_cast<vVKLRayIteratorN<W>>(rayIterator1);

      volumeObject.iterateIntervalV(
          validW, rayIteratorW, rayIntervalW, resultW);

      rayIterator1 = static_cast<vVKLRayIteratorN<1>>(rayIteratorW);

      for (int i = 0; i < OW; i++) {
        rayInterval.tRange.lower[i]  = rayIntervalW.tRange.lower[i];
        rayInterval.tRange.upper[i]  = rayIntervalW.tRange.upper[i];
        rayInterval.nominalDeltaT[i] = rayIntervalW.nominalDeltaT[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLRayIteratorN<OW> &rayIterator,
                                           vVKLRayIntervalN<OW> &rayInterval,
                                           vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(rayIterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLRayIntervalN<W> rayIntervalW;

      vintn<W> resultW;

      volumeObject.iterateIntervalV(validW, rayIterator, rayIntervalW, resultW);

      for (int i = 0; i < OW; i++) {
        rayInterval.tRange.lower[i]  = rayIntervalW.tRange.lower[i];
        rayInterval.tRange.upper[i]  = rayIntervalW.tRange.upper[i];
        rayInterval.nominalDeltaT[i] = rayIntervalW.nominalDeltaT[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           vVKLRayIteratorN<OW> &rayIterator,
                                           vVKLRayIntervalN<OW> &rayInterval,
                                           vintn<OW> &result)
    {
      throw std::runtime_error(
          "cannot iterate on ray intervals for widths different than the "
          "native runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == 1), void>::type
    ISPCDriver<W>::iterateSurfaceAnyWidth(const int *valid,
                                          vVKLRayIteratorN<OW> &rayIterator1,
                                          vVKLSurfaceHitN<OW> &surfaceHit,
                                          vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(rayIterator1.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLSurfaceHitN<W> surfaceHitW;

      vintn<W> resultW;

      vVKLRayIteratorN<W> rayIteratorW =
          static_cast<vVKLRayIteratorN<W>>(rayIterator1);

      volumeObject.iterateSurfaceV(validW, rayIteratorW, surfaceHitW, resultW);

      rayIterator1 = static_cast<vVKLRayIteratorN<1>>(rayIteratorW);

      for (int i = 0; i < OW; i++) {
        surfaceHit.t[i]      = surfaceHitW.t[i];
        surfaceHit.sample[i] = surfaceHitW.sample[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW == W), void>::type
    ISPCDriver<W>::iterateSurfaceAnyWidth(const int *valid,
                                          vVKLRayIteratorN<OW> &rayIterator,
                                          vVKLSurfaceHitN<OW> &surfaceHit,
                                          vintn<OW> &result)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(rayIterator.volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vVKLSurfaceHitN<W> surfaceHitW;

      vintn<W> resultW;

      volumeObject.iterateSurfaceV(validW, rayIterator, surfaceHitW, resultW);

      for (int i = 0; i < OW; i++) {
        surfaceHit.t[i]      = surfaceHitW.t[i];
        surfaceHit.sample[i] = surfaceHitW.sample[i];
      }

      for (int i = 0; i < OW; i++)
        result[i] = resultW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW != W && OW != 1), void>::type
    ISPCDriver<W>::iterateSurfaceAnyWidth(const int *valid,
                                          vVKLRayIteratorN<OW> &rayIterator,
                                          vVKLSurfaceHitN<OW> &surfaceHit,
                                          vintn<OW> &result)
    {
      throw std::runtime_error(
          "cannot iterate on surfaces for widths different than the native "
          "runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW <= W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         vfloatn<OW> &samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vvec3fn<W> ocW = static_cast<vvec3fn<W>>(objectCoordinates);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vfloatn<W> samplesW;

      // if constexpr is not available in C++11, therefore all combinations of
      // W and OW would be compiled here if we conditionally selected the
      // appropriate volume sampling method; so we need to use void pointers as
      // the full set of explicit conversions are not legal
      volumeObject.computeSampleV(
          (const int *)&validW, (const void *)&ocW, (void *)&samplesW);

      for (int i = 0; i < OW; i++)
        samples[i] = samplesW[i];
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VKLVolume volume,
                                         const vvec3fn<OW> &objectCoordinates,
                                         vfloatn<OW> &samples)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      const int numPacks = OW / W + (OW % W != 0);

      for (int packIndex = 0; packIndex < numPacks; packIndex++) {
        vvec3fn<W> ocW = objectCoordinates.template extract_pack<W>(packIndex);

        vintn<W> validW;
        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          validW[i - packIndex * W] = i < OW ? valid[i] : 0;

        vfloatn<W> samplesW;

        volumeObject.computeSampleV(
            (const int *)&validW, (const void *)&ocW, (void *)&samplesW);

        for (int i = packIndex * W; i < (packIndex + 1) * W && i < OW; i++)
          samples[i] = samplesW[i - packIndex * W];
      }
    }

    VKL_REGISTER_DRIVER(ISPCDriver<4>, ispc_driver_4)
    VKL_REGISTER_DRIVER(ISPCDriver<8>, ispc_driver_8)
    VKL_REGISTER_DRIVER(ISPCDriver<16>, ispc_driver_16)

  }  // namespace ispc_driver
}  // namespace openvkl

extern "C" OPENVKL_DLLEXPORT void openvkl_init_module_ispc_driver() {}
