//TODO replace this
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
#include "gloiioFuncs.h"
#include <OpenImageIO/imageio.h>
#include <iostream>
#include <string>
#include <exception>

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

/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ImageRGBA> imageCache;
//current index in vector to attempt to load (left/right arrows)
static int imageIndex = 0;
//noisify chance (1/noiseDenom to replace with black pixel)
static int noiseDenom = 5;
//frame counter, currently only used for better random generation
static int drawCount = 0;

/** PROCESSING FUNCTIONS **/
//from starter code
/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
void inv_map(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = x/2;
  v = y/2; 
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}

void inv_map2(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = 0.5 * (x * x * x * x + sqrt(sqrt(y)));
  v = 0.5 * (sqrt(sqrt(x)) + y * y * y * y);
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
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
	//increment draws count
	drawCount++;
	//flush to viewport
	glFlush();
}

/* resets the window size to fit the current image exactly */
void refitWindow() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[imageIndex].spec;
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
		case 'z':
		case 'Z':
			refitWindow();
			break;

		case 'r':
		case 'R':
			//TODO this is where the magnif reconstruction function gets called
			break;
		
		case 'w':
		case 'W':
			cout << "enter output filename: ";
			cin >> fn;
			writeImage(fn, imageCache[imageIndex]);
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
			//cout << "was displaying image " << imageIndex+1 << " of " << imageCache.size() << endl;
			if (imageIndex > 0) {
				imageIndex--;
				cout << "image " << imageIndex+1 << " of " << imageCache.size() << endl;
			}
			break;
		case GLUT_KEY_RIGHT:
			//cout << "was displaying image " << imageIndex+1 << " of " << imageCache.size() << endl;
			if (imageIndex < imageCache.size()-1 && imageCache.size() > 0) {
				imageIndex++;
				cout << "image " << imageIndex+1 << " of " << imageCache.size() << endl;
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
		imageCache.push_back(readImage(string(argv[i])));
	}
	//always display first image on load
	imageIndex = 0;

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
