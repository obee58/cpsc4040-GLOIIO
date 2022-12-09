//TODO literally everything in here lmao but mostly selection system & controls
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
#include <cctype>
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
//max allowed images/files in cache
#define CACHE_COUNT 10
//background colors
#define COLOR_EMPTY 0.2,0.2,0.2,1
#define COLOR_NONE 0,0,0,1
#define COLOR_COVER 0,0,0.5,1
#define COLOR_SECRET 0,0.5,0,1
#define COLOR_DECODE 0.5,0,0,1
#define COLOR_OUTPUT 0.5,0.3,0,1

/** CONTROL & GLOBAL STATICS **/
static vector<StenoImage> imageCache; //list of read images for multi-image viewing mode
static int imageIndex = 0; //current index in vector to attempt to load (left/right arrows)
static bool opMode = false; //false = encode, true = decode (yes it's obtuse)
static string outstr; //current output path target
//selection memory, set to -1 to deselect
static int selTarget = -1; //cover if encode
static int selSecret = -1;
static int selOutput = -1;


/** PROCESSING & HELPER FUNCTIONS **/
//quick check if displaying image at this index is ok
bool validIndex(int) {
	if (imageIndex >= CACHE_COUNT || imageIndex >= imageCache.size()) {
		return false;
	}
	return true;
}

/** OPENGL FUNCTIONS **/
/* main display callback: displays the image of current index from imageCache. 
if no images are loaded, only draws a black background */
void draw(){
	if (imageCache.size() > 0) {
		StenoImage* item = &imageCache[imageIndex];
		//clear and make background based on selection mode
		if (imageIndex == selTarget) {
			if (opMode) { glClearColor(COLOR_DECODE); }
			else { glClearColor(COLOR_COVER); }
		}
		else if (imageIndex == selSecret && !opMode) { glClearColor(COLOR_SECRET); }
		else if (imageIndex == selOutput) { glClearColor(COLOR_OUTPUT); }
		else { glClearColor(COLOR_NONE); }
		glClear(GL_COLOR_BUFFER_BIT);

		//display transparency properly via blending
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		//make sure settings are correct
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1); //Parrot Fixer 2000
		glRasterPos2i(0,0); //draw from bottom left
		if (item.isRaw) {
			//TODO determine viewport size & ideal wrap width
			int wrapWidth = 100;
			glPixelZoom(1.0,1.0);
			glDrawPixels(item->data.size/wrapWidth,wrapWidth,GL_RGBA,GL_UNSIGNED_BYTE,item->data.array[0]);
		}
		else {
			//draw based on image spec
			ImageSpec* spec = &item.image.spec;
			//TODO determine viewport size & zoom
			glPixelZoom(1.0,1.0);
			glDrawPixels(spec->width,spec->height,GL_RGBA,GL_UNSIGNED_BYTE,item->image.pixels[0]);
		}
		glDisable(GL_BLEND);
	}
	else {
		//clear and make gray background
		glClearColor(COLOR_EMPTY);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	
	glFlush(); //flush to viewport
}

/* resets the window size to fit the current image better */
void refitWindow() {
	if (imageCache.size() > 0) {
		ImageSpec* spec = &imageCache[imageIndex].image.spec;
		glutReshapeWindow(spec->width*1.1, spec->height*1.1);
	}
}

/*
   Keyboard Callback Routine
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
	string fn;

	//handle jump number keys less awfully
	if (isdigit(key)) {
		keystr = string(key);
		int num = stoi(keystr);
		if (num == 0) { num = 10; } //0 goes to 10th position
		if (validIndex(num-1)) {
			imageIndex = num-1;
			cout << "jumped to slot " << num << endl;
		}
		
	}

	switch(key) {
		//TODO so many keys...
		//misc commands
		case '`': //refit
		case '~':
			refitWindow();
			break;

		//I/O commands
		case 8: //backspace; delete from cache TODO
			//delete[] and vector erase
			//decrement all selection indices after current one
			break;
		
		case 'r': //read
		case 'R':
			cout << "enter input filename: ";
			cin >> fn;
			try {
				//TODO decide when to use raw input mode
				StenoImage newimg;
				newimg.image = readImage(fn);
				newimg.isRaw = false;
				newimg.select = SEL_NONE;
				if (imageCache.size() >= CACHE_COUNT) {
					//TODO warn and prompt for replacing current index
				}
				else {
					imageCache.push_back(newimg);
					imageIndex = imageCache.size()-1;
				}
			}
			catch (exception &e) {}
			break;
		
		case 'w': //write
		case 'W':
			if (outstr.empty() || glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				//let user reset output (save as)
				cout << "enter output filename: ";
				cin >> outstr;
			}
			writeImage(outstr, imageCache[imageIndex]);
			break;

		//selection commands
		case 'z': //swap mode
		case 'Z':
			opMode = !opMode;
			if (opMode) {
				cout << "decode mode" << endl;
			}
			else {
				cout << "encode mode" << endl;
			}
			break;

		case 'x': //select cover/target
		case 'X':
			selTarget = imageIndex;
			break;

		case 'c': //select secret
		case 'C':
			if (opMode) {
				selSecret = imageIndex;
			}
			break;
		
		//settings commands TODO
		case 'a': //scale secret before encoding
		case 'A':
			break;

		//confirm and perform operation TODO major
		case '\n':
			break;

		//exit
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
	//TODO init and cmd handling

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("steno");

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
