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

#include "VKLHitIteratorRenderer.h"
#include "VKLHitIteratorRenderer_ispc.h"
#include "ospray/SDK/common/Data.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VKLHitIteratorRenderer::VKLHitIteratorRenderer()
  {
    setParam<std::string>("externalNameFromAPI", "vkl_hit_iterator");

    ispcEquivalent = ispc::VKLHitIteratorRenderer_create(this);
  }

  std::string VKLHitIteratorRenderer::toString() const
  {
    return "ospray::render::VKLHitIteratorRenderer";
  }

  void VKLHitIteratorRenderer::commit()
  {
    Renderer::commit();

    VKLVolume vklVolume = (VKLVolume)getParamVoidPtr("vklVolume", nullptr);

    if (!vklVolume)
      throw std::runtime_error("no volume specified on the OpenVKL renderer!");

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (!transferFunction)
      throw std::runtime_error(
          "no transfer function specified on the OpenVKL renderer!");

    Data *isosurfaces = (Data *)getParamData("isosurfaces", nullptr);

    ispc::VKLHitIteratorRenderer_set(
        getIE(),
        (ispc::OpenVKLVolume *)vklVolume,
        transferFunction->getIE(),
        isosurfaces ? isosurfaces->size() : 0,
        isosurfaces ? (float *)isosurfaces->data : nullptr);
  }

  OSP_REGISTER_RENDERER(VKLHitIteratorRenderer, vkl_hit_iterator);

}  // namespace ospray
