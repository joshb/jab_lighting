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
#include <GL/glu.h>
#include <GL/glut.h>

#define WINWIDTH  640
#define WINHEIGHT 480
#define FULLSCREEN 0

int window = -1;

extern void scene_load_model(char *md2_filename, char *pcx_filename);
extern void draw_scene();
extern int light;

extern void key_press(unsigned char, int, int);
extern void key_press_special(int, int, int);

/* makes a raw rgba screenshot */
void
screenshot()
{
    unsigned char *pixels;
    FILE *fp;
	int tmp;

    pixels = malloc(WINWIDTH * WINHEIGHT * 4);
    if(!pixels)
        return;

	tmp = light;
	light = 0;
	draw_scene();

    fp = fopen("screen0.raw", "wb");
    if(!fp) {
        free(pixels);
        return;
    }

    glReadPixels(0, 0, WINWIDTH, WINHEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    fwrite(pixels, (WINWIDTH * 4), WINHEIGHT, fp);
    fclose(fp);

	light = 1;
	draw_scene();

    fp = fopen("screen1.raw", "wb");
    if(!fp) {
        free(pixels);
        return;
    }

    glReadPixels(0, 0, WINWIDTH, WINHEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    fwrite(pixels, (WINWIDTH * 4), WINHEIGHT, fp);
    fclose(fp);

    free(pixels);
	light = tmp;

	printf("screenshot taken\n");
}

int
main(int argc, char *argv[])
{
	int stencil_bits;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
	glutInitWindowSize(WINWIDTH, WINHEIGHT);
	window = glutCreateWindow("jab_lighting OpenGL demo by Josh Beam - http://joshbeam.com/");
	if(FULLSCREEN)
		glutFullScreen();

	glutIdleFunc(draw_scene);
	glutDisplayFunc(draw_scene);
	glutKeyboardFunc(key_press);
	glutSpecialFunc(key_press_special);

	/* gl */
	glGetIntegerv(GL_STENCIL_BITS, &stencil_bits);
	if(stencil_bits < 1)
		fprintf(stderr, "Error: This program requires a stencil buffer for shadows, which your system apparently doesn't support. This will cause problems with shadows if you specified an md2 model to load.\n");
	else
		printf("Stencil bits: %d\n", stencil_bits);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClearDepth(1.0f);
	glClearStencil(0);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (float)WINWIDTH / (float)WINHEIGHT, 0.1f, 999.0f);
	glMatrixMode(GL_MODELVIEW);

	if(argc >= 3)
		scene_load_model(argv[1], argv[2]);

	glutMainLoop();
	return 0;
}
