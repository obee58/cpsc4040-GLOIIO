CXX = g++ #compiler
CPPFLAGS = -g #flags

ifeq ("$(shell uname)", "Darwin")
  LD = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else	
  ifeq ("$(shell uname)", "Linux")
    LD = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

# I have spent an hour on fixing this makefile and make refuses to read a list one-by-one
# just copy the lines and replace the filenames
default: clean steno

project1: imgview
project3: alphamask compose
project4: convolve
final: steno
all: imgview alphamask compose convolve steno
recompile: clean all

imgview:
	${CXX} ${CPPFLAGS} -o imgview.out src/imgview.cpp src/gloiioFuncs.cpp ${LD}
alphamask:
	${CXX} ${CPPFLAGS} -o alphamask.out src/alphamask.cpp src/gloiioFuncs.cpp ${LD}
compose:
	${CXX} ${CPPFLAGS} -o compose.out src/compose.cpp src/gloiioFuncs.cpp ${LD}
convolve:
	${CXX} ${CPPFLAGS} -o convolve.out src/convolve.cpp src/gloiioFuncs.cpp ${LD}
steno:
	${CXX} ${CPPFLAGS} -o steno.out src/steno.cpp src/gloiioFuncs.cpp ${LD}

clean:
	rm -f core.* *.o *~ imgview alphamask compose convolve steno
