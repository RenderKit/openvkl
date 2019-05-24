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
#include "common/Data.h"

namespace volley {
  namespace ispc_driver {

    template <int W>
    bool ISPCDriver<W>::supportsWidth(int width)
    {
      return width == W || width == 4 || width == 8 || width == 16;
    }

    template <int W>
    void ISPCDriver<W>::commit()
    {
      Driver::commit();
    }

    template <int W>
    void ISPCDriver<W>::commit(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    template <int W>
    void ISPCDriver<W>::release(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VLYData ISPCDriver<W>::newData(size_t numItems,
                                   VLYDataType dataType,
                                   const void *source,
                                   VLYDataCreationFlags dataCreationFlags)
    {
      Data *data = new Data(numItems, dataType, source, dataCreationFlags);
      return (VLYData)data;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Iterator ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

#define __define_newRayIteratorN(WIDTH)                         \
  template <int W>                                              \
  VLYRayIterator ISPCDriver<W>::newRayIterator##WIDTH(          \
      const int *valid,                                         \
      VLYVolume volume,                                         \
      const vvec3fn<WIDTH> &origin,                             \
      const vvec3fn<WIDTH> &direction,                          \
      const vrange1fn<WIDTH> &tRange,                           \
      VLYSamplesMask samplesMask)                               \
  {                                                             \
    return newRayIteratorAnyWidth<WIDTH>(                       \
        valid, volume, origin, direction, tRange, samplesMask); \
  }

    __define_newRayIteratorN(1);
    __define_newRayIteratorN(4);
    __define_newRayIteratorN(8);
    __define_newRayIteratorN(16);

#undef __define_newRayIteratorN

#define __define_iterateIntervalN(WIDTH)          \
  template <int W>                                \
  void ISPCDriver<W>::iterateInterval##WIDTH(     \
      const int *valid,                           \
      VLYRayIterator rayIterator,                 \
      vVLYRayIntervalN<WIDTH> &rayInterval,       \
      vintn<WIDTH> &result)                       \
  {                                               \
    return iterateIntervalAnyWidth<WIDTH>(        \
        valid, rayIterator, rayInterval, result); \
  }

    __define_iterateIntervalN(1);
    __define_iterateIntervalN(4);
    __define_iterateIntervalN(8);
    __define_iterateIntervalN(16);

#undef __define_iterateIntervalN

    template <int W>
    void ISPCDriver<W>::iterateSurface8(const int *valid,
                                        VLYRayIterator rayIterator,
                                        VLYSurfaceHit8 &surfaceHit,
                                        vintn<8> &result)
    {
      auto &rayIteratorObject =
          referenceFromHandle<RayIterator<8>>(rayIterator);
      rayIteratorObject.iterateSurface(valid, result);
      surfaceHit = *reinterpret_cast<const VLYSurfaceHit8 *>(
          rayIteratorObject.getCurrentSurfaceHit());
    }

    ///////////////////////////////////////////////////////////////////////////
    // Module /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VLYError ISPCDriver<W>::loadModule(const char *moduleName)
    {
      return volley::loadLocalModule(moduleName);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    void ISPCDriver<W>::set1f(VLYObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::set1i(VLYObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    template <int W>
    void ISPCDriver<W>::setVec3f(VLYObject object,
                                 const char *name,
                                 const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setVec3i(VLYObject object,
                                 const char *name,
                                 const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    template <int W>
    void ISPCDriver<W>::setObject(VLYObject object,
                                  const char *name,
                                  VLYObject setObject)
    {
      ManagedObject *target = (ManagedObject *)object;
      ManagedObject *value  = (ManagedObject *)setObject;
      target->setParam(name, value);
    }

    template <int W>
    void ISPCDriver<W>::setString(VLYObject object,
                                  const char *name,
                                  const std::string &s)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, s);
    }

    template <int W>
    void ISPCDriver<W>::setVoidPtr(VLYObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    /////////////////////////////////////////////////////////////////////////
    // Samples mask /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    template <int W>
    VLYSamplesMask ISPCDriver<W>::newSamplesMask(VLYVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return (VLYSamplesMask)volumeObject.newSamplesMask();
    }

    template <int W>
    void ISPCDriver<W>::samplesMaskSetRanges(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setRanges(ranges);
    }

    template <int W>
    void ISPCDriver<W>::samplesMaskSetValues(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const float> &values)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    VLYVolume ISPCDriver<W>::newVolume(const char *type)
    {
      std::stringstream ss;
      ss << type << "_" << W;
      return (VLYVolume)Volume<W>::createInstance(ss.str());
    }

    template <int W>
    float ISPCDriver<W>::computeSample(VLYVolume volume,
                                       const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.computeSample(objectCoordinates);
    }

#define __define_computeSampleN(WIDTH)                                       \
  template <int W>                                                           \
  void ISPCDriver<W>::computeSample##WIDTH(                                  \
      const int *valid,                                                      \
      VLYVolume volume,                                                      \
      const vvec3fn<WIDTH> &objectCoordinates,                               \
      vfloatn<WIDTH> &samples)                                               \
  {                                                                          \
    computeSampleAnyWidth<WIDTH>(valid, volume, objectCoordinates, samples); \
  }

    __define_computeSampleN(4);
    __define_computeSampleN(8);
    __define_computeSampleN(16);

#undef __define_computeSampleN

    template <int W>
    vec3f ISPCDriver<W>::computeGradient(VLYVolume volume,
                                         const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.computeGradient(objectCoordinates);
    }

    template <int W>
    box3f ISPCDriver<W>::getBoundingBox(VLYVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);
      return volumeObject.getBoundingBox();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private methods ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <int W>
    template <int OW>
    typename std::enable_if<(OW <= W), VLYRayIterator>::type
    ISPCDriver<W>::newRayIteratorAnyWidth(const int *valid,
                                          VLYVolume volume,
                                          const vvec3fn<OW> &origin,
                                          const vvec3fn<OW> &direction,
                                          const vrange1fn<OW> &tRange,
                                          VLYSamplesMask samplesMask)
    {
      auto &volumeObject = referenceFromHandle<Volume<W>>(volume);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vvec3fn<W> originW    = static_cast<vvec3fn<W>>(origin);
      vvec3fn<W> directionW = static_cast<vvec3fn<W>>(direction);
      vrange1fn<W> tRangeW  = static_cast<vrange1fn<W>>(tRange);

      return (VLYRayIterator)volumeObject.newRayIteratorV(
          originW,
          directionW,
          tRangeW,
          reinterpret_cast<const SamplesMask *>(samplesMask));
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW > W), VLYRayIterator>::type
    ISPCDriver<W>::newRayIteratorAnyWidth(const int *valid,
                                          VLYVolume volume,
                                          const vvec3fn<OW> &origin,
                                          const vvec3fn<OW> &direction,
                                          const vrange1fn<OW> &tRange,
                                          VLYSamplesMask samplesMask)
    {
      throw std::runtime_error(
          "ray iterators cannot be created for widths greater than the native "
          "runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW <= W), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           VLYRayIterator rayIterator,
                                           vVLYRayIntervalN<OW> &rayInterval,
                                           vintn<OW> &result)
    {
      auto &rayIteratorObject =
          referenceFromHandle<RayIterator<W>>(rayIterator);

      vintn<W> validW;
      for (int i = 0; i < W; i++)
        validW[i] = i < OW ? valid[i] : 0;

      vintn<W> resultW;

      rayIteratorObject.iterateInterval(validW, resultW);

      vVLYRayIntervalN<W> rayIntervalW;

      rayIntervalW = *reinterpret_cast<const vVLYRayIntervalN<W> *>(
          rayIteratorObject.getCurrentRayInterval());

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
    typename std::enable_if<(OW > W), void>::type
    ISPCDriver<W>::iterateIntervalAnyWidth(const int *valid,
                                           VLYRayIterator rayIterator,
                                           vVLYRayIntervalN<OW> &rayInterval,
                                           vintn<OW> &result)
    {
      throw std::runtime_error(
          "cannot iterate on ray intervals for widths greater than the native "
          "runtime vector width");
    }

    template <int W>
    template <int OW>
    typename std::enable_if<(OW <= W), void>::type
    ISPCDriver<W>::computeSampleAnyWidth(const int *valid,
                                         VLYVolume volume,
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
                                         VLYVolume volume,
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

    VLY_REGISTER_DRIVER(ISPCDriver<4>, ispc_driver_4)
    VLY_REGISTER_DRIVER(ISPCDriver<8>, ispc_driver_8)
    VLY_REGISTER_DRIVER(ISPCDriver<16>, ispc_driver_16)

  }  // namespace ispc_driver
}  // namespace volley

extern "C" VOLLEY_DLLEXPORT void volley_init_module_ispc_driver() {}
