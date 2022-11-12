//	tonemap: experimental OpenGL & OIIO program to apply color/gamma corrections & tone mapping to HDR images for digital display
//	Only supports HDR and EXR images! Do not try to input png, jpg, etc.
//
//	CLI usage: tonemap [input].[hdr/exr] (output)
//	Currently only supports simple tonemapping function.
//	
//	Window controls:
//	left/right arrows - switch view between original or working copy (similar behavior to imgview)
//		note: using any other function will automatically switch back to the working copy
//	c - toggle 1/2.2 gamma correction when tonemapping; disabled by default
//	b - create basic tonemapped version of HDR image (no gamma compression)
//	g - tonemap using gamma compression (prompts for gamma value)
//	r - revert working copy to the original and start over
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
//toggle whether or not to use 1/2.2 gamma correction when tonemapping
static bool useCorrection = false;
//memory of loaded files/data
static vector<ImageHDR> imageCache;
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
		imageCache.push_back(cloneHDR(imageCache[0]));
		imageIndex = imageCache.size()-1;
	}
	if (inform) { cout << "reverted to original" << endl; }
	return;
}
/* avoid modifying original in cache 
 * returns true if this check was necessary 
 * bool = whether or not to to warn user */
bool preserveOriginal(bool inform) {
	if (imageIndex == 0) {
		imageIndex = 1;
		if (inform) { cout << "showing working copy" << endl; }
		return true;
	}
	return false;
}

/** GL PROCESSING FUNCTIONS **/
//stupid reimplementation of old writeImage
void captureWrite(string filename) {
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

	// get the current pixels from the OpenGL framebuffer as bytes and store in pixmap
	glReadPixels(0, 0, xr, yr, GL_RGBA, GL_UNSIGNED_BYTE, px);

	//FLIP the image data so that it isn't upside down (framebuffer starts at bottom left)
	unsigned char* pxflip = new unsigned char[xr*yr*channels];
	int linebytes = xr*channels;
	int destline = 0; //my pointer math was bad that day
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
	delete[] pxflip;

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
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (imageCache.size() > 0) {
		//get image spec & current window size
		ImageSpec* spec = &imageCache[imageIndex].spec;
		glPixelZoom(1.0,1.0);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
		glRasterPos2i(0,0); //draw from bottom left
		//draw float image
		glDrawPixels(spec->width,spec->height,GL_RGB,GL_FLOAT,&imageCache[imageIndex].pixels[0]);
	}
	//flush to viewport
	glFlush();
}

/* resets the window size to fit the current image exactly at 100% pixelzoom */
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
			useCorrection = !useCorrection;
			if (useCorrection) {
				cout << "gamma correction enabled" << endl;
			}
			else {
				cout << "gamma correction disabled" << endl;
			}
			return;
		case 'b':
		case 'B':
			preserveOriginal(true);
			tonemap(imageCache[imageIndex], 1.0, useCorrection);
			return;
		case 'g':
		case 'G':
			preserveOriginal(true);
			cout << "enter gamma value: ";
			double gamma;
			cin >> gamma;
			if (gamma > 0.0 && gamma < 1.0) {
				tonemap(imageCache[imageIndex], gamma, useCorrection);
			}
			else {
				cout << "gamma must be between 0 and 1 (aborted)" << endl;
			}
			return;
		case 'r':
		case 'R':
			preserveOriginal(true);
			revertOriginal(true);
			return;
		case 'n':
		case 'N':
			preserveOriginal(true);
			//TODO excredit: normalize (core op)
			return;
		case 'h':
		case 'H':
			preserveOriginal(true);
			//TODO excredit: histogram eq (core op)
			return;
		case 'w':
		case 'W':
			preserveOriginal(true); //assume user wants to write modified image
			if (outstr.empty() || glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				cout << "enter output filename: ";
				cin >> outstr;
			}
			captureWrite(outstr);
			return;
		case 'q':		// q - quit
		case 'Q':
		case 27:		// esc - quit
			exit(0);
		default:		// not a valid key -- just ignore it
			return;
	}
}

/* handles arrow keys & fn keys */
void specialKey(int key, int x, int y) {
	switch(key) {
		case GLUT_KEY_LEFT:
			if (imageIndex > 0) {
				imageIndex--;
				cout << "showing original" << endl;
			}
			break;
		case GLUT_KEY_RIGHT:
			if (imageIndex < imageCache.size()-1 && imageCache.size() > 0) {
				imageIndex++;
				cout << "showing working copy" << endl;
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
	//read arguments as filenames and attempt to read requested input files
	if (argc >= 2) {
		string instr = string(argv[1]);

		//read from files
		imageCache.push_back(readHDR(instr));

		//output if given 2nd filename (no default extension appending, sorry)
		if (argc >= 3) {
			outstr = string(argv[2]);
		}

		imageCache.push_back(cloneHDR(imageCache[0])); //create copy for working on
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
	glutSpecialFunc(specialKey);
	glutReshapeFunc(handleReshape); // window resize callback
	glutTimerFunc(0, timer, 0); //timer func to force redraws

	// Routine that loops forever looking for events. It calls the registered
	// callback routine to handle each event that is detected
	glutMainLoop();
	return 0;
}
