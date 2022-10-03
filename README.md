# cpsc4040-GLOIIO
Dr. Karamouzas "Get the picture"

Program by Owen Book (obook@clemson.edu)

*CPSC 4040 | Fall 2022*

**Simple OpenGL/GLUT Program to read, display, modify, and write images using the OpenImageIO API**

### Compilation
Included is a slightly modified version of Dr. Karamouzas' sample makefile.

`make`: compile the code

`make run`: run the program without arguments

`make clean`: delete compiled outputs (will not touch images the program creates)

### Controls
*Left/right arrows: swap between multiple loaded images*

R: read new image from file (prompt)

W: write current image to file (prompt)

I: invert colors of current image

N: randomly add black noise to current image

*P: display the first set of bytes of the image data in hex*  

Q or ESC: quit program

### Command line usage
Load images at launch by including their file paths as arguments. You can add as many files as you want. Any argument that is not a filename will be read as one.

```./imgview [filenames]```

If a file does not exist or cannot be opened, the program will notify you and ignore it. This goes for both reading and writing within the program as well.
