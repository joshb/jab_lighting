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
#include <math.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include "my_math.h"
#include "lighting.h"
#include "pcx.h"

#include "md2.h"

#define USE_STENCIL

int light = 1;

struct surface {
	int occluder;
	int tex_num;

	float texcoords[4][2];
	float vertices[4][3];
	float d_x, d_y;

	float down_vector[3];
	float right_vector[3];
};

static struct surface *surfaces = NULL;
static unsigned int num_surfaces = 0;

static int lights[3] = { -1, -1, -1 };
static float light_rot[3] = { 0.0f, 0.0f, 0.0f };

static float cam_pos[3] = { 0.0f, 0.0f, 0.0f };
static float cam_rot[3] = { 0.0f, -180.0f, 0.0f };

static struct md2_model *m = NULL;

static void
load_texture(int tex_num, char *filename)
{
	unsigned int width, height;
	unsigned char *data;

	data = read_pcx(filename, &width, &height);
	if(!data)
		exit(1);

	glBindTexture(GL_TEXTURE_2D, tex_num);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	free(data);
}

void
scene_load_model(char *md2_filename, char *pcx_filename)
{
	if(m)
		md2_free(m);

	m = md2_load(md2_filename);
	if(!m)
		exit(1);

	glEnable(GL_TEXTURE_2D);
	load_texture(4, pcx_filename);
}

void
scene_free()
{
	if(m)
		md2_free(m);
	if(surfaces)
		free(surfaces);

	destroy_light(lights[0]);
	destroy_light(lights[1]);
	destroy_light(lights[2]);
}

static void
render_stencil_shadow()
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0x0, 0xff);
	glStencilOp(GL_REPLACE, GL_KEEP, GL_KEEP);

	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, 1, 1, 0, 0, 1);
	glDisable(GL_DEPTH_TEST);

	glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
	glBegin(GL_QUADS);
		glVertex2f(0, 0);
		glVertex2f(1, 0);
		glVertex2f(1, 1);
		glVertex2f(0, 1);
	glEnd();

	glEnable(GL_DEPTH_TEST);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glDisable(GL_STENCIL_TEST);
}

static void
set_surfaces_vectors()
{
	int i;

	for(i = 0; i < num_surfaces; i++) {
		float tmp1[3], tmp2[3];

		/* down vector */
		tmp1[0] = surfaces[i].vertices[3][0] - surfaces[i].vertices[0][0];
		tmp1[1] = surfaces[i].vertices[3][1] - surfaces[i].vertices[0][1];
		tmp1[2] = surfaces[i].vertices[3][2] - surfaces[i].vertices[0][2];
		surfaces[i].d_y = sqrtf(tmp1[0]*tmp1[0] + tmp1[1]*tmp1[1] + tmp1[2]*tmp1[2]);
		normalize(tmp1);

		/* right vector */
		tmp2[0] = surfaces[i].vertices[1][0] - surfaces[i].vertices[0][0];
		tmp2[1] = surfaces[i].vertices[1][1] - surfaces[i].vertices[0][1];
		tmp2[2] = surfaces[i].vertices[1][2] - surfaces[i].vertices[0][2];
		surfaces[i].d_x = sqrtf(tmp2[0]*tmp2[0] + tmp2[1]*tmp2[1] + tmp2[2]*tmp2[2]);
		normalize(tmp2);

		surfaces[i].down_vector[0] = tmp1[0];
		surfaces[i].down_vector[1] = tmp1[1];
		surfaces[i].down_vector[2] = tmp1[2];

		surfaces[i].right_vector[0] = tmp2[0];
		surfaces[i].right_vector[1] = tmp2[1];
		surfaces[i].right_vector[2] = tmp2[2];
	}
}

