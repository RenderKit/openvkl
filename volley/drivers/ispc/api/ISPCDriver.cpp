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

    void ISPCDriver::commit()
    {
      Driver::commit();
    }

    void ISPCDriver::commit(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    void ISPCDriver::release(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Data ///////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYData ISPCDriver::newData(size_t numItems,
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

    VLYRayIterator ISPCDriver::newRayIterator(VLYVolume volume,
                                              const vec3f &origin,
                                              const vec3f &direction,
                                              const range1f &tRange,
                                              VLYSamplesMask samplesMask)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return (VLYRayIterator)volumeObject.newRayIterator(
          origin,
          direction,
          tRange,
          reinterpret_cast<const SamplesMask *>(samplesMask));
    }

    VLYRayIterator ISPCDriver::newRayIterator8(const int *valid,
                                               VLYVolume volume,
                                               const vvec3fn<8> &origin,
                                               const vvec3fn<8> &direction,
                                               const vrange1fn<8> &tRange,
                                               VLYSamplesMask samplesMask)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return (VLYRayIterator)volumeObject.newRayIterator8(
          origin,
          direction,
          tRange,
          reinterpret_cast<const SamplesMask *>(samplesMask));
    }

    bool ISPCDriver::iterateInterval(VLYRayIterator rayIterator,
                                     VLYRayInterval &rayInterval)
    {
      throw std::runtime_error(
          "iterateInterval() not implemented on this driver!");
    }

    void ISPCDriver::iterateInterval8(const int *valid,
                                      VLYRayIterator rayIterator,
                                      VLYRayInterval8 &rayInterval,
                                      vintn<8> &result)
    {
      auto &rayIteratorObject =
          referenceFromHandle<RayIterator<8>>(rayIterator);
      rayIteratorObject.iterateInterval(valid, result);
      rayInterval = *reinterpret_cast<const VLYRayInterval8 *>(
          rayIteratorObject.getCurrentRayInterval());
    }

    void ISPCDriver::iterateSurface8(const int *valid,
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

    VLYError ISPCDriver::loadModule(const char *moduleName)
    {
      return volley::loadLocalModule(moduleName);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void ISPCDriver::set1f(VLYObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    void ISPCDriver::set1i(VLYObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    void ISPCDriver::setVec3f(VLYObject object,
                              const char *name,
                              const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    void ISPCDriver::setVec3i(VLYObject object,
                              const char *name,
                              const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    void ISPCDriver::setString(VLYObject object,
                               const char *name,
                               const std::string &s)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, s);
    }

    void ISPCDriver::setVoidPtr(VLYObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    /////////////////////////////////////////////////////////////////////////
    // Samples mask /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    VLYSamplesMask ISPCDriver::newSamplesMask(VLYVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return (VLYSamplesMask)volumeObject.newSamplesMask();
    }

    void ISPCDriver::samplesMaskSetRanges(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setRanges(ranges);
    }

    void ISPCDriver::samplesMaskSetValues(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const float> &values)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setValues(values);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYVolume ISPCDriver::newVolume(const char *type)
    {
      return (VLYVolume)Volume::createInstance(type);
    }

    float ISPCDriver::computeSample(VLYVolume volume,
                                    const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.computeSample(objectCoordinates);
    }

    void ISPCDriver::computeSample8(const int *valid,
                                    VLYVolume volume,
                                    const vly_vvec3f8 &objectCoordinates,
                                    float *samples)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      volumeObject.computeSample8(valid, objectCoordinates, samples);
    }

    vec3f ISPCDriver::computeGradient(VLYVolume volume,
                                      const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.computeGradient(objectCoordinates);
    }

    box3f ISPCDriver::getBoundingBox(VLYVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.getBoundingBox();
    }

    VLY_REGISTER_DRIVER(ISPCDriver, ispc_driver)
  }  // namespace ispc_driver
}  // namespace volley

extern "C" VOLLEY_DLLEXPORT void volley_init_module_ispc_driver() {}
