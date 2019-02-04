// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "VolleyVolumeWrapper.h"
#include "../common/Data.h"

namespace ospray {
  namespace scalar_volley_device {

    void VolleyVolumeWrapper::commit()
    {
      Volume::commit();

      static bool volleyInitialized = false;

      if (!volleyInitialized) {
        std::cout << "initializing Volley" << std::endl;

        vlyLoadModule("scalar_driver");

        VLYDriver driver = vlyNewDriver("scalar_driver");
        vlyCommitDriver(driver);
        vlySetCurrentDriver(driver);

        vlyIntegrator = vlyNewIntegrator("ray_marching_integrator");

        vlyVolume = vlyNewVolume("simple_procedural_volume");
        vlyCommit(vlyVolume);

        volleyInitialized = true;
      }

      // update parameters
      samplingRate     = getParam<float>("samplingRate", 1.f);
      adaptiveSampling = bool(getParam<int>("adaptiveSampling", 0));

      // commit parameters on volume
      vlySet1f(vlyVolume, "samplingRate", samplingRate);
      vlySet1i(vlyVolume,
               "samplingMethod",
               getParam<int>("vlySamplingMethod", VLY_SAMPLE_LINEAR));

      vlyCommit(vlyVolume);
    }

    bool VolleyVolumeWrapper::intersect(Ray &ray) const
    {
      vly_box3f boundingBox = vlyGetBoundingBox(vlyVolume);

      auto hits =
          intersectBox(ray, *reinterpret_cast<const box3f *>(&boundingBox));

      if (hits.first < hits.second) {
        ray.t0 = hits.first;
        ray.t  = hits.second;
        return true;
      } else {
        ray.t0 = ray.t = ospcommon::nan;
        return false;
      }
    }

    float VolleyVolumeWrapper::computeSample(
        const vec3f &worldCoordinates) const
    {
      return vlySampleVolume(vlyVolume, (vly_vec3f *)&worldCoordinates);
    }

    std::vector<float> VolleyVolumeWrapper::computeSamples(
        const std::vector<vec3f> &worldCoordinates) const
    {
      std::vector<float> samples;
      samples.resize(worldCoordinates.size());

      std::transform(worldCoordinates.begin(),
                     worldCoordinates.end(),
                     samples.begin(),
                     [&](const vec3f &c) {
                       return vlySampleVolume(vlyVolume, (vly_vec3f *)&c);
                     });

      return samples;
    }

    void VolleyVolumeWrapper::advance(Ray &ray) const
    {
      vlyAdvanceRays(vlyVolume,
                     samplingRate,
                     1,
                     (const vly_vec3f *)&ray.org,
                     (const vly_vec3f *)&ray.dir,
                     &ray.t0);
    }

    int VolleyVolumeWrapper::setRegion(const void *,
                                       const vec3i &,
                                       const vec3i &)
    {
      // no-op
      return 0;
    }

    VLYIntegrator VolleyVolumeWrapper::getVLYIntegrator()
    {
      return vlyIntegrator;
    }

    VLYVolume VolleyVolumeWrapper::getVLYVolume()
    {
      return vlyVolume;
    }

    bool VolleyVolumeWrapper::getAdaptiveSampling() const
    {
      return adaptiveSampling;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