static void
create_surfaces()
{
	int i;
	float tmp[3];

	num_surfaces = 6;
	surfaces = malloc(sizeof(struct surface) * num_surfaces);

	/* floor */
	surfaces[0].occluder = 0;
	surfaces[0].tex_num = 1;
	load_texture(surfaces[0].tex_num, "data/floor.pcx");

	surfaces[0].texcoords[0][0] = 0.0f; surfaces[0].texcoords[0][1] = 0.0f;
	surfaces[0].vertices[0][0] = -20.0f;
	surfaces[0].vertices[0][1] = -2.0f;
	surfaces[0].vertices[0][2] = 20.0f;

	surfaces[0].texcoords[1][0] = 10.0f; surfaces[0].texcoords[1][1] = 0.0f;
	surfaces[0].vertices[1][0] = 20.0f;
	surfaces[0].vertices[1][1] = -2.0f;
	surfaces[0].vertices[1][2] = 20.0f;

	surfaces[0].texcoords[2][0] = 10.0f; surfaces[0].texcoords[2][1] = 10.0f;
	surfaces[0].vertices[2][0] = 20.0f;
	surfaces[0].vertices[2][1] = -2.0f;
	surfaces[0].vertices[2][2] = -20.0f;

	surfaces[0].texcoords[3][0] = 0.0f; surfaces[0].texcoords[3][1] = 10.0f;
	surfaces[0].vertices[3][0] = -20.0f;
	surfaces[0].vertices[3][1] = -2.0f;
	surfaces[0].vertices[3][2] = -20.0f;

	/* ceiling */
	surfaces[1].occluder = 0;
	surfaces[1].tex_num = 2;
	load_texture(surfaces[1].tex_num, "data/ceiling.pcx");

	surfaces[1].texcoords[0][0] = 0.0f; surfaces[1].texcoords[0][1] = 0.0f;
	surfaces[1].vertices[0][0] = -20.0f;
	surfaces[1].vertices[0][1] = 2.0f;
	surfaces[1].vertices[0][2] = 20.0f;

	surfaces[1].texcoords[1][0] = 10.0f; surfaces[1].texcoords[1][1] = 0.0f;
	surfaces[1].vertices[1][0] = 20.0f;
	surfaces[1].vertices[1][1] = 2.0f;
	surfaces[1].vertices[1][2] = 20.0f;

	surfaces[1].texcoords[2][0] = 10.0f; surfaces[1].texcoords[2][1] = 10.0f;
	surfaces[1].vertices[2][0] = 20.0f;
	surfaces[1].vertices[2][1] = 2.0f;
	surfaces[1].vertices[2][2] = -20.0f;

	surfaces[1].texcoords[3][0] = 0.0f; surfaces[1].texcoords[3][1] = 10.0f;
	surfaces[1].vertices[3][0] = -20.0f;
	surfaces[1].vertices[3][1] = 2.0f;
	surfaces[1].vertices[3][2] = -20.0f;

	/* front wall */
	surfaces[2].occluder = 0;
	surfaces[2].tex_num = 3;
	load_texture(surfaces[2].tex_num, "data/wall.pcx");

	surfaces[2].texcoords[0][0] = 0.0f; surfaces[2].texcoords[0][1] = 0.0f;
	surfaces[2].vertices[0][0] = -20.0f;
	surfaces[2].vertices[0][1] = 2.0f;
	surfaces[2].vertices[0][2] = -20.0f;

	surfaces[2].texcoords[1][0] = 5.0f; surfaces[2].texcoords[1][1] = 0.0f;
	surfaces[2].vertices[1][0] = 20.0f;
	surfaces[2].vertices[1][1] = 2.0f;
	surfaces[2].vertices[1][2] = -20.0f;

	surfaces[2].texcoords[2][0] = 5.0f; surfaces[2].texcoords[2][1] = 0.5f;
	surfaces[2].vertices[2][0] = 20.0f;
	surfaces[2].vertices[2][1] = -2.0f;
	surfaces[2].vertices[2][2] = -20.0f;

	surfaces[2].texcoords[3][0] = 0.0f; surfaces[2].texcoords[3][1] = 0.5f;
	surfaces[2].vertices[3][0] = -20.0f;
	surfaces[2].vertices[3][1] = -2.0f;
	surfaces[2].vertices[3][2] = -20.0f;

	/* back wall */
	surfaces[3].occluder = 0;
	surfaces[3].tex_num = 3;

	surfaces[3].texcoords[0][0] = 0.0f; surfaces[3].texcoords[0][1] = 0.0f;
	surfaces[3].vertices[0][0] = -20.0f;
	surfaces[3].vertices[0][1] = 2.0f;
	surfaces[3].vertices[0][2] = 20.0f;

	surfaces[3].texcoords[1][0] = 5.0f; surfaces[3].texcoords[1][1] = 0.0f;
	surfaces[3].vertices[1][0] = 20.0f;
	surfaces[3].vertices[1][1] = 2.0f;
	surfaces[3].vertices[1][2] = 20.0f;

	surfaces[3].texcoords[2][0] = 5.0f; surfaces[3].texcoords[2][1] = 0.5f;
	surfaces[3].vertices[2][0] = 20.0f;
	surfaces[3].vertices[2][1] = -2.0f;
	surfaces[3].vertices[2][2] = 20.0f;

	surfaces[3].texcoords[3][0] = 0.0f; surfaces[3].texcoords[3][1] = 0.5f;
	surfaces[3].vertices[3][0] = -20.0f;
	surfaces[3].vertices[3][1] = -2.0f;
	surfaces[3].vertices[3][2] = 20.0f;

	/* left wall */
	surfaces[4].occluder = 0;
	surfaces[4].tex_num = 3;

	surfaces[4].texcoords[0][0] = 0.0f; surfaces[4].texcoords[0][1] = 0.0f;
	surfaces[4].vertices[0][0] = -20.0f;
	surfaces[4].vertices[0][1] = 2.0f;
	surfaces[4].vertices[0][2] = -20.0f;

	surfaces[4].texcoords[1][0] = 5.0f; surfaces[4].texcoords[1][1] = 0.0f;
	surfaces[4].vertices[1][0] = -20.0f;
	surfaces[4].vertices[1][1] = 2.0f;
	surfaces[4].vertices[1][2] = 20.0f;

	surfaces[4].texcoords[2][0] = 5.0f; surfaces[4].texcoords[2][1] = 0.5f;
	surfaces[4].vertices[2][0] = -20.0f;
	surfaces[4].vertices[2][1] = -2.0f;
	surfaces[4].vertices[2][2] = 20.0f;

	surfaces[4].texcoords[3][0] = 0.0f; surfaces[4].texcoords[3][1] = 0.5f;
	surfaces[4].vertices[3][0] = -20.0f;
	surfaces[4].vertices[3][1] = -2.0f;
	surfaces[4].vertices[3][2] = -20.0f;

	/* right wall */
	surfaces[5].occluder = 0;
	surfaces[5].tex_num = 3;

	surfaces[5].texcoords[0][0] = 0.0f; surfaces[5].texcoords[0][1] = 0.0f;
	surfaces[5].vertices[0][0] = 20.0f;
	surfaces[5].vertices[0][1] = 2.0f;
	surfaces[5].vertices[0][2] = -20.0f;

	surfaces[5].texcoords[1][0] = 5.0f; surfaces[5].texcoords[1][1] = 0.0f;
	surfaces[5].vertices[1][0] = 20.0f;
	surfaces[5].vertices[1][1] = 2.0f;
	surfaces[5].vertices[1][2] = 20.0f;

	surfaces[5].texcoords[2][0] = 5.0f; surfaces[5].texcoords[2][1] = 0.5f;
	surfaces[5].vertices[2][0] = 20.0f;
	surfaces[5].vertices[2][1] = -2.0f;
	surfaces[5].vertices[2][2] = 20.0f;

	surfaces[5].texcoords[3][0] = 0.0f; surfaces[5].texcoords[3][1] = 0.5f;
	surfaces[5].vertices[3][0] = 20.0f;
	surfaces[5].vertices[3][1] = -2.0f;
	surfaces[5].vertices[3][2] = -20.0f;

	/***********************/
	set_surfaces_vectors();

	lights[0] = create_light();
	set_light_color(lights[0], 1.0f, 1.0f, 1.0f);

	lights[1] = create_light();
	set_light_color(lights[1], 1.0f, 1.0f, 1.0f);

	lights[2] = create_light();
	set_light_color(lights[2], 1.0f, 1.0f, 1.0f);

	for(i = 0; i < 3; i++) {
		tmp[0] = cosf(DEG2RAD(light_rot[i])) * 15.0f;
		tmp[1] = 0.33f;
		tmp[2] = sinf(DEG2RAD(light_rot[i])) * 15.0f;
		if(i == 2) {
			tmp[0] *= 0.125f;
			tmp[2] = 0.0f;
		}
		set_light_position(lights[i], tmp);
	}
}

