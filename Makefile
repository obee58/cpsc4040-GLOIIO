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
project3: alphamask compose

all: imgview alphamask compose

recompile: clean all

imgview:
	${CXX} ${CPPFLAGS} -o imgview src/imgview.cpp ${LD}
alphamask:
	${CXX} ${CPPFLAGS} -o alphamask src/alphamask.cpp ${LD}
compose:
	${CXX} ${CPPFLAGS} -o compose src/compose.cpp ${LD}

clean:
	rm -f core.* *.o *~ imgview alphamask compose