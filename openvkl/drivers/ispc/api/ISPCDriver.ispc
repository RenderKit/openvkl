// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../common/export_util.h"

export uniform int32 EXPORT_UNIQUE(ISPCDriver_getProgramCount)
{
  return programCount;
}
