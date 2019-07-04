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

#include "SimpleNativeVolumeRenderer.h"
#include "SimpleNativeVolumeRenderer_ispc.h"
#include "ospray/SDK/transferFunction/TransferFunction.h"
#include "ospray/SDK/volume/Volume.h"

namespace ospray {

  SimpleNativeVolumeRenderer::SimpleNativeVolumeRenderer()
  {
    setParam<std::string>("externalNameFromAPI", "native");

    ispcEquivalent = ispc::SimpleNativeVolumeRenderer_create(this);
  }

  std::string SimpleNativeVolumeRenderer::toString() const
  {
    return "ospray::render::SimpleNativeVolumeRenderer";
  }

  void SimpleNativeVolumeRenderer::commit()
  {
    Renderer::commit();

    Volume *volume = (Volume *)getParamVoidPtr("volume", nullptr);

    TransferFunction *transferFunction =
        (TransferFunction *)getParamObject("transferFunction", nullptr);

    if (transferFunction == nullptr)
      throw std::runtime_error(
          "no transfer function specified on the OpenVKL renderer!");

    ispc::SimpleNativeVolumeRenderer_set(
        getIE(), volume->getIE(), transferFunction->getIE());
  }

  OSP_REGISTER_RENDERER(SimpleNativeVolumeRenderer, simple_native);

}  // namespace ospray
