/*
 * Copyright (C) 2003 Josh A. Beam
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __MY_MATH_H__
#define __MY_MATH_H__

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef cosf
#define cosf (float)cos
#endif

#ifndef sinf
#define sinf (float)sin
#endif

#ifndef sqrtf
#define sqrtf (float)sqrt
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#define DEG2RAD(a) ((float)(a) * (M_PI / 180.0f))
#define VEC_MAGNITUDE(v) (sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]))

void swap_matrix_major(float m[16]);
void multiply_matrix(float out[16], float m1[16], float m2[16]);
void transpose_matrix(float out[16], float m[16]);
void normalize(float v[3]);
float dot_product(float v1[3], float v2[3]);
void cross_product(float dst[3], float v1[3], float v2[3]);
void setup_plane(float plane[4], float v1[3], float v2[3], float v3[3], int normalize_dir);

#endif /* __MY_MATH_H__ */
