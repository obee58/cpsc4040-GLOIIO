//	tonemap: OpenGL & OIIO program to apply color/gamma corrections & tone mapping to HDR images
//	Only supports HDR and EXR images! Do not try to input png, jpg, etc.
//
//	CLI usage: tonemap [input].[hdr/exr] (output)
//	Currently only supports simple tonemapping function.
//	
//	Window controls: (using specific letters is getting complicated, geez)
//	left/right arrows - switch between original or working copy (similar behavior to imgview)
//	b - create basic tonemapped version of HDR image
//	g - tonemap using gamma correction (prompts for gamma value)
//	r - revert working copy to the original and start over
//	(tentative) n - apply image normalization
//	(tentative) h - apply histogram equalization
//	w - write working copy to file (prompts on first use if no output argument provided)
//	shift+w - "save as"; write working copy to file (always prompts)
//	q or ESC - exit
//	
//	CPSC 4040 | Owen Book | November 2022
#include "gloiioFuncs.h"
#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>

#ifdef __APPLE__
	#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

using namespace std;
OIIO_NAMESPACE_USING;

//default window dimensions
#define DEFAULT_WIDTH 600	
#define DEFAULT_HEIGHT 600
//output filename
static string outstr;
//memory of loaded files/data
static vector<ImageFloat> imageCache;
//current index in vector to draw/use/modify
static int imageIndex = 0;

/** CONTROL FUNCTIONS **/
/* removes an image from the imageCache */
int removeImage(int index) {
	if (imageCache.size() > index) {
		//doing it w/o changing gloiioFuncs cause i'm lazy and discardImage is a one liner function for babies
		delete[] imageCache[index].pixels;
		imageCache.erase(imageCache.begin()+index);
	}
	else {
		return -1; //cannot remove from empty cache
	}
	return imageCache.size();
}
/* revert to original image by deleting working copy and making a new one */
void revertOriginal(bool inform) {
	if (imageCache.size() > 1) {
		removeImage(imageIndex);
		imageCache.push_back(cloneImage(imageCache[0]));
		imageIndex = imageCache.size()-1;
	}
	if (inform) { cout << "reverted to original image" << endl; }
	return;
}
/* avoid modifying original in cache 
 * returns true if this check was necessary */
bool preserveOriginal(bool revert, bool inform) {
	if (imageIndex = 0) {
		imageIndex = 1;
		if (revert) {
			revertOriginal(inform);
		}
		return true;
	}
	return false;
}

/** OPENGL FUNCTIONS **/
/* main display callback: displays the image of current index from imageCache. 
if no images are loaded, only draws a black background */
void draw(){
	//clear and make black background
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (imageCache.size() > 0) {
		//display transparent images properly via blending (can probably just leave this in?)
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//get image spec & current window size
		ImageSpec* spec = &imageCache[imageIndex].spec;
		glPixelZoom(1.0,1.0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
		glRasterPos2i(0,0); //draw from bottom left
		//draw float image
		//TODO no alpha? :raised_eyebrow:
		glDrawPixels(spec->width,spec->height,GL_RGB,GL_FLOAT,&imageCache[imageIndex].pixels[0]);
		glDisable(GL_BLEND);
	}
	//flush to viewport
	glFlush();
}

/*
   Keyboard Callback Routine
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
	string fn;
	switch(key){
		//TODO implement special keys
		//TODO should it revert when user is looking at original?
		case 'b':
		case 'B':
			preserveOriginal(false,false);
			tonemap(imageCache[imageIndex], 2.2);
			return;
		case 'g':
		case 'G':
			preserveOriginal(false,false);
			cout << "enter gamma value: " << endl;
			double gamma;
			cin >> gamma;
			tonemap(imageCache[imageIndex]. gamma);
			return;
		case 'r':
		case 'R':
			revertOriginal(true);
			return;
		case 'n':
		case 'N':
			preserveOriginal(false,false);
			//TODO excredit: normalize (core op)
			return;
		case 'h':
		case 'H':
			preserveOriginal(false,false);
			//TODO excredit: histogram eq (core op)
			return;
		case 'w':
		case 'W':
			preserveOriginal(false,false); //assume user wants to write modified image
			if (outstr.empty() || glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				cout << "enter output filename: ";
				cin >> outstr;
			}
			writeImage(outstr, imageCache[imageIndex]);
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
	if (argc >= 2) {
		string instr = string(argv[1]);

		//read from files
		imageCache.push_back(readImage(instr));

		//output if given 2nd filename (no default extension appending, sorry)
		if (argc >= 3) {
			outstr = string(argv[2]);
		}

		imageCache.push_back(cloneImage(imageCache[0])); //create copy for working on
		imageIndex = imageCache.size()-1;
	}
	else {
		cerr << "usage: tonemap [input].[hdr/exr] (output)" << endl;
		exit(1);
	}

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("tonemap");

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
