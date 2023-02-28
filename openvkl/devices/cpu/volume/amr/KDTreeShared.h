// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

/*! ispc equivalent of the c++-side kdtree that we build over the
    boxes */
struct KDTreeNode
{
  vkl_uint32 dim_and_ofs;
  vkl_uint32 pos_or_numItems;
};
