#for MacOS
#INCDIR=-I/opt/X11/include -I/opt/local/include
#LIBDIR=-L/opt/local/lib -L/opt/X11/lib
#LIBS=-framework OpenGL

#for linux
LIBS=-lGL -lGLU

all:
	gcc $(INCDIR) -g main.cpp -o gltest -g $(LIBDIR) $(LIBS) -lGLEW -lglut -lm

clean:
	rm shadedepth
