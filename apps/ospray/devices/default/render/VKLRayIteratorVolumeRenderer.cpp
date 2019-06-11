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

#include "VKLRayIteratorVolumeRenderer.h"
#include "VKLRayIteratorVolumeRenderer_ispc.h"
#include "ospray/SDK/common/Data.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VKLRayIteratorVolumeRenderer::VKLRayIteratorVolumeRenderer()
  {
    setParam<std::string>("externalNameFromAPI", "vkl_ray_iterator");

    ispcEquivalent = ispc::VKLRayIteratorVolumeRenderer_create(this);
  }

  std::string VKLRayIteratorVolumeRenderer::toString() const
  {
    return "ospray::render::VKLRayIteratorVolumeRenderer";
  }

  void VKLRayIteratorVolumeRenderer::commit()
  {
    Renderer::commit();

    VKLVolume vklVolume = (VKLVolume)getParamVoidPtr("vklVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the OpenVKL renderer!");

    float samplingRate = getParam1f("samplingRate", 1.f);

    ispc::VKLRayIteratorVolumeRenderer_set(getIE(),
                                              (ispc::OpenVKLVolume *)vklVolume,
                                              transferFunction->getIE(),
                                              samplingRate);
  }

  OSP_REGISTER_RENDERER(VKLRayIteratorVolumeRenderer,
                        vkl_ray_iterator_volume);

}  // namespace ospray
