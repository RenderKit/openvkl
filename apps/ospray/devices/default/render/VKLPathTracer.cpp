// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "VKLPathTracer.h"
#include "VKLPathTracer_ispc.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VKLPathTracer::VKLPathTracer()
  {
    setParam<std::string>("externalNameFromAPI", "vkl_pathtracer");

    ispcEquivalent = ispc::VKLPathTracer_create(this);
  }

  std::string VKLPathTracer::toString() const
  {
    return "ospray::render::VKLPathTracer";
  }

  void VKLPathTracer::commit()
  {
    Renderer::commit();

    VKLVolume vklVolume = (VKLVolume)getParamVoidPtr("vklVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the OpenVKL renderer!");

    float sigmaTScale           = getParam1f("sigmaTScale", 1.f);
    float sigmaSScale           = getParam1f("sigmaSScale", 1.f);
    int maxNumScatters          = getParam1i("maxNumScatters", 1);
    int useRatioTracking        = getParam1i("useRatioTracking", 0);

    float ambientLightIntensity = getParam1f("ambientLightIntensity", 1.f);

    float directionalLightIntensity =
        getParam1f("directionalLightIntensity", 1.f);
    float directionalLightAngularDiameter =
        getParam1f("directionalLightAngularDiameter", 45.f) * M_PI / 180.f;
    float directionalLightAzimuth =
        getParam1f("directionalLightAzimuth", 0.f) * M_PI / 180.f;
    float directionalLightElevation =
        getParam1f("directionalLightElevation", 90.f) * M_PI / 180.f;

    // use y direction as up for elevation = 90 deg
    vec3f directionalLightDirection =
        vec3f(cosf(directionalLightAzimuth) * cosf(directionalLightElevation),
              sinf(directionalLightElevation),
              sinf(directionalLightAzimuth) * cosf(directionalLightElevation));

    ispc::VKLPathTracer_set(getIE(),
                               (ispc::OpenVKLVolume *)vklVolume,
                               transferFunction->getIE(),
                               sigmaTScale,
                               sigmaSScale,
                               maxNumScatters,
                               useRatioTracking,
                               ambientLightIntensity,
                               directionalLightIntensity,
                               directionalLightAngularDiameter,
                               *(ispc::vec3f *)&directionalLightDirection);
  }

  OSP_REGISTER_RENDERER(VKLPathTracer, vkl_pathtracer);

}  // namespace ospray
