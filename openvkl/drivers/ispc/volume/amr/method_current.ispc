// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.ih"
#include "CellRef.ih"
#include "DualCell.ih"
#include "common/export_util.h"

varying float AMR_current(const AMRVolume *uniform self, const varying vec3f &P)
{
  const AMR *uniform amr = &self->amr;

  vec3f lP;  // local amr space
  self->transformWorldToLocal(self, P, lP);

  const CellRef C = findLeafCell(amr, lP);

  DualCell D;
  initDualCell(D, lP, C.width);
  findDualCell(amr, D);

  return lerp(D);
}

varying float AMR_currentLevel(const AMRVolume *uniform self,
                               const varying vec3f &P)
{
  const AMR *uniform amr = &self->amr;

  const CellRef C = findLeafCell(amr, P);
  float width     = C.width;
  if (isnan(width))
    width = 1.0f;
  return width;
}

export void EXPORT_UNIQUE(AMR_install_current, void *uniform _self)
{
  AMRVolume *uniform self     = (AMRVolume * uniform) _self;
  self->computeSample_varying = AMR_current;
  self->computeSampleLevel    = AMR_currentLevel;
}
