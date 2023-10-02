// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ispc {

  struct FindStack
  {
    bool active;
    int32 nodeID;
  };

  inline FindStack *pushStack(FindStack *stackPtr, int nodeID)
  {
    stackPtr->active = false;

    stackPtr->active = true;
    stackPtr->nodeID = nodeID;
    return stackPtr + 1;
  }

  inline FindStack *pushStack(FindStack *stackPtr, int nodeID, bool valid)
  {
    stackPtr->active = false;

    stackPtr->active = valid;
    stackPtr->nodeID = nodeID;
    return stackPtr + 1;
  }

}  // namespace ispc
