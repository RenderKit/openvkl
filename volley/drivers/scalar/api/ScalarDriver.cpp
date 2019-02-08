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
#include "../integrator/Integrator.h"
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

    ///////////////////////////////////////////////////////////////////////////
    // Integrator /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    VLYIntegrator ScalarDriver::newIntegrator(const char *type)
    {
      return (VLYIntegrator)Integrator::createInstance(type);
    }

    void ScalarDriver::integrateVolume(
        VLYIntegrator integrator,
        VLYVolume volume,
        size_t numValues,
        const vly_vec3f *origins,
        const vly_vec3f *directions,
        const vly_range1f *ranges,
        void *rayUserData,
        IntegrationStepFunction integrationStepFunction)
    {
      auto &integratorObject = referenceFromHandle<Integrator>(integrator);
      auto &volumeObject     = referenceFromHandle<Volume>(volume);
      integratorObject.integrate(volumeObject,
                                 numValues,
                                 origins,
                                 directions,
                                 ranges,
                                 rayUserData,
                                 integrationStepFunction);
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
#warning not implemented
    }

    bool ScalarDriver::iterateInterval(VLYRayIterator rayIterator,
                                       range1f &tRange,
                                       VLYSamplesMask &intervalSamplesMask){
#warning not implemented
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

    VLYSamplesMask ScalarDriver::newSamplesMask()
    {
      return (VLYSamplesMask)SamplesMask::createInstance();
    }

    void ScalarDriver::samplesMaskAddRanges(
        VLYSamplesMask samplesMask,
        const utility::ArrayView<const range1f> &ranges)
    {
      auto &samplesMaskObject = referenceFromHandle<SamplesMask>(samplesMask);
      samplesMaskObject.addRanges(ranges);
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
