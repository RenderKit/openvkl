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

#include "VKLIntervalIteratorRenderer.h"
#include "VKLIntervalIteratorRenderer_ispc.h"
#include "ospray/SDK/common/Data.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"

namespace ospray {

  VKLIntervalIteratorRenderer::VKLIntervalIteratorRenderer()
  {
    setParam<std::string>("externalNameFromAPI", "vkl_interval_iterator");

    ispcEquivalent = ispc::VKLIntervalIteratorRenderer_create(this);
  }

  std::string VKLIntervalIteratorRenderer::toString() const
  {
    return "ospray::render::VKLIntervalIteratorRenderer";
  }

  void VKLIntervalIteratorRenderer::commit()
  {
    Renderer::commit();

    VKLVolume vklVolume = (VKLVolume)getParamVoidPtr("vklVolume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the OpenVKL renderer!");

    float samplingRate = getParam1f("samplingRate", 1.f);

    ispc::VKLIntervalIteratorRenderer_set(getIE(),
                                          (ispc::OpenVKLVolume *)vklVolume,
                                          transferFunction->getIE(),
                                          samplingRate);
  }

  OSP_REGISTER_RENDERER(VKLIntervalIteratorRenderer, vkl_interval_iterator);

}  // namespace ospray
