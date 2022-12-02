// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "iterator/GridAcceleratorIterator.ih"
#include "common/Interval.ih"
#include "common/Hit.ih"
#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"
#include "openvkl/openvkl.isph"
#include "openvkl/device/openvkl.isph"

export void delete_uniform(void *uniform ptr)
{
  delete ptr;
}

export uniform int sizeofVaryingRange1f()
{
  return sizeof(varying box1f);
}

#pragma ignore warning(all)
export varying box1f *uniform newVaryingRange1f()
{
  return uniform new varying box1f;
}

export uniform int sizeofVaryingVec3f()
{
  return sizeof(varying vec3f);
}

#pragma ignore warning(all)
export varying vec3f *uniform newVaryingVec3f()
{
  return uniform new varying vec3f;
}

export uniform int sizeofVaryingVKLIntervalIterator()
{
  return sizeof(varying VKLIntervalIterator);
}

#pragma ignore warning(all)
export varying Interval *uniform newVaryingInterval()
{
  return uniform new varying Interval;
}

export uniform int sizeofVaryingInterval()
{
  return sizeof(varying Interval);
}

export uniform int sizeofVaryingHit()
{
  return sizeof(varying Hit);
}

#pragma ignore warning(all)
export varying Hit *uniform newVaryingHit()
{
  return uniform new varying Hit;
}

export uniform int sizeofGridAcceleratorIteratorU()
{
  return sizeof(uniform GridAcceleratorIterator);
}

export uniform int sizeofGridAcceleratorIteratorV()
{
  return sizeof(varying GridAcceleratorIterator);
}

export void *uniform newGridAcceleratorIteratorU()
{
  return uniform new uniform GridAcceleratorIterator;
}

export void *uniform newGridAcceleratorIteratorV()
{
  return uniform new varying GridAcceleratorIterator;
}
