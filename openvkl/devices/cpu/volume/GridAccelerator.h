// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// bit count used to represent the brick width in macrocells
#define BRICK_WIDTH_BITCOUNT (4)

// brick width in macrocells
#define BRICK_WIDTH (1 << BRICK_WIDTH_BITCOUNT)

// brick count in macrocells
#define BRICK_CELL_COUNT (BRICK_WIDTH * BRICK_WIDTH * BRICK_WIDTH)

// bit count used to represent the macrocell width in volume cells
#define CELL_WIDTH_BITCOUNT (4)

// macrocell width in volume cells
#define CELL_WIDTH (1 << CELL_WIDTH_BITCOUNT)

// reciprocal of macrocell width in volume cells
#define RCP_CELL_WIDTH 1.f / CELL_WIDTH
