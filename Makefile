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
# just copy the line and replace the filenames
default:
	${CXX} ${CPPFLAGS} -o alphamask src/alphamask.cpp ${LD}
	${CXX} ${CPPFLAGS} -o compose src/compose.cpp ${LD}

clean:
	rm -f core.* *.o *~ imgview alphamask compose