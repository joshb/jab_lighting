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
#include <stdlib.h>
#include <string.h>
#include <GL/gl.h>
#include "my_math.h"
#include "endian.h"
#include "md2.h"

#define INFINITY 150.0f

#define MAX_MODELS 16
#define MD2_SCALE 0.05f

struct md2_anim {
	unsigned int start_frame;
	unsigned int end_frame;
};

static struct md2_anim animations[] = {
	{ 0, 39 }, /* ANIM_STAND */
	{ 40, 45 }, /* ANIM_WALK */
	{ 66, 67 }, /* ANIM_JUMP */
	{ 72, 83 }, /* ANIM_FLIPOFF */
	{ 95, 111 }, /* ANIM_TAUNT */
	{ 135, 153 }, /* ANIM_CRSTAND */
	{ 154, 159 }, /* ANIM_CRWALK */
	{ 54, 57}, /* ANIM_PAIN1 */
	{ 178, 183 }, /* ANIM_DEATH1 */
	{ 184, 189 }, /* ANIM_DEATH2 */
	{ 190, 197 }, /* ANIM_DEATH3 */
	{ 112, 122 }, /* ANIM_WAVE */
	{ 46, 53 }, /* ANIM_ATTACK */
};
static unsigned int num_animations = sizeof(animations) / sizeof(struct md2_anim);

struct md2_header {
	int32_t magic;
	int32_t version;
	int32_t skinWidth;
	int32_t skinHeight;
	int32_t frameSize;
	int32_t numSkins;
	int32_t numVertices;
	int32_t numTexCoords;
	int32_t numTriangles;
	int32_t numGlCommands;
	int32_t numFrames;
	int32_t offsetSkins;
	int32_t offsetTexCoords;
	int32_t offsetTriangles;
	int32_t offsetFrames;
	int32_t offsetGlCommands;
	int32_t offsetEnd;
};

void
md2_get_animation_frames(unsigned int anim, unsigned int *start_frame,
                         unsigned int *end_frame)
{
	struct md2_anim *ap;

	if(anim >= num_animations) {
		fprintf(stderr, "Error: animation %d does not exist\n", anim);
		*start_frame = *end_frame = 0;
		return;
	}

	ap = &animations[anim];
	*start_frame = ap->start_frame;
	*end_frame = ap->end_frame;
}

static void
transform_vertex(float v[3], float pos[3], float rot[3])
{
	float tmp[3];
	float c, s;

	tmp[0] = v[0];
	tmp[1] = v[1];
	tmp[2] = v[2];
	c = cosf(DEG2RAD(rot[0]));
	s = sinf(DEG2RAD(rot[0]));
	v[2] = tmp[2] * c + tmp[1] * s;
	v[1] = tmp[1] * c - tmp[2] * s;

	tmp[0] = v[0];
	tmp[1] = v[1];
	tmp[2] = v[2];
	c = cosf(DEG2RAD(rot[1]));
	s = sinf(DEG2RAD(rot[1]));
	v[0] = tmp[0] * c + tmp[2] * s;
	v[2] = tmp[2] * c - tmp[0] * s;

	tmp[0] = v[0];
	tmp[1] = v[1];
	tmp[2] = v[2];
	c = cosf(DEG2RAD(rot[2]));
	s = sinf(DEG2RAD(rot[2]));
	v[0] = tmp[0] * c + tmp[1] * s;
	v[1] = tmp[1] * c - tmp[0] * s;

	v[0] += pos[0];
	v[1] += pos[1];
	v[2] += pos[2];
}

static void
get_vertex_from_index(struct md2_model *mp, unsigned int frame,
                      unsigned int index, float v[3])
{
	v[0] = (mp->f[frame].vertices[index].vertex[0] * mp->f[frame].scale[0] + mp->f[frame].translate[0]) * MD2_SCALE;
	v[1] = (mp->f[frame].vertices[index].vertex[1] * mp->f[frame].scale[1] + mp->f[frame].translate[1]) * MD2_SCALE;
	v[2] = (mp->f[frame].vertices[index].vertex[2] * mp->f[frame].scale[2] + mp->f[frame].translate[2]) * MD2_SCALE;
}

