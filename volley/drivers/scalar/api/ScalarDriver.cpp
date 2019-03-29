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

#include "ScalarDriver.h"
#include "../samples_mask/SamplesMask.h"
#include "../volume/Volume.h"

namespace volley {
  namespace scalar_driver {

    void ScalarDriver::commit()
    {
      Driver::commit();
    }

    void ScalarDriver::commit(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->commit();
    }

    void ScalarDriver::release(VLYObject object)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->refDec();
    }

    ///////////////////////////////////////////////////////////////////////////
    // Iterator ///////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYRayIterator ScalarDriver::newRayIterator(VLYVolume volume,
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

    bool ScalarDriver::iterateInterval(VLYRayIterator rayIterator,
                                       VLYRayInterval &rayInterval)
    {
      auto &rayIteratorObject = referenceFromHandle<RayIterator>(rayIterator);
      bool result             = rayIteratorObject.iterateInterval();
      rayInterval             = *reinterpret_cast<const VLYRayInterval *>(
          rayIteratorObject.getCurrentRayInterval());
      return result;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Module /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYError ScalarDriver::loadModule(const char *moduleName)
    {
      return volley::loadLocalModule(moduleName);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parameters /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void ScalarDriver::set1f(VLYObject object, const char *name, const float x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    void ScalarDriver::set1i(VLYObject object, const char *name, const int x)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, x);
    }

    void ScalarDriver::setVec3f(VLYObject object,
                                const char *name,
                                const vec3f &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    void ScalarDriver::setVec3i(VLYObject object,
                                const char *name,
                                const vec3i &v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    void ScalarDriver::setVoidPtr(VLYObject object, const char *name, void *v)
    {
      ManagedObject *managedObject = (ManagedObject *)object;
      managedObject->setParam(name, v);
    }

    /////////////////////////////////////////////////////////////////////////
    // Samples mask /////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////

    VLYSamplesMask ScalarDriver::newSamplesMask(VLYVolume volume)
    {
      return (VLYSamplesMask)SamplesMask::createInstance();
    }

    void ScalarDriver::samplesMaskSetRanges(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.setRanges(ranges);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Volume /////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYVolume ScalarDriver::newVolume(const char *type)
    {
      return (VLYVolume)Volume::createInstance(type);
    }

    float ScalarDriver::computeSample(VLYVolume volume,
                                      const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.computeSample(objectCoordinates);
    }

    void ScalarDriver::computeSample8(const int *valid,
                                      VLYVolume volume,
                                      const vly_vvec3f8 &objectCoordinates,
                                      float *samples)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      volumeObject.computeSample8(valid, objectCoordinates, samples);
    }

    vec3f ScalarDriver::computeGradient(VLYVolume volume,
                                        const vec3f &objectCoordinates)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.computeGradient(objectCoordinates);
    }

    box3f ScalarDriver::getBoundingBox(VLYVolume volume)
    {
      auto &volumeObject = referenceFromHandle<Volume>(volume);
      return volumeObject.getBoundingBox();
    }

    VLY_REGISTER_DRIVER(ScalarDriver, scalar_driver)
  }  // namespace scalar_driver
}  // namespace volley

extern "C" VOLLEY_DLLEXPORT void volley_init_module_scalar_driver() {}
