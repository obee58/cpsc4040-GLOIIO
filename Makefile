CXX = g++ #compiler
CPPFLAGS = -g #flags

ifeq ("$(shell uname)", "Darwin")
  LD = -framework Foundation -framework GLUT -framework OpenGL -lOpenImageIO -lm
else	
  ifeq ("$(shell uname)", "Linux")
    LD = -L /usr/lib64/ -lglut -lGL -lGLU -lOpenImageIO -lm
  endif
endif

default: clean tonemap

project1: imgview
project3: alphamask compose
project4: convolve
project5: tonemap
all: imgview alphamask compose convolve tonemap
recompile: clean all

imgview:
	${CXX} ${CPPFLAGS} -o imgview src/imgview.cpp src/gloiioFuncs.cpp ${LD}
alphamask:
	${CXX} ${CPPFLAGS} -o alphamask src/alphamask.cpp src/gloiioFuncs.cpp ${LD}
compose:
	${CXX} ${CPPFLAGS} -o compose src/compose.cpp src/gloiioFuncs.cpp ${LD}
convolve:
	${CXX} ${CPPFLAGS} -o convolve src/convolve.cpp src/gloiioFuncs.cpp ${LD}
tonemap:
	${CXX} ${CPPFLAGS} -o tonemap src/tonemap.cpp src/gloiioFuncs.cpp ${LD}

clean:
	rm -f core.* *.o *~ imgview alphamask compose convolve tonemap