static struct md2_tri_edge *
get_edge_with_verts(struct md2_tri_edge *edges, unsigned int num_edges,
                    unsigned int v1, unsigned int v2)
{
	unsigned int i;

	for(i = 0; i < num_edges; i++) {
		if(!edges[i].taken) {
			edges[i].taken = 1;
			edges[i].new_tri = 1;
			edges[i].vertexIndices[0] = v1;
			edges[i].vertexIndices[1] = v2;

			return &edges[i];
		}

		if((edges[i].vertexIndices[0] == v1 && edges[i].vertexIndices[1] == v2) ||
		   (edges[i].vertexIndices[1] == v1 && edges[i].vertexIndices[0] == v2)) {
			edges[i].new_tri = 0;
			return &edges[i];
		}
	}

	fprintf(stderr, "Error: No room for edge, it looks like\n");
	exit(1);
	return NULL;
}

static void
get_edge_order_from_invisible_tri(struct md2_model *mp,
                                  struct md2_tri_edge *edge, unsigned int v[2])
{
	int n = 0;
	struct md2_triangle *t;

	if(mp->t_info[(edge->triangleIndices[0])].visible)
		n = 1;

	t = &(mp->t[(edge->triangleIndices[n])]);

	if((t->vertexIndices[0] == edge->vertexIndices[0] &&
	    t->vertexIndices[1] == edge->vertexIndices[1]) ||
	   (t->vertexIndices[0] == edge->vertexIndices[1] &&
	    t->vertexIndices[1] == edge->vertexIndices[0])) {
		v[0] = t->vertexIndices[0];
		v[1] = t->vertexIndices[1];
		return;
	}

	if((t->vertexIndices[1] == edge->vertexIndices[0] &&
	    t->vertexIndices[2] == edge->vertexIndices[1]) ||
	   (t->vertexIndices[1] == edge->vertexIndices[1] &&
	    t->vertexIndices[2] == edge->vertexIndices[0])) {
		v[0] = t->vertexIndices[1];
		v[1] = t->vertexIndices[2];
		return;
	}

	if((t->vertexIndices[2] == edge->vertexIndices[0] &&
	    t->vertexIndices[0] == edge->vertexIndices[1]) ||
	   (t->vertexIndices[2] == edge->vertexIndices[1] &&
	    t->vertexIndices[0] == edge->vertexIndices[0])) {
		v[0] = t->vertexIndices[2];
		v[1] = t->vertexIndices[0];
		return;
	}
}

/*
 * mark triangles that are visible from the specified point
 */
void
md2_calculate_visible_tris(struct md2_model *mp, unsigned int frame,
                           float model_pos[3], float model_rot[3],
                           float p[3])
{
	unsigned int i;

	for(i = 0; i < mp->num_triangles; i++) {
		float v[3][3];
		float plane[4];

		get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[0], v[0]);
		transform_vertex(v[0], model_pos, model_rot);
		get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[1], v[1]);
		transform_vertex(v[1], model_pos, model_rot);
		get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[2], v[2]);
		transform_vertex(v[2], model_pos, model_rot);

		setup_plane(plane, v[2], v[1], v[0], FALSE);
		if(dot_product(plane, p) + plane[3] > 0.0f)
			mp->t_info[i].visible = 1;
		else
			mp->t_info[i].visible = 0;

		mp->t_info[i].drawn = 0;
	}
}

