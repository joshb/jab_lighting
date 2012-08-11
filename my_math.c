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

#include <stdio.h>
#include <math.h>
#include "my_math.h"

/* column major to row major and vice-versa */
void
swap_matrix_major(float m[16])
{
	int i;
	float out[16];

	out[0] = m[0];
	out[1] = m[4];
	out[2] = m[8];
	out[3] = m[12];

	out[4] = m[1];
	out[5] = m[5];
	out[6] = m[9];
	out[7] = m[13];

	out[8] = m[2];
	out[9] = m[6];
	out[10] = m[10];
	out[11] = m[14];

	out[12] = m[3];
	out[13] = m[7];
	out[14] = m[11];
	out[15] = m[15];

	for(i = 0; i < 16; i++)
		m[i] = out[i];
}

void
multiply_matrix(float out[16], float m1[16], float m2[16])
{
	out[0] = m1[0]*m2[0] + m1[1]*m2[4] + m1[2]*m2[8] + m1[3]*m2[12];
	out[1] = m1[0]*m2[1] + m1[1]*m2[5] + m1[2]*m2[9] + m1[3]*m2[13];
	out[2] = m1[0]*m2[2] + m1[1]*m2[6] + m1[2]*m2[10] + m1[3]*m2[14];
	out[3] = m1[0]*m2[3] + m1[1]*m2[7] + m1[2]*m2[11] + m1[3]*m2[15];

	out[4] = m1[4]*m2[0] + m1[5]*m2[4] + m1[6]*m2[8] + m1[7]*m2[12];
	out[5] = m1[4]*m2[1] + m1[5]*m2[5] + m1[6]*m2[9] + m1[7]*m2[13];
	out[6] = m1[4]*m2[2] + m1[5]*m2[6] + m1[6]*m2[10] + m1[7]*m2[14];
	out[7] = m1[4]*m2[3] + m1[5]*m2[7] + m1[6]*m2[11] + m1[7]*m2[15];

	out[8] = m1[8]*m2[0] + m1[9]*m2[4] + m1[10]*m2[8] + m1[11]*m2[12];
	out[9] = m1[8]*m2[1] + m1[9]*m2[5] + m1[10]*m2[9] + m1[11]*m2[13];
	out[10] = m1[8]*m2[2] + m1[9]*m2[6] + m1[10]*m2[10] + m1[11]*m2[14];
	out[11] = m1[8]*m2[3] + m1[9]*m2[7] + m1[10]*m2[11] + m1[11]*m2[15];

	out[12] = m1[12]*m2[0] + m1[13]*m2[4] + m1[14]*m2[8] + m1[15]*m2[12];
	out[13] = m1[12]*m2[1] + m1[13]*m2[5] + m1[14]*m2[9] + m1[15]*m2[13];
	out[14] = m1[12]*m2[2] + m1[13]*m2[6] + m1[14]*m2[10] + m1[15]*m2[14];
	out[15] = m1[12]*m2[3] + m1[13]*m2[7] + m1[14]*m2[11] + m1[15]*m2[15];
}

void
transpose_matrix(float out[16], float m[16])
{
	int i, j;

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++)
			out[i*4 + j] = m[j*4 + i];
	}
}

void
normalize(float v[3])
{
#ifdef USE_3DNOW
	float scale[2];

	scale[0] = scale[1] = (v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);

	__asm__ __volatile__(
	"femms\n\t"
	"movq (%1), %%mm0\n\t"
	"pfrsqrt %%mm0, %%mm0\n\t"
	"movq %%mm0, (%1)\n\t"

	"movq (%0), %%mm0\n\t"
	"movq (%1), %%mm1\n\t"
	"pfmul %%mm1, %%mm0\n\t"
	"movq %%mm0, (%0)\n\t"
	"femms\n\t"
	:
	: "a" (v), "d" (scale));
	v[2] *= scale[0];
#else
	float scale;

	scale = 1.0f / sqrtf(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
	v[0] *= scale;
	v[1] *= scale;
	v[2] *= scale;
#endif /* USE_3DNOW */
}

float
dot_product(float v1[3], float v2[3])
{
#ifdef USE_3DNOW
	float v[2];

	__asm__ __volatile__(
	"femms\n\t"
	"movq (%1), %%mm0\n\t"
	"movq (%2), %%mm1\n\t"
	"pfmul %%mm1, %%mm0\n\t"
	"movq %%mm0, (%0)\n\t"
	"femms\n\t"
	:
	: "a" (v), "d" (v1), "c" (v2));

	return (v[0] + v[1] + v1[2]*v2[2]);
#else 
	return (v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2]);
#endif /* USE_3DNOW */
}

void
cross_product(float dst[3], float v1[3], float v2[3])
{
	dst[0] = v1[1] * v2[2] - v1[2] * v2[1];
	dst[1] = v1[2] * v2[0] - v1[0] * v2[2];
	dst[2] = v1[0] * v2[1] - v1[1] * v2[0];
}

/* create a plane from three vertices */
void
setup_plane(float plane[4], float v1[3], float v2[3], float v3[3],
            int normalize_dir)
{
	float normal[3];
	float d1[3], d2[3];

	d1[0] = v2[0] - v1[0];
	d1[1] = v2[1] - v1[1];
	d1[2] = v2[2] - v1[2];

	d2[0] = v3[0] - v1[0];
	d2[1] = v3[1] - v1[1];
	d2[2] = v3[2] - v1[2];

	cross_product(normal, d2, d1);
	if(normalize_dir)
		normalize(normal);

	plane[0] = normal[0];
	plane[1] = normal[1];
	plane[2] = normal[2];
	plane[3] = -(normal[0] * v1[0] + normal[1] * v1[1] + normal[2] * v1[2]);
}
