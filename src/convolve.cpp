//	OpenGL/GLUT Program to do simple image composition of image A over image B
//	Displays resulting image when done with optional export to file
//
//	Usage: compose [foreground].png [background] (output)
//	Inputs can be any image type, but it's recommended the foreground is from running alphamask.
//	Foreground must be same size or smaller than background!
//	There is currently no function to position the foreground image elsewhere.
//
//	CPSC 4040 | Owen Book | October 2022

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <ifstream>
#include <string>
#include <cmath>

#ifdef __APPLE__
#  pragma clang diagnostic ignored "-Wdeprecated-declarations"
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif

using namespace std;
OIIO_NAMESPACE_USING

/** CONSTANTS & DEFINITIONS **/
//default window dimensions
#define DEFAULT_WIDTH 600	
#define DEFAULT_HEIGHT 600
//assumed maximum value of pixel data - change if modifying for int, float, etc.
#define MAX_VAL 255
//preprocess macros
#define percentOf(a,max) (double)a/max
#define maximum(x,y,z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x,y,z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

//struct that holds RGBA 4-tuple of chars
typedef struct pixel_rgba_t {
	unsigned char red, green, blue, alpha;
} pxRGBA;
//struct that holds RGB 3-tuple of chars
typedef struct pixel_rgb_t {
	unsigned char red, green, blue;
} pxRGB;
//struct that holds HSV 3-tuple of doubles
typedef struct pixel_hsv_t {
	double hue, saturation, value;
} pxHSV;
//struct that holds RGBA 4-tuple of doubles (0~1)
typedef struct floating_rgba_t {
	double red, green, blue, alpha;
} flRGBA;

//struct to tie image spec and pixels together
typedef struct image_rgba_uint8_t {
	ImageSpec spec;
	pxRGBA* pixels;
} ImageRGBA;
//struct representing raw .filt data before conversion
typedef struct convolve_filt_t {
	int size; //NxN
	double* kernel;
} RawFilter;
//struct representing convolution filter, converted to -1~1 floating values and scale factor
typedef struct convolve_filt_normalized_t {
	int size; //NxN
	double scalefactor;
	double* kernel;
} NormFilter;


/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ImageRGBA> imageCache;
//list of loaded filters (only one is used here but would be a nice function to add in the future)
static vector<NormFilter> filtCache;
//current index in vector to draw/use/modify
static int imageIndex = 0;
static int filtIndex = 0;

//TODO move these to separate file as library?
/** UTILITY FUNCTIONS **/
/*	clean up memory of unneeded ImageRGBA
	don't forget to remove it from imageCache first! */
void discardImage(ImageRGBA image) {
	delete image.pixels;
}
/*	clean up memory of unneeded RawFilter
 *	call this after converting to NormFilter */
void discardRawFilt(RawFilter filt) {
	delete filt.kernel;
}
/*	clean up memory of unneeded NormFilter
 *	don't forget to remove it from filtCache first! */
void discardNormFilt(NormFilter filt) {
	delete filt.kernel;
}

/* functions to quickly create pxRGB, pxRGBA, pxHSV,... from arbitrary values*/
pxRGB linkRGB(unsigned char r, unsigned char g, unsigned char b) {
	pxRGB px;
	px.red = r;
	px.green = g;
	px.blue = b;
	return px;
}
pxRGBA linkRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	pxRGBA px;
	px.red = r;
	px.green = g;
	px.blue = b;
	px.alpha = a;
	return px;
}
pxHSV linkHSV(double h, double s, double v) {
	pxHSV px;
	px.hue = h;
	px.saturation = s;
	px.value = v;
	return px;
}

/* get percents of maximum from pxRGBA as a flRGBA */
flRGBA percentify(pxRGBA px) {
	flRGBA pct;
	pct.red = double(px.red)/MAX_VAL;
	pct.green = double(px.green)/MAX_VAL;
	pct.blue = double(px.blue)/MAX_VAL;
	pct.alpha = double(px.alpha)/MAX_VAL;
	return pct;
}