void
md2_render_shadow_volume(struct md2_model *mp, unsigned int frame,
                         float model_pos[3], float model_rot[3],
                         float light_pos[3])
{
	unsigned int i, j;

	glFrontFace(GL_CW);

	/* silhouette edges */
	glBegin(GL_QUADS);
	for(i = 0; i < mp->num_edges && mp->t_edges[i].taken; i++) {
		struct md2_triangle *t[2];
		struct md2_tri_info *t_info[2];

		t[0] = mp->t + mp->t_edges[i].triangleIndices[0];
		t[1] = mp->t + mp->t_edges[i].triangleIndices[1];

		t_info[0] = mp->t_info + mp->t_edges[i].triangleIndices[0];
		t_info[1] = mp->t_info + mp->t_edges[i].triangleIndices[1];

		if(t_info[0]->visible != t_info[1]->visible) {
			float v[4][3];
			float n[2][3];
			unsigned int edge_v[2];

			get_edge_order_from_invisible_tri(mp, mp->t_edges + i, edge_v);

			get_vertex_from_index(mp, frame, edge_v[0], v[0]);
			transform_vertex(v[0], model_pos, model_rot);
			get_vertex_from_index(mp, frame, edge_v[1], v[1]);
			transform_vertex(v[1], model_pos, model_rot);

			n[0][0] = v[0][0] - light_pos[0];
			n[0][1] = v[0][1] - light_pos[1];
			n[0][2] = v[0][2] - light_pos[2];
			normalize(n[0]);

			v[2][0] = v[0][0] + INFINITY * n[0][0];
			v[2][1] = v[0][1] + INFINITY * n[0][1];
			v[2][2] = v[0][2] + INFINITY * n[0][2];

			n[1][0] = v[1][0] - light_pos[0];
			n[1][1] = v[1][1] - light_pos[1];
			n[1][2] = v[1][2] - light_pos[2];
			normalize(n[1]);

			v[3][0] = v[1][0] + INFINITY * n[1][0];
			v[3][1] = v[1][1] + INFINITY * n[1][1];
			v[3][2] = v[1][2] + INFINITY * n[1][2];

			glVertex3fv(v[3]);
			glVertex3fv(v[1]);
			glVertex3fv(v[0]);
			glVertex3fv(v[2]);
		}
	}
	glEnd();

	glFrontFace(GL_CCW);
	glBegin(GL_TRIANGLES);
	for(i = 0; i < mp->num_triangles; i++) {
		if(mp->t_info[i].visible) { /* close cap */
			float v[3][3];

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[0], v[0]);
			transform_vertex(v[0], model_pos, model_rot);

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[1], v[1]);
			transform_vertex(v[1], model_pos, model_rot);

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[2], v[2]);
			transform_vertex(v[2], model_pos, model_rot);

			glVertex3fv(v[0]);
			glVertex3fv(v[1]);
			glVertex3fv(v[2]);
		} else { /* far cap */
			float v[3][3];
			float n[3];

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[0], v[0]);
			transform_vertex(v[0], model_pos, model_rot);

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[1], v[1]);
			transform_vertex(v[1], model_pos, model_rot);

			get_vertex_from_index(mp, frame, mp->t[i].vertexIndices[2], v[2]);
			transform_vertex(v[2], model_pos, model_rot);

			for(j = 0; j < 3; j++) {
				n[0] = v[j][0] - light_pos[0];
				n[1] = v[j][1] - light_pos[1];
				n[2] = v[j][2] - light_pos[2];
				normalize(n);

				v[j][0] += INFINITY * n[0];
				v[j][1] += INFINITY * n[1];
				v[j][2] += INFINITY * n[2];
			}

			glVertex3fv(v[0]);
			glVertex3fv(v[1]);
			glVertex3fv(v[2]);
		}
	}
	glEnd();

	glFrontFace(GL_CCW);
}

