// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.ih"
#include "CellRef.ih"
#include "DualCell.ih"
#include "common/export_util.h"
#include "sampler/Sampler.ih"

varying float AMR_current(const Sampler *uniform self, const varying vec3f &P)
{
  const AMRVolume *uniform volume = (const AMRVolume *uniform)self->volume;
  const AMR *uniform amr          = &volume->amr;

  vec3f lP;  // local amr space
  AMRVolume_transformObjectToLocal(volume, P, lP);

  const CellRef C = findLeafCell(amr, lP);

  DualCell D;
  initDualCell(D, lP, C.width);
  findDualCell(amr, D);

  return lerp(D);
}

export void EXPORT_UNIQUE(AMR_install_current, void *uniform _sampler)
{
  Sampler *uniform sampler       = (Sampler * uniform) _sampler;
  sampler->computeSample_varying = AMR_current;
}
