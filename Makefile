CC=gcc
CFLAGS=-Wall -pedantic -g -I/usr/X11R6/include -I/usr/local/include -funroll-loops
# CFLAGS+=-DUSE_3DNOW
LDFLAGS=-pthread -L/usr/X11R6/lib -L/usr/local/lib -lm -lX11 -lXmu -lXi -lXext -lGL -lGLU -lglut
OBJS=endian.o input.o lighting.o main.o md2.o my_math.o pcx.o scene.o

lighting:	$(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o main

clean:
	rm -f main
	rm -f $(OBJS)

endian.o: endian.c
input.o: input.c
lighting.o: lighting.c
main.o: main.c
md2.o: md2.c
my_math.o: my_math.c
pcx.o: pcx.c
scene.o: scene.c
