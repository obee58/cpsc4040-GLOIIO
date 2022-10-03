//	OpenGL/GLUT Program to create alpha masks for any greenscreen image
//	Displays resulting image when done, optional export to file
//
//	Usage: alphamask input.(img) output.png
//	Input can be any image type, output will be png
//	See "settings.ini" to set chroma-keying sensitivity options (TODO)
//
//	CPSC 4040 | Owen Book | October 2022

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <random>
#include <algorithm>

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
//misc settings
#define PEEK_COUNT 40
//preprocess macros
#define maximum(x,y,z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x,y,z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

//simple struct to hold image spec and pixels as unsigned chars
typedef struct image_uint8_t {
	ImageSpec spec;
	bool hasAlpha;
	unsigned char* pixels;
} ByteImage;
//utility struct that holds RGBA 4-tuple of chars
typedef struct pixel_rgba_t {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char alpha;
} pxRGBA;
//utility struct that holds RGB 3-tuple of chars
typedef struct pixel_rgb_t {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
} pxRGB;
//utility struct that holds HSV 3-tuple of doubles
typedef struct pixel_hsv_t {
	double hue;
	double saturation;
	double value;
} pxHSV;

/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ByteImage> imageCache;
//current index in vector to attempt to load (probably won't be touched in this program)
static int cacheIndex = 0;

/** DEBUG FUNCTIONS **/
/* prints the first PEEK bytes of the current image to stdout 
	does nothing if no image is loaded */
void peekBytes() {
	if (imageCache.size() > 0) {
		cout << "first " << PEEK_COUNT << " bytes of this image in memory (note that it is vertically flipped when loaded): " << endl;
		for (int i=0; i<PEEK_COUNT; i++) {
			cout << hex << +imageCache[cacheIndex].pixels[i] << " ";
		}
		cout << dec << endl;
	}
	else cout << "cannot peek when no images are loaded" << endl;
}

/** PROCESSING FUNCTIONS **/
/*  reads in image from specified filename as RGBA 8bit pixmap
	puts a new ByteImage in the imageCache vector if successful */
void readImage(string filename) {
	std::unique_ptr<ImageInput> in = ImageInput::open(filename);
	if (!in) {
		std::cerr << "could not open input file! " << geterror();
		//cancel routine
		return;
	}

	//store spec and get metadata from it
	ByteImage image;
	image.spec = in->spec();
	int xr = image.spec.width;
	int yr = image.spec.height;
	int channels = image.spec.nchannels;

	//declare memory to read image data
	unsigned char* px = new unsigned char[xr*yr*channels];

	//read in image data, mapping as unsigned char
	in->read_image(TypeDesc::UINT8, px);

	//store in struct, close input
	image.pixels = px;
	switch(channels) {
		case 1:
		case 3:
			image.hasAlpha = false;
			break;
		case 2:
		case 4:
			image.hasAlpha = true;
			break;
		default:
			image.hasAlpha = true;
			break;
	}
	in->close();
	//store struct in cache and switch to display the image
	imageCache.push_back(image);
	cacheIndex = imageCache.size()-1;
}

/* functions to quickly create pxRGB, pxRGBA, pxHSV,...*/
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
pxHSV linkHSV(unsigned char h, unsigned char s, unsigned char v) {
	pxHSV px;
	px.hue = h;
	px.saturation = s;
	px.value = v;
	return px;
}

/*convert RGB values into HSV values*/
pxHSV RGBtoHSV(pxRGB rgb) {
	pxHSV hsv;
	double huer, hueg, hueb, max, min, delta;
	/* convert from 0-255 to 0~1 */
	huer = rgb.red / 255.0;
	hueg = rgb.green / 255.0;
	hueb = rgb.blue / 255.0;

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
				hsv.hue = (hueb - huer) / delta;
			}
			else {
				hsv.hue = (huer - hueg) / delta;
			}
			hsv.hue *= 60.0;
			if (hsv.hue < 0) {
				hsv.hue += 360.0;
			}
		}
	}

	return hsv;
}

/* chroma-key image to create alphamask */
void chromaKey(int hue, int sat, int val) {

}

/* writes currently dixplayed pixmap (as RGBA) to a file
	(mostly the same as sample code) */
void writeImage(string filename){
	int xr = glutGet(GLUT_WINDOW_WIDTH);
	int yr = glutGet(GLUT_WINDOW_HEIGHT);
	int channels = 4;
	unsigned char px[channels * xr * yr];

	// create the oiio file handler for the image
	std::unique_ptr<ImageOutput> outfile = ImageOutput::create(filename);
	if(!outfile){
		cerr << "could not create output file! " << geterror() << endl;
		//cancel routine
		return;
	}

	// get the current pixels from the OpenGL framebuffer and store in pixmap
	glReadPixels(0, 0, xr, yr, GL_RGBA, GL_UNSIGNED_BYTE, px);

	//FLIP the image data so that it isn't upside down (framebuffer starts at bottom left)
	unsigned char* pxflip = new unsigned char [xr*yr*channels];
	int linebytes = xr*channels;
	int destline = 0; //my pointer math is bad today
	for (int i=yr-1; i>=0; i--) {
		for (int j=0; j<linebytes; j++) {
			pxflip[(linebytes*i)+j] = px[(destline*linebytes)+j];
		}
		destline++;
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
	// unsigned chars
	if(!outfile->write_image(TypeDesc::UINT8, pxflip)){
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

/** OPENGL FUNCTIONS **/
/* main display callback: displays the image of current index from imageCache. 
if no images are loaded, only draws a black background */
void draw(){
	//clear and make black background
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);
	if (imageCache.size() > 0) {
		//get image spec
		ImageSpec* spec = &imageCache[cacheIndex].spec;
		//fit window to image
		glutReshapeWindow(spec->width,spec->height);
		//determine pixel format
		GLenum format;
		switch(spec->nchannels) {
			case 1:
				format = GL_LUMINANCE;
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
				/* my guess is that for some reason GL doesn't default to reading byte-by-byte?? */
				break;
			case 2:
				format = GL_LUMINANCE_ALPHA;
				break;
			case 3:
				format = GL_RGB;
				break;
			case 4:
				format = GL_RGBA;
				break;
		}
		//start at the top left and flip it because nobody can decide on a consistent coordinate mapping
		glRasterPos2i(0,spec->height);
		glPixelZoom(1.0,-1.0);
		//draw image
		glDrawPixels(spec->width,spec->height,format,GL_UNSIGNED_BYTE,imageCache[cacheIndex].pixels);
	}
	//flush to viewport
	glFlush();
}

/* resets the window size to fit the current image exactly */
void refitWindow() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[cacheIndex].spec;
		glutReshapeWindow(spec->width, spec->height);
	}
}

/*
   Keyboard Callback Routine: 'c' cycle through colors, 'q' or ESC quit,
   'w' write the framebuffer to a file.
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
	string fn;
	switch(key){
		case 'p':
		case 'P':
			peekBytes();
			break;

		case 'z':
		case 'Z':
			refitWindow();
			break;
		
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
	//read arguments as filenames and attempt to read requested files
	for (int i=1; i<argc; i++) {
		readImage(string(argv[i]));
	}
	//always display first image on load
	cacheIndex = 0;

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("alphamask Result");

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
