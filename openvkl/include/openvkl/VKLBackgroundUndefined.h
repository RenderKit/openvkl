// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ispc_cpp_interop.h"

// A special value we use to distinguish an undefined field value. This could
// be the result of sampling out of bounds, or sampling at a position in bounds
// but not covered by any input data.
// This value is a quiet NaN.
#define VKL_BACKGROUND_UNDEFINED floatbits(0xFFC068B5u)

