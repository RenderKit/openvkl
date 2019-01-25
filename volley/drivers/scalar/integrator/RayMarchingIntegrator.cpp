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

#include "RayMarchingIntegrator.h"

namespace volley {
  namespace scalar_driver {

    void RayMarchingIntegrator::commit()
    {
      Integrator::commit();

      samplingRate = getParam<float>("samplingRate", 1.f);
      samplingType =
          VLYSamplingType(getParam<int>("samplingType", VLY_SAMPLE_LINEAR));
    }

    void RayMarchingIntegrator::integrate(
        const Volume &volume,
        size_t numValues,
        const vly_vec3f *origins,
        const vly_vec3f *directions,
        const vly_range1f *ranges,
        void *rayUserData,
        IntegrationStepFunction integrationStepFunction)
    {
      // pre-allocate storage used in integration step callback
      std::vector<vly_vec3f> worldCoordinates(numValues);
      std::vector<float> samples(numValues);
      std::vector<vly_vec3f> gradients;

      if (computeGradients) {
        gradients.resize(numValues);
      }

      // std::vector<bool> provides no direct data() access...
      std::unique_ptr<bool[]> rayTerminationMask(new bool[numValues]);

      // initial values for all rays
      std::vector<float> t(numValues);

      for (size_t i = 0; i < numValues; i++) {
        t[i]                  = ranges[i].lower;
        rayTerminationMask[i] = isnan(t[i]);
      }

      // temporary buffer for projected future t values
      std::vector<float>currentDeltaT(numValues);
      std::vector<float>nextDeltaT(numValues);

      size_t numActiveRays = numValues;

      while (numActiveRays) {
        // reset counter
        numActiveRays = 0;

        // generate world coordinates for all active rays
        for (size_t i = 0; i < numValues; i++) {
          if (isnan(t[i])) {
            rayTerminationMask[i] = true;
          }

          if (!rayTerminationMask[i]) {
            numActiveRays++;

            const vec3f temp =
                (*reinterpret_cast<const vec3f *>(&origins[i])) +
                t[i] * (*reinterpret_cast<const vec3f *>(&directions[i]));

            worldCoordinates[i] = (*reinterpret_cast<const vly_vec3f *>(&temp));
          }
        }

        // generate samples
        // TODO: this only needs to be done for *active* rays
        volume.sample(
            samplingType, numValues, worldCoordinates.data(), samples.data());

        if (computeGradients) {
          volume.gradient(samplingType,
                          numValues,
                          worldCoordinates.data(),
                          gradients.data());
        }

        // pre-compute ray advancement, so we can pass this information to the
        // integration step function for possible modification by the
        // application
        // TODO: this only needs to be done for *active* rays
        nextDeltaT = t;

        volume.advanceRays(
            samplingRate, numValues, origins, directions, nextDeltaT.data());

        std::transform(nextDeltaT.begin(),
                       nextDeltaT.end(),
                       t.begin(),
                       nextDeltaT.begin(),
                       std::minus<float>());

        // call user-provided integration step function
        integrationStepFunction(worldCoordinates.size(),
                                worldCoordinates.data(),
                                directions,
                                currentDeltaT.data(),
                                samples.data(),
                                gradients.data(),
                                rayUserData,
                                nextDeltaT.data(),
                                rayTerminationMask.get());

        // add current deltaT values to previous t for next iteration's samples
        std::transform(
            t.begin(), t.end(), nextDeltaT.begin(), t.begin(), std::plus<float>());
        
        // save "current" deltaT for next integration step function call
        currentDeltaT = nextDeltaT;
      }
    }

    VLY_REGISTER_INTEGRATOR(RayMarchingIntegrator, ray_marching_integrator)

  }  // namespace scalar_driver
}  // namespace volley
