# cpsc4040-GLOIIO (v1.b "warper")
#### OpenGL/GLUT Program Suite to read, display, modify, and write images using the OpenImageIO API
by Owen Book (obook@clemson.edu)

*CPSC 4040 | Dr. Karamouzas | Fall 2022*

### Compilation
Makefile is included. You can run these commands:

`make`: clean compiled outputs and compile only `convolve`

`make all`: compile all code into the program suite

`make recompile`: clean compiled outputs and recompile all code into the program suite

`make [imgview, alphamask, compose, convolve]`: compile just one program at a time

`make clean`: delete compiled outputs (will not touch images the program creates)

## imgview
**imgview** is a multi-purpose image viewer that comes with some functions to play around with. It can load multiple images at once and write modified images to files.

#### Controls
*Left/right arrows: swap between multiple loaded images*

R: read new image from file (prompt)

W: write current image to file (prompt)

I: invert colors of current image

N: randomly add black noise to current image

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

If not enough values are provided for target or fuzz, the program will ignore the rest. (It's not particularly helpful but at least you can see a result - a .cfg file was planned but trying to user-proof it is a nightmare)

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

## convolve
**convolve** allows you to apply simple global filters (.filt) to an image as many times as desired.

#### Controls
The program will automatically display the original image.

C: apply filter once

R: revert to original image (specifically, deletes the displayed data and copies the original data)

W: write result to file (prompts if not specified in command line)

Q or ESC: quit program

Note that applying filters will take some time depending on the image and filter sizes. It's not recommended to use large images with this program at the moment.


#### Command line usage
Load the desired filter file first, then the image you want to open. Additionally, you can specify your desired output filename from the command line instead of entering it upon pressing W.

```./convolve [filter].filt [image] (output)```

If either input file or filter file do not exist or cannot be opened, the program will exit. This specific program was designed for .png images foremost, but should theoretically work with most common formats.


#### .filt format
**.filt** files are plaintext data files that define a global convolution filter. Only numbers (integer or floating point, anything's fine) should be put inside each file - any non-numerical data may cause errors.

The first number in the file, designated *N*, informs the program of the size of the filter kernel, or its number of rows and columns. The program expects *N*^2 entries after that, separated by whitespace. The exact line usage does not matter, but keeping each row of the filter kernel seperate is recommended.

You can find various examples in the included `filters` directory.


#### Known issues
convolve currently copies the original pixel value to calculate values for pixels that would be outside the image. This may cause edges of images to be bizzarely colored or not change.

Additionally, the program currently uses a scale factor and clamping function to keep resulting values within 8 bits per channel. The original plan was to normalize the kernel when loading it, but I discarded this behavior during some confusion with calculations. The current method can make areas of some images too dark or too bright, especially if relatively large negative values are in the filter kernel.

Edge calculation filters in general do not work too well with this version of the program.

## warper
TODO