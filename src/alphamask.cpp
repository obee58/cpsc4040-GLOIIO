//	OpenGL/GLUT Program to create alpha masks for any greenscreen image
//	Displays resulting image when done & exports to file
//
//	Usage: alphamask input.(img) output.png [3 floats HSV of target] [3 floats HSV of tolerance]
//	Input can be any image type, output will be png
//
//	CPSC 4040 | Owen Book | October 2022
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
//current index in vector to attempt to load (probably won't be touched in this program)
static int cacheIndex = 0;

/** OPENGL FUNCTIONS **/
/* main display callback: displays the image of current index from imageCache. 
if no images are loaded, only draws a black background */
void draw(){
	//clear and make black background
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (imageCache.size() > 0) {
		//display alphamasked image not the original input via blending
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//get image spec & current window size
		ImageSpec* spec = &imageCache[cacheIndex].spec;
		glPixelZoom(1.0,1.0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
		glRasterPos2i(0,0); //draw from bottom left
		//draw image
		glDrawPixels(spec->width,spec->height,GL_RGBA,GL_UNSIGNED_BYTE,&imageCache[cacheIndex].pixels[0]);
		glDisable(GL_BLEND);
	}
	//flush to viewport
	glFlush();
}

/* resets the window size to fit the current image exactly at 100% pixelzoom (unused at the moment)*/
void refitWindow() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[cacheIndex].spec;
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
	//read arguments as filenames and attempt to read requested input file
	if (argc >= 3) {
		string instr = string(argv[1]);
		string outstr = string(argv[2]);
		size_t extPos = outstr.rfind(".png");
		if (extPos == string::npos || extPos+4 < outstr.length()) { //append ".png" automatically
			outstr += ".png";
		}

		pxHSV target = linkHSV(120.0, 0.7, 0.7); //fallback
		pxHSV fuzz = linkHSV(20.0, 0.2, 0.2); //fallback
		if (argc >= 6) {
			target = linkHSV(stod(argv[3],nullptr),stod(argv[4],nullptr),stod(argv[5],nullptr));
			cout << stod(argv[3],nullptr) << stod(argv[4],nullptr) << stod(argv[5],nullptr) << endl;
		}
		if (argc >= 9) {
			fuzz = linkHSV(stod(argv[6],nullptr),stod(argv[7],nullptr),stod(argv[8],nullptr));
			cout << stod(argv[6],nullptr) << stod(argv[7],nullptr) << stod(argv[8],nullptr) << endl;
		}

		//do the things
		imageCache.push_back(readImage(instr));
		chromaKey(imageCache[0],target,fuzz.hue,fuzz.saturation,fuzz.value);
		cout << "writing alphamask to file " << outstr << endl;
		writeImage(outstr, imageCache[0]);
		cout << "press ESC or Q to close" << endl;
	}
	else {
		cerr << "usage: alphamask [input] [output].png" << endl;
		exit(1);
	}

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("alphamask Result");

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
