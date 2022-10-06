//
//   OpenGL/GLUT Program to read, display, modify, and write images
//   using the OpenImageIO API
//
//   Load images at launch by including their file paths as arguments
//   ** Left/right arrows swap between multiple loaded images **
//
//   R: read new image from file (prompt)
//   W: write current image to file (prompt)
//   I: invert colors of current image
//   N: randomly add black noise to current image
//   ** P: display the first set of bytes of the image data in hex **
//   
//   Q or ESC: quit program
//
//   CPSC 4040 | Owen Book | September 2022
//
//   usage: imgview [filenames]
//

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
//misc settings (note: noise chance is in statics section)
#define PEEK_COUNT 40

//simple struct to hold image spec and pixels as unsigned chars
typedef struct image_uint8_t {
	ImageSpec spec;
	bool hasAlpha;
	unsigned char* pixels;
} ByteImage;

/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ByteImage> imageCache;
//current index in vector to attempt to load (left/right arrows)
static int cacheIndex = 0;
//noisify chance (1/noiseDenom to replace with black pixel)
static int noiseDenom = 5;
//frame counter, currently only used for better random generation
static int drawCount = 0;

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

/* inverts all colors of the currently loaded image
	attempts to ignore alpha channel */
void invert() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[cacheIndex].spec;
		unsigned char* px = imageCache[cacheIndex].pixels;
		if (imageCache[cacheIndex].hasAlpha) {
			for (int i=0; i<spec->width*spec->height*spec->nchannels; i++) {
				//ignore every channel'th byte since it is most likely the alpha channel
				if ((i+1)%spec->nchannels != spec->nchannels) {
					px[i] = MAX_VAL-px[i];
				}
			}
		}
		else {
			for (int i=0; i<spec->width*spec->height*spec->nchannels; i++) {
				px[i] = MAX_VAL-px[i];
			}
		}
		cout << "inverted image" << endl;
	}
	else cout << "cannot invert when no images are loaded" << endl;
}

/* randomly replaces pixels with black
	chance defined by 1/noiseDenom */
void noisify() {
	//the c++ library for this is kind of gross looking but cpp.com has a good example
	default_random_engine gen(time(NULL)+drawCount);
	uniform_int_distribution<int> dist(1,noiseDenom);
	auto rando = bind(dist,gen);
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[cacheIndex].spec;
		unsigned char* px = imageCache[cacheIndex].pixels;
		if (imageCache[cacheIndex].hasAlpha) {
			for (int i=0; i<spec->width*spec->height; i++) {
				if (rando() == 1) {
					//set only the color/luminance values to 0
					for (int j=0; j<spec->nchannels; j++) {
						if ((j+1)%spec->nchannels != spec->nchannels) {
							px[(spec->nchannels*i)+j] = 0;
						}
					}
				}
			}
		}
		else {
			for (int i=0; i<spec->width*spec->height; i++) {
				if (rando() == 1) {
					//set all values to 0
					for (int j=0; j<spec->nchannels; j++) {
						px[(spec->nchannels*i)+j] = 0;
					}
				}
			}
		}
		cout << "added noise" << endl;
	}
	else cout << "cannot add noise when no images are loaded" << endl;
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
	//increment draws count
	drawCount++;
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

		case 'r':
		case 'R':
			cout << "enter input filename: ";
			cin >> fn;
			readImage(fn);
			break;
		
		case 'w':
		case 'W':
			cout << "enter output filename: ";
			cin >> fn;
			writeImage(fn);
			break;

		case 'i':
		case 'I':
			invert();
			break;
		
		case 'n':
		case 'N':
			noisify();
			break;
		
		case 'q':		// q - quit
		case 'Q':
		case 27:		// esc - quit
			exit(0);
		
		default:		// not a valid key -- just ignore it
			return;
	}
}

void specialKey(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_LEFT:
			//cout << "was displaying image " << cacheIndex+1 << " of " << imageCache.size() << endl;
			if (cacheIndex > 0) {
				cacheIndex--;
				cout << "image " << cacheIndex+1 << " of " << imageCache.size() << endl;
			}
			break;
		case GLUT_KEY_RIGHT:
			//cout << "was displaying image " << cacheIndex+1 << " of " << imageCache.size() << endl;
			if (cacheIndex < imageCache.size()-1 && imageCache.size() > 0) {
				cacheIndex++;
				cout << "image " << cacheIndex+1 << " of " << imageCache.size() << endl;
			}
			break;
		default:
			return; //ignore other keys
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
	glutCreateWindow("Get the Picture");

	// set up the callback routines to be called when glutMainLoop() detects
	// an event
	glutDisplayFunc(draw);	  // display callback
	glutKeyboardFunc(handleKey);	  // keyboard callback
	glutSpecialFunc(specialKey); //special callback (arrow keys, etc.)
	glutReshapeFunc(handleReshape); // window resize callback
	glutTimerFunc(0, timer, 0); //timer func to force redraws

	// Routine that loops forever looking for events. It calls the registered
	// callback routine to handle each event that is detected
	glutMainLoop();
	return 0;
}
