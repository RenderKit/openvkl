// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Scene.ih"

export void *uniform Scene_create()
{
  Scene *uniform scene = uniform new Scene;
  memset(scene, 0, sizeof(uniform Scene));
  return scene;
}

export void Scene_destroy(void *uniform self)
{
  delete ((Scene * uniform) self);
}

export void Scene_setVolume(void *uniform self, void *uniform volume)
{
  ((Scene * uniform) self)->volume = ((uniform VKLVolume)volume);
}

export void Scene_setSampler(void *uniform self, const void *uniform sampler)
{
  ((Scene * uniform) self)->sampler = *((VKLSampler *uniform)sampler);
}

export void Scene_setCamera(void *uniform self,
    uniform float sensorWidth,
    uniform float focalLength,
    const uniform vec3f &cameraToWorld_R0,
    const uniform vec3f &cameraToWorld_R1,
    const uniform vec3f &cameraToWorld_R2,
    const uniform vec3f &cameraToWorld_t)
{
  uniform Camera &camera = ((Scene * uniform) self)->camera;

  camera.sensorWidth = sensorWidth;
  camera.focalLength = focalLength;
  camera.ctw_R[0] = cameraToWorld_R0;
  camera.ctw_R[1] = cameraToWorld_R1;
  camera.ctw_R[2] = cameraToWorld_R2;
  camera.ctw_t = cameraToWorld_t;
}


export void Scene_setRendererParams(void *uniform self,
                                    uniform int attributeIndex,
                                    uniform float time,
                                    uniform bool restrictPixelRange,
                                    uniform const box2i &pixelRange)
{
  uniform RendererParams &params = ((Scene * uniform) self)->rendererParams;

  params.attributeIndex       = attributeIndex;
  params.time                 = time;
  params.restrictPixelRange   = restrictPixelRange;
  params.pixelRange           = pixelRange;
}

export void Scene_setTransferFunction(void *uniform self,
                                      const uniform box1f &valueRange,
                                      uniform size_t numValues,
                                      const vec4f *uniform colorsAndOpacities)
{
  uniform TransferFunction &tf = ((Scene * uniform) self)->transferFunction;

  tf.valueRange         = valueRange;
  tf.numValues          = numValues;
  tf.colorsAndOpacities = colorsAndOpacities;
}
