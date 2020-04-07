// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.ih"
#include "CellRef.ih"
#include "DualCell.ih"
#include "common/export_util.h"

varying float AMR_finest(const void *uniform _self, const varying vec3f &P)
{
  const AMRVolume *uniform self = (const AMRVolume *)_self;
  const AMR *uniform amr        = &self->amr;

  vec3f lP;  // local amr space
  self->transformWorldToLocal(self, P, lP);

  DualCell D;
  initDualCell(D, lP, *amr->finestLevel);
  findDualCell(amr, D);
  return lerp(D);
}

varying float AMR_finestLevel(const void *uniform _self, const varying vec3f &P)
{
  const AMRVolume *uniform self = (const AMRVolume *uniform)_self;
  const AMR *uniform amr        = &self->amr;

  vec3f lP;  // local amr space
  self->transformWorldToLocal(self, P, lP);

  const CellRef C = findLeafCell(amr, lP);
  float width     = C.width;
  if (isnan(width))
    width = 1.0f;
  return width;
}

export void EXPORT_UNIQUE(AMR_install_finest, void *uniform _self)
{
  AMRVolume *uniform self           = (AMRVolume * uniform) _self;
  self->super.computeSample_varying = AMR_finest;
  self->computeSampleLevel          = AMR_finestLevel;
}
