# cpsc4040-GLOIIO project3
#### OpenGL/GLUT Programs to read, display, modify, and write images using the OpenImageIO API
Dr. Karamouzas "Green screening"

by Owen Book (obook@clemson.edu)

*CPSC 4040 | Fall 2022*

<<<<<<< HEAD


=======
>>>>>>> 58fba8520405c31f447bf08fdef83788ad006e7b
### Compilation
Included is a slightly modified version of Dr. Karamouzas' sample makefile.

`make`: compile the code into the program suite

`make clean`: delete compiled outputs (will not touch images the program creates)

## imgview
**imgview** is a multi-purpose image viewer that comes with some functions to play around with. It can load multiple images at once and write modified images to files.

#### Controls
*Left/right arrows: swap between multiple loaded images*

R: read new image from file (prompt)

W: write current image to file (prompt)

I: invert colors of current image

N: randomly add black noise to current image

*P: display the first set of bytes of the image data in hex*  

Q or ESC: quit program

#### Command line usage
Load images at launch by including their file paths as arguments. You can add as many files as you want. Any argument that is not a filename will be read as one.

```./imgview [filenames]```

If a file does not exist or cannot be opened, the program will notify you and ignore it. This goes for both reading and writing within the program as well.

## alphamask
**alphamask** can create and export transparent images. It works best with clear backgrounds such as greenscreens.

<<<<<<< HEAD

=======
>>>>>>> 58fba8520405c31f447bf08fdef83788ad006e7b
#### Controls
The program will automatically display the resulting image.

Q or ESC: quit program


#### Command line usage
Load an image by including its file path, then specify an output file. The input file can be any image format as long as it uses RGBA channels(?).

```./alphamask [input] [output].png```

If the input file does not exist or cannot be opened, the program will exit.

If .png is omitted from output, the program will append it automatically. You cannot print any other image format from this program - convert it with other software.

## compose
**compose** draws one image over another, taking transparency into account. It does not support cropping - the background image *B* must be the same size as or larger than the foreground image *A*.

<<<<<<< HEAD

=======
>>>>>>> 58fba8520405c31f447bf08fdef83788ad006e7b
#### Controls
The program will automatically display the resulting image.

W: write result to file (prompt) 

Q or ESC: quit program


#### Command line usage
Load the foreground image A and background image B using their file paths. You can optionally specify an output file with any image format to write the result to that file automatically.

```./compose [A] [B] (output)```

If either input file does not exist or cannot be opened, the program will exit.

If the file extension is omitted from output, the program will assume .png format.