void
draw_scene()
{
	static float model_pos[3] = { 0.0f, -0.8f, 2.0f };
	static float model_rot[3] = { -90.0f, -90.0f, 0.0f };
	static unsigned int model_frame = 0;
	int i, j;
	int tex;
	float tmp[3];

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -10.0f);
	glRotatef(cam_rot[0], 1.0f, 0.0f, 0.0f);
	glRotatef(cam_rot[1], 0.0f, 1.0f, 0.0f);
	glRotatef(cam_rot[2], 0.0f, 0.0f, 1.0f);
	glTranslatef(cam_pos[0], cam_pos[1], cam_pos[2]);

	if(!surfaces) {
		glEnable(GL_TEXTURE_2D);
		create_surfaces();
	}

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	for(i = 0; i < num_surfaces; i++) {
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, surfaces[i].tex_num);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		if(light) {
			tex = gen_lightmap_texture(surfaces[i].vertices[0], surfaces[i].d_x, surfaces[i].d_y, surfaces[i].down_vector, surfaces[i].right_vector, 64);
			if(tex != -1) {
				glEnable(GL_TEXTURE_2D);
				glBindTexture(GL_TEXTURE_2D, tex);
				glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
			} else {
				glDisable(GL_TEXTURE_2D);
				glColor4f(0.25f, 0.25f, 0.25f, 1.0f);
			}
		} else {
			glDisable(GL_TEXTURE_2D);
			glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		}

		glBegin(GL_QUADS);
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, surfaces[i].texcoords[0]);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 0.0f);
			glVertex3fv(surfaces[i].vertices[0]);
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, surfaces[i].texcoords[1]);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 0.0f);
			glVertex3fv(surfaces[i].vertices[1]);
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, surfaces[i].texcoords[2]);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0f, 1.0f);
			glVertex3fv(surfaces[i].vertices[2]);
			glMultiTexCoord2fvARB(GL_TEXTURE0_ARB, surfaces[i].texcoords[3]);
			glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0f, 1.0f);
			glVertex3fv(surfaces[i].vertices[3]);
		glEnd();
	}

	glActiveTextureARB(GL_TEXTURE0_ARB);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE1_ARB);
	glDisable(GL_TEXTURE_2D);
	glActiveTextureARB(GL_TEXTURE0_ARB);
	render_lights();

	/* render textured md2 model */
	if(m) {
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glPushMatrix();
		glTranslatef(model_pos[0], model_pos[1], model_pos[2]);
		glRotatef(model_rot[2], 0.0f, 0.0f, -1.0f);
		glRotatef(model_rot[1], 0.0f, 1.0f, 0.0f);
		glRotatef(model_rot[0], 1.0f, 0.0f, 0.0f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 4);
		md2_render(m, model_frame);
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
	}