void
md2_render(struct md2_model *mp, unsigned int frame)
{
	int i, j;

	if(!mp || frame > mp->num_frames)
		return;

	glPushMatrix();
	glTranslatef(mp->f[frame].translate[0] * MD2_SCALE, mp->f[frame].translate[1] * MD2_SCALE, mp->f[frame].translate[2] * MD2_SCALE);

	for(i = 0; i < mp->num_glcommands; i++) {
		glBegin((mp->g[i].type ? GL_TRIANGLE_FAN : GL_TRIANGLE_STRIP));
		for(j = 0; j < mp->g[i].num_vertices; j++) {
			float v[3];

			v[0] = mp->f[frame].vertices[(mp->g[i].vertices[j].vertexIndex)].vertex[0] * mp->f[frame].scale[0] * MD2_SCALE;
			v[1] = mp->f[frame].vertices[(mp->g[i].vertices[j].vertexIndex)].vertex[1] * mp->f[frame].scale[1] * MD2_SCALE;
			v[2] = mp->f[frame].vertices[(mp->g[i].vertices[j].vertexIndex)].vertex[2] * mp->f[frame].scale[2] * MD2_SCALE;

			glTexCoord2f(mp->g[i].vertices[j].s, mp->g[i].vertices[j].t);
			glVertex3fv(v);
		}
		glEnd();
	}

	glPopMatrix();
}

static void
my_bzero(void *p, unsigned int size)
{
	unsigned int i;

	for(i = 0; i < size; i++)
		((int8_t *)p)[i] = 0;
}

