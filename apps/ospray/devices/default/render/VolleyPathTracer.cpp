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

#include "VolleyPathTracer.h"
#include "VolleyPathTracer_ispc.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VolleyPathTracer::VolleyPathTracer()
  {
    setParam<std::string>("externalNameFromAPI", "volley_pathtracer");

    ispcEquivalent = ispc::VolleyPathTracer_create(this);
  }

  std::string VolleyPathTracer::toString() const
  {
    return "ospray::render::VolleyPathTracer";
  }

  void VolleyPathTracer::commit()
  {
    Renderer::commit();

    VLYVolume vlyVolume = (VLYVolume)getParamVoidPtr("vlyVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the Volley renderer!");

    float sigmaTScale    = getParam1f("sigmaTScale", 1.f);
    float sigmaSScale    = getParam1f("sigmaSScale", 1.f);
    int maxNumScatters   = getParam1i("maxNumScatters", 1);
    float lightIntensity = getParam1f("lightIntensity", 1.f);
    int useRatioTracking   = getParam1i("useRatioTracking", 0);

    ispc::VolleyPathTracer_set(getIE(),
                               (ispc::VolleyVolume *)vlyVolume,
                               transferFunction->getIE(),
                               sigmaTScale,
                               sigmaSScale,
                               maxNumScatters,
                               lightIntensity,
                               useRatioTracking);
  }

  OSP_REGISTER_RENDERER(VolleyPathTracer, volley_pathtracer);

}  // namespace ospray