#ifdef USE_STENCIL
	if(light && m) {
		glClear(GL_STENCIL_BUFFER_BIT);

		glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		glDepthMask(GL_FALSE);

		glEnable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST);

		for(j = 0; j < 3; j++) {
			get_light_position(lights[j], tmp);
			md2_calculate_visible_tris(m, model_frame, model_pos, model_rot, tmp);

			/* render back faces, incrementing stencil on zfail... */
			glCullFace(GL_FRONT);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_INCR, GL_KEEP); /* INCR */
			md2_render_shadow_volume(m, model_frame, model_pos, model_rot, tmp);

			/* ... and render front faces, decrementing on zfail. */
			glCullFace(GL_BACK);
			glStencilFunc(GL_ALWAYS, 0x0, 0xff);
			glStencilOp(GL_KEEP, GL_DECR, GL_KEEP); /* DECR */
			md2_render_shadow_volume(m, model_frame, model_pos, model_rot, tmp);
		}

		glDisable(GL_STENCIL_TEST);
		glDisable(GL_CULL_FACE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		render_stencil_shadow();
		glDepthMask(GL_TRUE);
	}
#endif /* USE_STENCIL */

	glFlush();
	glutSwapBuffers();

	for(i = 0; i < 3; i++) {
		tmp[0] = cosf(DEG2RAD(light_rot[i])) * 15.0f;
		tmp[1] = 0.33f;
		tmp[2] = sinf(DEG2RAD(light_rot[i])) * 15.0f;
		if(i == 2) {
			tmp[0] *= 0.125f;
			tmp[2] = 0.0f;
		}
		set_light_position(lights[i], tmp);
	}

	cam_rot[1] += 0.15f;

	light_rot[0] += 0.5f;
	light_rot[1] -= 0.5f;
	light_rot[2] += 2.0f;
}
