# cpsc4040-GLOIIO project3
#### OpenGL/GLUT Program Suite to read, display, modify, and write images using the OpenImageIO API
Dr. Karamouzas "Green screening"

by Owen Book (obook@clemson.edu)

*CPSC 4040 | Fall 2022*

### Compilation
Makefile is included. You can run these commands:

`make project3` or `make`: compile only alphamask and compose

`make all`: compile all code into the program suite

`make recompile`: clean compiled outputs and recompile all code into the program suite

`make [imgview, alphamask, compose]`: compile just one program at a time

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
**alphamask** can create and export transparent images. It works best with clear backgrounds such as greenscreens. This version does some alpha smoothing for colors that are just barely masked out so that the image appears less jagged.

#### Controls
The program will automatically display the resulting image once computation is complete.

Q or ESC: quit program


#### Command line usage
Load an image by including its file path, then specify an output file. The input file can be any image format as long as it uses RGBA channels(?).

From there, you must specify a "target" of HSV values (the color you want to mask out) and optionally a "fuzz" value of HSV values (the tolerance in difference for those colors). YOu may need to run the program several times, tweaking your inputs to get the desired result.

```./alphamask [input] [output].png [0~360] [0~1] [0~1] [0~360] [0~1] [0~1]```

If the input file does not exist or cannot be opened, the program will exit.

If .png is omitted from output, the program will append it automatically. You cannot print any other image format from this program - convert it with other software.

If not enough values are provided for target or fuzz, the program will ignore the rest. (It's not particularly helpful but at least you can see a result - a .cfg file was planned but trying to user-proof it was a nightmare)

#### My inputs for masking each image
D.House: 135.0 0.7 0.4 60.0 0.5 0.5 - there's a cloud of low alpha pixels to the left, but it's not a big issue

Hand: 90.0 0.5 0.5 40.0 0.6 0.6 - this one looks pretty clean!

Scientist: 130.0 0.7 0.6 40.0 0.7 0.6 - best I could get without wrecking parts of the image

## compose
**compose** draws one image over another, taking transparency into account. It does not support cropping - the background image *B* must be the same size as or larger than the foreground image *A*.

#### Controls
The program will automatically display the resulting image.

W: write result to file (prompt) 

Q or ESC: quit program


#### Command line usage
Load the foreground image A and background image B using their file paths. You can optionally specify an output file with any image format to write the result to that file automatically.

```./compose [A] [B] (output)```

If either input file does not exist or cannot be opened, the program will exit.

If the file extension is omitted from output, the program will assume .png format.
