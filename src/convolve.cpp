//	convolve: OpenGL & OIIO program to apply convolution filters to an image multiple times
//
//	Usage: convolve [filter].filt [input].png (output)
//	A .filt file is plaintext full of any numerical values
//	that specifies its size and weights.
//	See README.md for more details
//
//	CPSC 4040 | Owen Book | October 2022
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
//output filename if provided
static string outstr;

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
		//debug stuff sorry
		/*case 'd':
			imageIndex++;
			if (imageIndex > imageCache.size()-1) {
				imageIndex = 0;
			}
			cout << "showing image " << imageIndex+1 << " of " << imageCache.size() << endl;
			return;*/
		case 'c':
		case 'C':
			convolve(filtCache[filtIndex]);
			//cout << "applied to image " << imageIndex+1 << " of " << imageCache.size() << endl;
			return;
		case 'r':
		case 'R':
			if (imageCache.size() > 1) {
				removeImage(imageIndex);
				imageIndex = cloneImage(0);
			}
			cout << "reverted to original image" << endl;
			return;
		case 'w':
		case 'W':
			if (outstr.empty()) {
				cout << "enter output filename: ";
				cin >> outstr;
			}
			writeImage(outstr);
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
		string filtstr = string(argv[1]);
		string instr = string(argv[2]);

		//read from files
		readFilter(filtstr);
		readImage(instr);

		//output if given 3rd filename (no default extension appending, sorry)
		if (argc >= 4) {
			outstr = string(argv[3]);
		}

		imageIndex = cloneImage(0); //create copy for working on
	}
	else {
		cerr << "usage: convolve [filter].filt [input].png (output)" << endl;
		exit(1);
	}

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("convolve");

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
