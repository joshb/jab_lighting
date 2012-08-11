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

#ifndef __MD2_H__
#define __MD2_H__

#include "endian.h"

#define ANIM_STAND	0
#define ANIM_WALK	1
#define ANIM_JUMP	2
#define ANIM_FLIPOFF	3
#define ANIM_TAUNT	4
#define ANIM_CRSTAND	5
#define ANIM_CRWALK	6
#define ANIM_PAIN1	7
#define ANIM_DEATH1	8
#define ANIM_DEATH2	9
#define ANIM_DEATH3	10
#define ANIM_WAVE	11
#define ANIM_ATTACK	12

struct md2_triangle_vertex {
	uint8_t vertex[3];
	uint8_t lightNormalIndex;
};

struct md2_triangle {
	int16_t vertexIndices[3];
	int16_t textureIndices[3];
};

struct md2_tri_edge {
	char taken;

	short vertexIndices[2];
	char new_tri;
	short triangleIndices[2];
};

struct md2_tri_info {
	char visible;
	char drawn;
};

struct md2_frame {
	float scale[3];
	float translate[3];
	int8_t name[16];
	struct md2_triangle_vertex *vertices;
};

struct md2_glcommand_vertex {
	float s, t;
	int32_t vertexIndex;
};

struct md2_glcommand {
	int32_t type;
	int32_t num_vertices;
	struct md2_glcommand_vertex *vertices;
};

struct md2_model {
	char name[128];

	struct md2_frame *f;
	unsigned int num_frames;
	struct md2_triangle *t;
	struct md2_tri_info *t_info;
	unsigned int num_triangles;
	struct md2_tri_edge *t_edges;
	unsigned int num_edges;
	struct md2_glcommand *g;
	unsigned int num_glcommands;
};

/* functions */
void md2_get_animation_frames(unsigned int anim, unsigned int *start_frame, unsigned int *end_frame);
void md2_calculate_visible_tris(struct md2_model *mp, unsigned int frame, float model_pos[3], float model_rot[3], float p[3]);
void md2_render_shadow_volume(struct md2_model *mp, unsigned int frame, float model_pos[3], float model_rot[3], float light_pos[3]);
void md2_render(struct md2_model *mp, unsigned int frame);
struct md2_model *md2_load(const char *filename);
void md2_free(struct md2_model *mp);

#endif /* __MD2_H__ */
