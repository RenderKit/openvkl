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

#include "VolleyRayIteratorVolumeRenderer.h"
#include "VolleyRayIteratorVolumeRenderer_ispc.h"
#include "ospray/SDK/common/Data.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VolleyRayIteratorVolumeRenderer::VolleyRayIteratorVolumeRenderer()
  {
    setParam<std::string>("externalNameFromAPI", "volley_ray_iterator");

    ispcEquivalent = ispc::VolleyRayIteratorVolumeRenderer_create(this);
  }

  std::string VolleyRayIteratorVolumeRenderer::toString() const
  {
    return "ospray::render::VolleyRayIteratorVolumeRenderer";
  }

  void VolleyRayIteratorVolumeRenderer::commit()
  {
    Renderer::commit();

    VLYVolume vlyVolume = (VLYVolume)getParamVoidPtr("vlyVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the Volley renderer!");

    float samplingRate = getParam1f("samplingRate", 1.f);

    ispc::VolleyRayIteratorVolumeRenderer_set(getIE(),
                                              (ispc::VolleyVolume *)vlyVolume,
                                              transferFunction->getIE(),
                                              samplingRate);
  }

  OSP_REGISTER_RENDERER(VolleyRayIteratorVolumeRenderer,
                        volley_ray_iterator_volume);

}  // namespace ospray
