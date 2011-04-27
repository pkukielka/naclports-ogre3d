/*
 * Copyright (c) 2011 The Native Client Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */
#ifndef NATIVE_CLIENT_TESTS_EARTH_EARTH_H_
#define NATIVE_CLIENT_TESTS_EARTH_EARTH_H_

#include <stdint.h>
#include <string.h>

/* NaCl Earth demo */
#ifdef __cplusplus
extern "C" {
#endif

#define EARTH_A_SHIFT 24
#define EARTH_R_SHIFT 16
#define EARTH_G_SHIFT 8
#define EARTH_B_SHIFT 0

#define EARTH_R_MASK (0xff << EARTH_R_SHIFT)
#define EARTH_G_MASK (0xff << EARTH_G_SHIFT)
#define EARTH_B_MASK (0xff << EARTH_B_SHIFT)

void DebugPrintf(const char *fmt, ...);
void Earth_Init(int argcount, const char *argname[], const char *argvalue[]);
void Earth_Draw(uint32_t* image_data, int width, int height);
void Earth_Sync();

#ifdef __cplusplus
}
#endif

#endif  /* NATIVE_CLIENT_TESTS_EARTH_EARTH_H_ */