/* get premultiplied/associated version of a pxRGBA */
pxRGBA premult(pxRGBA pure) {
	double a = double(pure.alpha)/MAX_VAL; //alpha 0~1
	unsigned char r = pure.red*a;
	unsigned char g = pure.green*a;
	unsigned char b = pure.blue*a;
	return linkRGBA(r,g,b,pure.alpha);
}

/*convert RGB values into HSV values*/
pxHSV RGBtoHSV(pxRGB rgb) {
	pxHSV hsv;
	double huer, hueg, hueb, max, min, delta;
	/* convert from 0-MAX_VAL to 0~1 */
	huer = rgb.red / double(MAX_VAL);
	hueg = rgb.green / double(MAX_VAL);
	hueb = rgb.blue / double(MAX_VAL);

	max = maximum(huer, hueg, hueb);
	min = minimum(huer, hueg, hueb);
	hsv.value = max;

	if (max==0) {
		hsv.hue = 0;
		hsv.value = 0;
	}
	else {
		delta = max-min;
		hsv.saturation = delta/max; //https://youtu.be/QYbG1AMSdiY
		if (delta == 0) {
			hsv.hue = 0;
		}
		else {
			/* determine hue */
			if (huer == max) {
				hsv.hue = (hueg - hueb) / delta;
			}
			else if (hueg == max) {
				hsv.hue = 2.0 + (hueb - huer) / delta;
			}
			else {
				hsv.hue = 4.0 + (huer - hueg) / delta;
			}
			hsv.hue *= 60.0;
			if (hsv.hue < 0) {
				hsv.hue += 360.0;
			}
		}
	}

	return hsv;
}

/* shorthand function that double-converts RGBA to HSV, ignoring alpha channel */
pxHSV RGBAtoHSV(pxRGBA rgba) {
	return RGBtoHSV(linkRGB(rgba.red, rgba.green, rgba.blue));
}

/** PROCESSING FUNCTIONS **/
/*  reads in image from specified filename as RGBA 8bit pixmap
	puts a new ImageRGBA in the imageCache vector if successful */
void readImage(string filename) {
	std::unique_ptr<ImageInput> in = ImageInput::open(filename);
	if (!in) {
		std::cerr << "could not open input file! " << geterror();
		//cancel routine
		return;
	}

	//store spec and get metadata from it
	ImageRGBA image;
	image.spec = in->spec();
	int xr = image.spec.width;
	int yr = image.spec.height;
	int channels = image.spec.nchannels;

	//declare temp memory to read raw image data
	unsigned char temp_px[xr*yr*channels];

	// read the image into the temp_px from the input file, flipping it upside down using negative y-stride,
	// since OpenGL pixmaps have the bottom scanline first, and 
	// oiio expects the top scanline first in the image file.
	int scanlinesize = xr * channels * sizeof(unsigned char);
	if(!in->read_image(TypeDesc::UINT8, temp_px+((yr-1)*scanlinesize), AutoStride, -scanlinesize)){
		cerr << "Could not read image from " << filename << ", error = " << geterror() << endl;
		//cancel routine
		return;
  	}
	
	//allocate data for converted pxRGBA version
	image.pixels = new pxRGBA[xr*yr];
	//convert and store raw image data as pxRGBAs
	for (int i=0; i<yr*xr; i++) {
		switch(channels) {
			//this could be more cleanly programmed but the less convoluted the better
			case 1: //grayscale
				image.pixels[i].red = temp_px[i];
				image.pixels[i].green = temp_px[i];
				image.pixels[i].blue = temp_px[i];
				image.pixels[i].alpha = MAX_VAL;
				break;
			case 2: //weird grayscale with alpha (just covering my ass here)
				image.pixels[i].red = temp_px[2*i];
				image.pixels[i].green = temp_px[2*i];
				image.pixels[i].blue = temp_px[2*i];
				image.pixels[i].alpha = temp_px[(2*i)+1];
				break;
			case 3: //RGB
				image.pixels[i].red = temp_px[(3*i)];
				image.pixels[i].green = temp_px[(3*i)+1];
				image.pixels[i].blue = temp_px[(3*i)+2];
				image.pixels[i].alpha = MAX_VAL;
				break;
			case 4: //RGBA
				image.pixels[i].red = temp_px[(4*i)];
				image.pixels[i].green = temp_px[(4*i)+1];
				image.pixels[i].blue = temp_px[(4*i)+2];
				image.pixels[i].alpha = temp_px[(4*i)+3];
				break;
			default: //something weird, just do nothing
				break;
		}
	}

	//close input
	in->close();
	//store struct in cache and switch to display this image
	imageCache.push_back(image);
	imageIndex = imageCache.size()-1;
}