struct md2_model *
md2_load(const char *filename)
{
	unsigned int i, j;
	FILE *fp;
	struct md2_header m;
	struct md2_model *mp;

	mp = malloc(sizeof(struct md2_model));
	if(!mp) {
		fprintf(stderr, "Error: Couldn't allocate memory for md2 model\n");
		return NULL;
	}
	snprintf(mp->name, 128, "%s", filename);

	fp = fopen(filename, "rb");
	if(!fp) {
		fprintf(stderr, "Error: Can't open %s\n", filename);
		return NULL;
	}

	fread(&m, sizeof(struct md2_header), 1, fp);
	for(i = 0; i < sizeof(struct md2_header) / sizeof(int); i++) {
		int32_t *p = (int32_t *)&m.magic;

		p += i;
		*p = le_to_native_int(*p);
	}

	/* load frames */
	mp->f = malloc(sizeof(struct md2_frame) * m.numFrames);
	if(!mp->f) {
		fprintf(stderr, "Error: Couldn't allocate memory for frames\n");
		return NULL;
	}
	fseek(fp, m.offsetFrames, SEEK_SET);
	for(i = 0; i < m.numFrames; i++) {
		mp->f[i].vertices = malloc(sizeof(struct md2_triangle_vertex) * m.numVertices);
		fread(&(mp->f[i]), sizeof(struct md2_frame) - sizeof(void *), 1, fp);
		mp->f[i].scale[0] = le_to_native_float(mp->f[i].scale[0]);
		mp->f[i].scale[1] = le_to_native_float(mp->f[i].scale[1]);
		mp->f[i].scale[2] = le_to_native_float(mp->f[i].scale[2]);
		mp->f[i].translate[0] = le_to_native_float(mp->f[i].translate[0]);
		mp->f[i].translate[1] = le_to_native_float(mp->f[i].translate[1]);
		mp->f[i].translate[2] = le_to_native_float(mp->f[i].translate[2]);
		fread(mp->f[i].vertices, sizeof(struct md2_triangle_vertex), m.numVertices, fp);
	}

	/* load triangles */
	mp->t = malloc(sizeof(struct md2_triangle) * m.numTriangles);
	if(!mp->t) {
		fprintf(stderr, "Error: Couldn't allocate memory for triangles\n");
		return NULL;
	}
	fseek(fp, m.offsetTriangles, SEEK_SET);
	fread(mp->t, sizeof(struct md2_triangle), m.numTriangles, fp);
	for(i = 0; i < m.numTriangles; i++) {
		mp->t[i].vertexIndices[0] = le_to_native_short(mp->t[i].vertexIndices[0]);
		mp->t[i].vertexIndices[1] = le_to_native_short(mp->t[i].vertexIndices[1]);
		mp->t[i].vertexIndices[2] = le_to_native_short(mp->t[i].vertexIndices[2]);

		mp->t[i].textureIndices[0] = le_to_native_short(mp->t[i].textureIndices[0]);
		mp->t[i].textureIndices[1] = le_to_native_short(mp->t[i].textureIndices[1]);
		mp->t[i].textureIndices[2] = le_to_native_short(mp->t[i].textureIndices[2]);
	}
	mp->num_triangles = m.numTriangles;

	mp->t_edges = malloc(sizeof(struct md2_tri_edge) * m.numTriangles * 3);
	if(!mp->t_edges) {
		fprintf(stderr, "Error: Couldn't allocate memory for triangle edges\n");
		return NULL;
	}
	mp->num_edges = m.numTriangles * 3;
	my_bzero(mp->t_edges, sizeof(struct md2_tri_edge) * mp->num_edges);

	mp->t_info = malloc(sizeof(struct md2_tri_info) * m.numTriangles);
	if(!mp->t_info) {
		fprintf(stderr, "Error: Couldn't allocate memory for triangle info\n");
		return NULL;
	}
	my_bzero(mp->t_info, sizeof(struct md2_tri_info) * m.numTriangles);

	/* set up edges */
	for(i = 0; i < m.numTriangles; i++) {
		struct md2_tri_edge *edge;

		edge = get_edge_with_verts(mp->t_edges, mp->num_edges, mp->t[i].vertexIndices[0], mp->t[i].vertexIndices[1]);
		if(edge->new_tri)
			edge->triangleIndices[0] = i;
		else
			edge->triangleIndices[1] = i;

		edge = get_edge_with_verts(mp->t_edges, mp->num_edges, mp->t[i].vertexIndices[1], mp->t[i].vertexIndices[2]);
		if(edge->new_tri)
			edge->triangleIndices[0] = i;
		else
			edge->triangleIndices[1] = i;

		edge = get_edge_with_verts(mp->t_edges, mp->num_edges, mp->t[i].vertexIndices[2], mp->t[i].vertexIndices[0]);
		if(edge->new_tri)
			edge->triangleIndices[0] = i;
		else
			edge->triangleIndices[1] = i;
	}

	/* load glcommands */
	mp->g = malloc(sizeof(struct md2_glcommand) * m.numGlCommands);
	fseek(fp, m.offsetGlCommands, SEEK_SET);
	for(i = 0; i < m.numGlCommands; i++) {
		int32_t num;

		fread(&num, 4, 1, fp);
		num = le_to_native_int(num);
		if(num < 0) {
			mp->g[i].type = 1;
			num = -num;
		} else {
			mp->g[i].type = 0;
		}
		mp->g[i].num_vertices = num;
		mp->g[i].vertices = malloc(sizeof(struct md2_glcommand_vertex) * num);
		fread(mp->g[i].vertices, sizeof(struct md2_glcommand_vertex), num, fp);
		for(j = 0; j < mp->g[i].num_vertices; j++) {
			mp->g[i].vertices[j].s = le_to_native_float(mp->g[i].vertices[j].s);
			mp->g[i].vertices[j].t = le_to_native_float(mp->g[i].vertices[j].t);
			mp->g[i].vertices[j].vertexIndex = le_to_native_uint(mp->g[i].vertices[j].vertexIndex);
		}
	}

	mp->num_frames = m.numFrames;
	mp->num_glcommands = m.numGlCommands;

	fclose(fp);
	return mp;
}

void
md2_free(struct md2_model *mp)
{
	if(!mp)
		return;

	if(mp->f)
		free(mp->f);
	if(mp->t)
		free(mp->t);
	if(mp->t_info)
		free(mp->t_info);
	if(mp->t_edges)
		free(mp->t_edges);
	if(mp->g)
		free(mp->g);
	free(mp);
}
