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

#include "iterator/RayIterator.ih"
#include "math/box.ih"
#include "math/vec.ih"

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

export uniform int sizeofVaryingRayInterval()
{
  return sizeof(varying RayInterval);
}

export varying RayInterval *uniform newVaryingRayInterval()
{
  return uniform new varying RayInterval;
}

export uniform int sizeofVaryingSurfaceHit()
{
  return sizeof(varying SurfaceHit);
}

export varying SurfaceHit *uniform newVaryingSurfaceHit()
{
  return uniform new varying SurfaceHit;
}
