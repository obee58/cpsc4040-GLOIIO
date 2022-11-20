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
default: clean warper

project1: imgview
project3: alphamask compose
project4: convolve
project6: warper
all: imgview alphamask compose convolve warper
recompile: clean all

imgview:
	${CXX} ${CPPFLAGS} -o imgview src/imgview.cpp src/gloiioFuncs.cpp src/matrix.cpp ${LD}
alphamask:
	${CXX} ${CPPFLAGS} -o alphamask src/alphamask.cpp src/gloiioFuncs.cpp src/matrix.cpp ${LD}
compose:
	${CXX} ${CPPFLAGS} -o compose src/compose.cpp src/gloiioFuncs.cpp src/matrix.cpp ${LD}
convolve:
	${CXX} ${CPPFLAGS} -o convolve src/convolve.cpp src/gloiioFuncs.cpp src/matrix.cpp ${LD}
warper:
	${CXX} ${CPPFLAGS} -o warper src/warper.cpp src/gloiioFuncs.cpp src/matrixBuilder.cpp src/matrix.cpp ${LD}

clean:
	rm -f core.* *.o *~ imgview alphamask compose convolve warper
