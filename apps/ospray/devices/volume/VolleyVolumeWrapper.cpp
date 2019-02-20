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

        vlyVolume = vlyNewVolume(volleyVolumeType.c_str());
        vlyCommit(vlyVolume);

        volleyInitialized = true;
      }

      // pass all supported parameters through to Volley volume object
      std::for_each(
          params_begin(), params_end(), [&](std::shared_ptr<Param> &p) {
            auto &param = *p;

            if (param.data.is<vec3f>()) {
              std::cerr << "[VolleyVolumeWrapper] passing through parameter: "
                        << param.name << std::endl;
              vlySet3f(vlyVolume,
                       param.name.c_str(),
                       param.data.get<vec3f>().x,
                       param.data.get<vec3f>().y,
                       param.data.get<vec3f>().z);
            } else if (param.data.is<vec3i>()) {
              std::cerr << "[VolleyVolumeWrapper] passing through parameter: "
                        << param.name << std::endl;
              vlySet3i(vlyVolume,
                       param.name.c_str(),
                       param.data.get<vec3i>().x,
                       param.data.get<vec3i>().y,
                       param.data.get<vec3i>().z);
            } else {
              std::cerr << "[VolleyVolumeWrapper] ignoring unsupported "
                           "parameter type: "
                        << param.name << std::endl;
            }
          });

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
      return vlyComputeSample(vlyVolume, (vly_vec3f *)&worldCoordinates);
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
                       return vlyComputeSample(vlyVolume, (vly_vec3f *)&c);
                     });

      return samples;
    }

    void VolleyVolumeWrapper::advance(Ray &ray) const
    {
#warning VolleyVolumeWrapper advance() method needs to use ray iterators

#if 1
            ray.t0 += 0.1f;
            return;
#else
      VLYSamplesMask samplesMask;  // = vlyNewSamplesMask();
      vly_range1f tRange{ray.t0, ray.t};

      VLYRayIterator rayIterator = vlyNewRayIterator(vlyVolume,
                                                     (vly_vec3f *)&ray.org.x,
                                                     (vly_vec3f *)&ray.dir.x,
                                                     &tRange,
                                                     samplesMask);
      VLYRayInterval rayInterval;
      vlyIterateInterval(rayIterator, &rayInterval);

      ray.t0 = rayInterval.tRange.upper;

      vlyRelease(rayIterator);
#endif
    }

    int VolleyVolumeWrapper::setRegion(const void *,
                                       const vec3i &,
                                       const vec3i &)
    {
      // no-op
      return 0;
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
