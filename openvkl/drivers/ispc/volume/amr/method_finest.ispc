// Copyright 2019-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AMRVolume.ih"
#include "CellRef.ih"
#include "DualCell.ih"
#include "common/export_util.h"

varying float AMR_finest(const AMRVolume *uniform self, const varying vec3f &P)
{
  const AMR *uniform amr = &self->amr;

  vec3f lP;  // local amr space
  AMRVolume_transformObjectToLocal(self, P, lP);

  DualCell D;
  initDualCell(D, lP, *amr->finestLevel);
  findDualCell(amr, D);
  return lerp(D);
}

export void EXPORT_UNIQUE(AMR_install_finest, void *uniform _self)
{
  AMRVolume *uniform self     = (AMRVolume * uniform) _self;
  self->computeSample_varying = AMR_finest;
}
