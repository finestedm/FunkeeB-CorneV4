/*
 * Frames adapted from SamIAm2000/zmk, branch bongo-cat-dedicated-work-queue.
 * Copyright (c) 2021 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <stdint.h>

#define BONGO_FRAME_WIDTH 68
#define BONGO_FRAME_HEIGHT 36
#define BONGO_FRAME_STRIDE_BYTES ((BONGO_FRAME_WIDTH + 7) / 8)
#define BONGO_FRAME_BYTES (BONGO_FRAME_STRIDE_BYTES * BONGO_FRAME_HEIGHT)
#define BONGO_IDLE_FRAME_COUNT 5

extern const uint8_t bongo_idle_frames[BONGO_IDLE_FRAME_COUNT][BONGO_FRAME_BYTES];
extern const uint8_t bongo_tap_frames[2][BONGO_FRAME_BYTES];
