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

#include "VolleySimpleProceduralVolume.h"
#include "../common/Data.h"

namespace ospray {
  namespace scalar_volley_device {

    void VolleySimpleProceduralVolume::commit()
    {
      Volume::commit();

      static bool volleyInitialized = false;

      if (!volleyInitialized) {
        std::cout << "initializing Volley" << std::endl;

        vlyLoadModule("scalar_driver");

        VLYDriver driver = vlyNewDriver("scalar_driver");
        vlyCommitDriver(driver);
        vlySetCurrentDriver(driver);

        volleyInitialized = true;
      }

      vlyVolume = vlyNewVolume("simple_procedural_volume");

      // TODO
      samplingStep = 0.01f;
    }

    bool VolleySimpleProceduralVolume::intersect(Ray &ray) const
    {
      vly_range1f range;

      vlyIntersectVolume(vlyVolume,
                         1,
                         (const vly_vec3f *)&ray.org,
                         (const vly_vec3f *)&ray.dir,
                         &range);

      if (range.lower < range.upper) {  // should be nan check
        ray.t0 = range.lower;
        ray.t  = range.upper;
        return true;
      } else {
        return false;
      }
    }

    float VolleySimpleProceduralVolume::computeSample(
        const vec3f &worldCoordinates) const
    {
      float sample;

      vlySampleVolume(vlyVolume,
                      VLY_SAMPLE_LINEAR,
                      1,
                      (vly_vec3f *)&worldCoordinates,
                      &sample);

      return sample;
    }

    std::vector<float> VolleySimpleProceduralVolume::computeSamples(
        const std::vector<vec3f> &worldCoordinates) const
    {
      std::vector<float> samples;
      samples.resize(worldCoordinates.size());

      vlySampleVolume(vlyVolume, VLY_SAMPLE_LINEAR, worldCoordinates.size(), (vly_vec3f *)worldCoordinates.data(), (float *)samples.data());

      return samples;
    }

    void VolleySimpleProceduralVolume::advance(Ray &ray) const
    {
      ray.t0 += samplingStep;
    }

    int VolleySimpleProceduralVolume::setRegion(const void *,
                                                const vec3i &,
                                                const vec3i &)
    {
      // no-op
      return 0;
    }

  }  // namespace scalar_volley_device
}  // namespace ospray