/* writes currently dixplayed pixmap (as RGBA) to a file
	(mostly the same as sample code) */
void writeImage(string filename){
	int xr = imageCache[imageIndex].spec.width;
	int yr = imageCache[imageIndex].spec.height;
	int channels = 4;
	//temporary 1d array to stick all the pxRGBA data into
	//write_image does not like my structs >:(
	unsigned char temp_px[xr*yr*channels];
	for (int i=0; i<xr*yr; i++) {
		temp_px[(4*i)] = imageCache[imageIndex].pixels[i].red;
		temp_px[(4*i)+1] = imageCache[imageIndex].pixels[i].green;
		temp_px[(4*i)+2] = imageCache[imageIndex].pixels[i].blue;
		temp_px[(4*i)+3] = imageCache[imageIndex].pixels[i].alpha;
	}

	// create the oiio file handler for the image
	std::unique_ptr<ImageOutput> outfile = ImageOutput::create(filename);
	if(!outfile){
		cerr << "could not create output file! " << geterror() << endl;
		//cancel routine
		return;
	}

	// open a file for writing the image. The file header will indicate an image of
	// width xr, height yr, and 4 channels per pixel (RGBA). All channels will be of
	// type unsigned char
	ImageSpec spec(xr, yr, channels, TypeDesc::UINT8);
	if(!outfile->open(filename, spec)){
		cerr << "could not open output file! " << geterror() << endl;
		return;
	}

	// write the image to the file. All channel values in the pixmap are taken to be
	// unsigned chars. flip using stride to undo same effort in readImage
	int scanlinesize = xr * channels * sizeof(unsigned char);
	if(!outfile->write_image(TypeDesc::UINT8, temp_px+((yr-1)*scanlinesize), AutoStride, -scanlinesize)){
		cerr << "could not write to file! " << geterror() << endl;
		return;
	}
	else cout << "successfully written image to " << filename << endl;

	// close the image file after the image is written
	if(!outfile->close()){
		cerr << "could not close output file! " << geterror() << endl;
		return;
	}
}


/*  reads in filter data from specified filename as RawFilter and converts it to NormFilter
	puts a new NormFilter in the filtCache vector if successful */
void readFilter(string filename) {
	ifstream filt(filename);
	if (!filt) {
		cerr << "could not open .filt file!" << endl;
		return;
	}
	
	RawFilter data;
	//read in size & allocate accordingly
	filt >> data.size;
	int n = data.size;
	data.kernel = new double[data.size*data.size];

	//read in weights until kernel is full
	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			filt >> data.kernel[(r*n)+c];
		}
	}

	//TODO make sure this works
	NormFilter norm;
	norm.size = n;
	norm.kernel = new double[n*n]
	//find max magnitude of raw kernel
	int ind = 0;
	double max = 0.0;
	while (ind < n*n) {
		int mag = abs(data.kernel[ind++]);
		if (mag > max) {max = mag;}
	}
	norm.scalefactor = max;
	//convert all kernel values to percent of scale factoir
	ind = 0;
	while (ind < n*n) {
		norm.kernel[ind++] = percentOf(data.kernel[ind],max);
	}
	
	//delete rawfilter and put normfilter in cache
	discardRawFilt(data);
	filtCache.push_back(norm);
	//select this filter
	filtIndex = filtCache.size()-1;
}

