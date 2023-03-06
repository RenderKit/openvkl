// Copyright 2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rkcommon/math/box.ih"
#include "rkcommon/math/vec.ih"

#include "AMRVolume.ih"
#include "CellRef.ih"
#include "DualCell.ih"
#include "common/export_util.h"
#include "sampler/SamplerShared.h"

varying float AMR_finest(const SamplerShared *uniform self,
                         const varying vec3f &P,
                         const uniform uint32 _attributeIndex,
                         const varying float &_time)
{
  const AMRVolume *uniform volume = (const AMRVolume *uniform)self->volume;
  const AMR *uniform amr          = &volume->amr;

  if (!box_contains(volume->boundingBox, P)) {
    return volume->super.super.background[0];
  }

  vec3f lP;  // local amr space
  AMRVolume_transformObjectToLocal(volume, P, lP);

  DualCell D;
  initDualCell(D, lP, *amr->finestLevel);
  findDualCell(amr, D);
  return lerp(D);
}

export void EXPORT_UNIQUE(AMR_install_finest, void *uniform _sampler)
{
  SamplerShared *uniform sampler = (SamplerShared * uniform) _sampler;
  sampler->computeSample_varying = AMR_finest;
}
