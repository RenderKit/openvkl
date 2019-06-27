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

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <ospray/ospray.h>
#include "ospcommon/math/vec.h"

using namespace ospcommon::math;

using ColorPoint   = vec4f;
using OpacityPoint = vec2f;

class TransferFunctionWidget
{
 public:
  TransferFunctionWidget(OSPTransferFunction transferFunction,
                         std::function<void()> transferFunctionUpdatedCallback,
                         const vec2f &valueRange       = vec2f(-1.f, 1.f),
                         const std::string &widgetName = "Transfer Function");
  ~TransferFunctionWidget();

  void saveTransferFunction(const std::string &filename);
  void loadTransferFunction(const std::string &filename);

  // update UI and process any UI events
  void updateUI();

 private:
  void loadDefaultMaps();
  void setMap(int);

  vec3f interpolateColor(const std::vector<ColorPoint> &controlPoints, float x);

  float interpolateOpacity(const std::vector<OpacityPoint> &controlPoints,
                           float x);

  void updateTfnPaletteTexture();

  void drawEditor();

  // transfer function being modified and associated callback whenver it's
  // updated
  OSPTransferFunction transferFunction{nullptr};
  std::function<void()> transferFunctionUpdatedCallback{nullptr};

  // called to perform actual transfer function updates when control points
  // change in UI
  std::function<void(const std::vector<ColorPoint> &,
                     const std::vector<OpacityPoint> &)>
      updateTransferFunction{nullptr};

  // all available transfer functions
  std::vector<std::string> tfnsNames;
  std::vector<std::vector<ColorPoint>> tfnsColorPoints;
  std::vector<std::vector<OpacityPoint>> tfnsOpacityPoints;
  std::vector<bool> tfnsEditable;

  // properties of currently selected transfer function
  int currentMap{0};
  std::vector<ColorPoint> *tfnColorPoints;
  std::vector<OpacityPoint> *tfnOpacityPoints;
  bool tfnEditable{true};

  // flag indicating transfer function has changed in UI
  bool tfnChanged{true};

  // number of samples for interpolated transfer function passed to OSPRay
  int numSamples{256};

  // scaling factor for generated opacities passed to OSPRay
  float globalOpacityScale{1.f};

  // domain (value range) of transfer function
  vec2f valueRange{-1.f, 1.f};

  // texture for displaying transfer function color palette
  GLuint tfnPaletteTexture{0};

  // widget name (use different names to support multiple concurrent widgets)
  std::string widgetName;

  // input dialog for save / load filename
  std::array<char, 512> filenameInput{'\0'};
};