/* apply convolution filter to current image
 * must be using 0~1 NormFilter or things will break */
void convolve(NormFilter filt) {
	//TODO
	//flip the kernel horizontally and vertically
}

/** OPENGL FUNCTIONS **/
/* main display callback: displays the image of current index from imageCache. 
if no images are loaded, only draws a black background */
void draw(){
	//clear and make black background
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (imageCache.size() > 0) {
		//display alphamasked image properly via blending
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//get image spec & current window size
		ImageSpec* spec = &imageCache[imageIndex].spec;
		glPixelZoom(1.0,1.0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
		glRasterPos2i(0,0); //draw from bottom left
		//draw image
		glDrawPixels(spec->width,spec->height,GL_RGBA,GL_UNSIGNED_BYTE,&imageCache[imageIndex].pixels[0]);
		glDisable(GL_BLEND);
	}
	//flush to viewport
	glFlush();
}

/* resets the window size to fit the current image exactly at 100% pixelzoom (unused at the moment)*/
void refitWindow() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[imageIndex].spec;
		glutReshapeWindow(spec->width, spec->height);
	}
}

/*
   Keyboard Callback Routine
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
	string fn;
	switch(key){
		case 'c':
		case 'C':
			convolve();
			return;
		case 'r':
		case 'R':
			reloadOriginal();
			cout << "reverted to original image" << endl;
			return;
		case 'w':
		case 'W':
			cout << "enter output filename: ";
			string wfn;
			cin >> wfn;
			writeImage(wfn);
			return;
		case 'q':		// q - quit
		case 'Q':
		case 27:		// esc - quit
			exit(0);
		default:		// not a valid key -- just ignore it
			return;
	}
}

/*
   Reshape Callback Routine: sets up the viewport and drawing coordinates
   This routine is called when the window is created and every time the window
   is resized, by the program or by the user
*/
void handleReshape(int w, int h){
  // set the viewport to be the entire window
  glViewport(0, 0, w, h);
  
  // define the drawing coordinate system on the viewport
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluOrtho2D(0, w, 0, h);
}

/* timer function to make the OS always update the window 
	(fixes hang on launch...)*/
void timer( int value )
{
    glutPostRedisplay();
    glutTimerFunc( 33, timer, 0 );
}

/* main control method that sets up the GL environment
	and handles command line arguments */
int main(int argc, char* argv[]){
	//read arguments as filenames and attempt to read requested input files
	if (argc >= 3) {
		string Astr = string(argv[1]);
		string Bstr = string(argv[2]);

		//do the things
		readImage(Astr);
		readImage(Bstr);
		compose(0,1);

		//output if given 3rd filename (no default extension appending, sorry)
		if (argc >= 4) {
			string outstr = string(argv[3]);
			writeImage(outstr);
		}
	}
	else {
		cerr << "usage: compose [foreground].png [background] (output)" << endl;
		exit(1);
	}

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("compose Result");

	//if there is an image already loaded, set the resolution to fit it
	refitWindow();
	
	// set up the callback routines to be called when glutMainLoop() detects
	// an event
	glutDisplayFunc(draw);	  // display callback
	glutKeyboardFunc(handleKey);	  // keyboard callback
	glutReshapeFunc(handleReshape); // window resize callback
	glutTimerFunc(0, timer, 0); //timer func to force redraws

	// Routine that loops forever looking for events. It calls the registered
	// callback routine to handle each event that is detected
	glutMainLoop();
	return 0;
}
