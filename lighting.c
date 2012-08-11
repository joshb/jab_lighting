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
#include <GL/gl.h>
#include "my_math.h"

#define MAX_LIGHTS 16

struct light {
	float position[3];
	float size;
	float color[3];
};

static struct light *lights[MAX_LIGHTS];
static char lights_pointers_initialized = 0;

static int lightmap_gl_num = 0;

static void
lights_pointers_init()
{
	int i;

	if(lights_pointers_initialized)
		return;

	for(i = 0; i < MAX_LIGHTS; i++)
		lights[i] = NULL;

	lights_pointers_initialized = 1;
}

int
gen_lightmap_texture(float v[3], float d_x, float d_y, float down[3],
                     float right[3], unsigned char min)
{
	static unsigned char *data = NULL;
	static int lightmap_size = 16;
	int i, j;
	int index;
	float tmp[3];
	float d[3];
	float m, c;
	int n;
	int lit;

	lights_pointers_init();

	if(!data) {
		data = malloc(lightmap_size*lightmap_size * 3);
		if(!data) {
			fprintf(stderr, "Error: Couldn't allocate memory for lightmap data\n");
			return -1;
		}
	}

	lit = 0;
	for(n = 0; n < MAX_LIGHTS; n++) {
		if(!lights[n])
			continue;

		d[0] = lights[n]->position[0] - v[0];
		d[1] = lights[n]->position[1] - v[1];
		d[2] = lights[n]->position[2] - v[2];

		if(d[0]*d[0] + d[1]*d[1] + d[2]*d[2] > 80.0f*80.0f)
			continue;

		for(i = 0; i < lightmap_size; i++) {
			for(j = 0; j < lightmap_size; j++) {
				tmp[0] = v[0];
				tmp[1] = v[1];
				tmp[2] = v[2];

				tmp[0] += d_x * ((float)(j) / (float)lightmap_size) * right[0];
				tmp[1] += d_x * ((float)(j) / (float)lightmap_size) * right[1];
				tmp[2] += d_x * ((float)(j) / (float)lightmap_size) * right[2];

				tmp[0] += d_y * ((float)(i) / (float)lightmap_size) * down[0];
				tmp[1] += d_y * ((float)(i) / (float)lightmap_size) * down[1];
				tmp[2] += d_y * ((float)(i) / (float)lightmap_size) * down[2];

				d[0] = lights[n]->position[0] - tmp[0];
				d[1] = lights[n]->position[1] - tmp[1];
				d[2] = lights[n]->position[2] - tmp[2];

				m = (d[0]*d[0] + d[1]*d[1] + d[2]*d[2]) * (1.0f / lights[n]->size);
				if(m == 0.0f)
					m = 0.001f;
				c = 1.0f / m;
				if(c > 1.0f)
					c = 1.0f;

				index = i * lightmap_size * 3 + j * 3;
				if(!lit) {
					data[index + 0] = min + (255 - min) * c * lights[n]->color[0];
					data[index + 1] = min + (255 - min) * c * lights[n]->color[1];
					data[index + 2] = min + (255 - min) * c * lights[n]->color[2];
				} else { /* if texture already has some lighting, modify it instead of ignoring the existing lighting */
					data[index + 0] += (255 - data[index + 0]) * c * lights[n]->color[0];
					data[index + 1] += (255 - data[index + 1]) * c * lights[n]->color[1];
					data[index + 2] += (255 - data[index + 2]) * c * lights[n]->color[2];
				}
			}
		}
		lit = 1;
	}

	glBindTexture(GL_TEXTURE_2D, lightmap_gl_num);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, lightmap_size, lightmap_size, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

	if(lit)
		return lightmap_gl_num;
	else
		return -1;
}

/* dumb little sphere drawing function */
static void
draw_sphere(int skip, float x_size, float y_size, float z_size, unsigned char half, float x, float y, float z)
{
	int i, j;

	glPushMatrix();
	glTranslatef(x, y, z);
	glBegin(GL_QUADS);
	for(i = 0; i < 180; i += skip) {
		float c[2], s[2];

		c[0] = cosf(DEG2RAD(i));
		c[1] = cosf(DEG2RAD(i + skip));

		s[0] = sinf(DEG2RAD(i));
		s[1] = sinf(DEG2RAD(i + skip));

		for(j = 0; j < 360; j += skip) {
			glVertex3f(cosf(DEG2RAD(j)) * s[0] * x_size, sinf(DEG2RAD(j)) * s[0] * y_size, c[0] * z_size);
			glVertex3f(cosf(DEG2RAD(j + skip)) * s[0] * x_size, sinf(DEG2RAD(j + skip)) * s[0] * y_size, c[0] * z_size);
			glVertex3f(cosf(DEG2RAD(j + skip)) * s[1] * x_size, sinf(DEG2RAD(j + skip)) * s[1] * y_size, c[1] * z_size);
			glVertex3f(cosf(DEG2RAD(j)) * s[1] * x_size, sinf(DEG2RAD(j)) * s[1] * y_size, c[1] * z_size);
		}
	}
	glEnd();
	glPopMatrix();
}

/* draw small spheres to represent the lights */
void
render_lights()
{
	int i;

	lights_pointers_init();

	for(i = 0; i < MAX_LIGHTS; i++) {
		if(!lights[i])
			continue;

		glColor4f(lights[i]->color[0], lights[i]->color[1], lights[i]->color[2], 1.0f);
		draw_sphere(40, 0.05f, 0.05f, 0.05f, 0, lights[i]->position[0], lights[i]->position[1], lights[i]->position[2]);
	}
}

void
set_light_position(int n, float p[3])
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	lights[n]->position[0] = p[0];
	lights[n]->position[1] = p[1];
	lights[n]->position[2] = p[2];
}

void
get_light_position(int n, float p[3])
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	p[0] = lights[n]->position[0];
	p[1] = lights[n]->position[1];
	p[2] = lights[n]->position[2];
}

void
translate_light_position(int n, float p[3])
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	lights[n]->position[0] += p[0];
	lights[n]->position[1] += p[1];
	lights[n]->position[2] += p[2];
}

void
set_light_size(int n, float size)
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	lights[n]->size = size;
}

void
set_light_color(int n, float r, float g, float b)
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	lights[n]->color[0] = r;
	lights[n]->color[1] = g;
	lights[n]->color[2] = b;
}

int
create_light()
{
	int i;

	lights_pointers_init();

	for(i = 0; i < MAX_LIGHTS; i++) {
		if(!lights[i]) {
			lights[i] = malloc(sizeof(struct light));

			lights[i]->size = 40.0f;
			lights[i]->color[0] = 1.0f;
			lights[i]->color[1] = 1.0f;
			lights[i]->color[2] = 1.0f;

			return i;
		}
	}

	return -1;
}

void
destroy_light(int n)
{
	lights_pointers_init();

	if(n == -1 || !lights[n])
		return;

	free(lights[n]);
	lights[n] = NULL;
}
