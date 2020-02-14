// ======================================================================== //
// Copyright 2019 Intel Corporation                                         //
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

#include "iterator/Iterator.ih"
#include "iterator/GridAcceleratorIterator.ih"
#include "math/box.ih"
#include "math/vec.ih"
#include "openvkl/openvkl.isph"

export uniform int sizeofVaryingRange1f()
{
  return sizeof(varying box1f);
}

export varying box1f *uniform newVaryingRange1f()
{
  return uniform new varying box1f;
}

export uniform int sizeofVaryingVec3f()
{
  return sizeof(varying vec3f);
}

export varying vec3f *uniform newVaryingVec3f()
{
  return uniform new varying vec3f;
}

export uniform int sizeofVaryingVKLIntervalIterator()
{
  return sizeof(varying VKLIntervalIterator);
}

export varying VKLIntervalIterator *uniform newVaryingVKLIntervalIterator()
{
  return uniform new varying VKLIntervalIterator;
}

export uniform int sizeofVaryingVKLHitIterator()
{
  return sizeof(varying VKLHitIterator);
}

export varying VKLHitIterator *uniform newVaryingVKLHitIterator()
{
  return uniform new varying VKLHitIterator;
}

export uniform int sizeofVaryingInterval()
{
  return sizeof(varying Interval);
}

export varying Interval *uniform newVaryingInterval()
{
  return uniform new varying Interval;
}

export uniform int sizeofVaryingHit()
{
  return sizeof(varying Hit);
}

export varying Hit *uniform newVaryingHit()
{
  return uniform new varying Hit;
}

export uniform int sizeofGridAcceleratorIterator()
{
  return sizeof(varying GridAcceleratorIterator);
}

export void *uniform newGridAcceleratorIterator()
{
  return uniform new varying GridAcceleratorIterator;
}
