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
#define COLOR_EMPTY 0.1,0.1,0.1,1
#define COLOR_NONE 0,0,0,1
#define COLOR_COVER 0,0,0.3,1
#define COLOR_SECRET 0,0.3,0,1
#define COLOR_DECODE 0.3,0,0,1
#define COLOR_OUTPUT 0.4,0.2,0,1

/** CONTROL & GLOBAL STATICS **/
static vector<StenoImage> imageCache; //list of read images for multi-image viewing mode
static int imageIndex = 0; //current index in vector to attempt to load (left/right arrows)
static bool opMode = false; //false = encode, true = decode (yes it's obtuse)
static string outstr; //current output path target
static double scaleX = 0; //0 = auto
static double scaleY = 0;
static int bits = 3; //default to 3, max hardcoded to 8
//selection memory, set to -1 to deselect
static int selTarget = -1; //cover if encode
static int selSecret = -1;
static int selOutput = -1; //only for display

/** PROCESSING & HELPER FUNCTIONS **/
//quick check if displaying image at this index is ok
bool validIndex(int index) {
	if (index < 0 || index >= CACHE_COUNT || index >= imageCache.size()) {
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
		if (item->isRaw) {
			//TODO determine viewport size & ideal wrap width
			int wrapWidth = 100;
			glPixelZoom(1.0,1.0);
			glDrawPixels(item->data.size/wrapWidth,wrapWidth,GL_RGBA,GL_UNSIGNED_BYTE,&(item->data.array[0]));
		}
		else {
			//draw based on image spec
			ImageSpec* spec = &(item->image.spec);
			//TODO determine viewport size & zoom
			glPixelZoom(1.0,1.0);
			glDrawPixels(spec->width,spec->height,GL_RGBA,GL_UNSIGNED_BYTE,&(item->image.pixels[0]));
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
void handleKey(unsigned char key, int x, int y) {
	//handle jump number keys less awfully
	//TODO this currently explodes because stoi no like
	if (isdigit(key)) {
		string keystr;
		keystr = key;
		int num = stoi(keystr);
		if (num == 0) { num = 10; } //0 goes to 10th position
		if (validIndex(num-1)) {
			imageIndex = num-1;
			cout << "<i> jumped to slot " << num << endl;
		}
		return;
	}

	string fn; //for R case
	StenoImage newimg; //for R case
	StenoImage trash; //for backspace case
	switch(key) {
		//misc commands
		case '`': //refit
		case '~':
			refitWindow();
			break;

		//I/O commands
		case 8: //backspace; delete from cache
			if (!validIndex(imageIndex)) {break;}
			trash = imageCache[imageIndex];
			if (trash.isRaw) {
				delete[] trash.data.array;
			}
			else {
				delete[] trash.image.pixels;
			}

			imageCache.erase(imageCache.begin()+imageIndex);
			//deselect this position
			if (imageIndex == selTarget) { selTarget = -1; }
			if (imageIndex == selOutput) { selOutput = -1; }
			if (imageIndex == selSecret) { selSecret = -1; }
			//decrement all selection indices after current one
			if (imageIndex < selTarget) { selTarget--; }
			if (imageIndex < selOutput) { selOutput--; }
			if (imageIndex < selSecret) { selSecret--; }
			cout << "<i> deleted image " << imageIndex << endl;
			break;
		
		case 'r': //read
		case 'R':
			cout << "enter input filename: ";
			cin >> fn;
			try {
				if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
					//load file data no matter what
					cout << "<i> reading as raw data" << endl;
					newimg.isRaw = true;
					newimg.data = readRaw(fn);
				}
				else {
					//load image with OIIO
					newimg.isRaw = false;
					newimg.image = readImage(fn);
				}
				
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
			if (!validIndex(imageIndex)) {break;}
			if (outstr.empty() || glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				//let user reset output (save as)
				cout << "enter output filename: ";
				cin >> outstr;
			}
			if (imageCache[imageIndex].isRaw) {
				writeRaw(outstr, imageCache[imageIndex].data);
			}
			else {
				writeImage(outstr, imageCache[imageIndex].image);
			}
			break;

		//selection commands
		case 'z': //swap mode
		case 'Z':
			opMode = !opMode;
			if (opMode) {
				cout << "<i> decoding mode" << endl;
			}
			else {
				cout << "<i> encoding mode" << endl;
			}
			break;

		case 'x': //select cover/target
		case 'X':
			if (!validIndex(imageIndex)) {break;}
			if (imageCache[imageIndex].isRaw) {
				cout << "<x> cover image cannot be raw" << endl;
			}
			else {
				selTarget = imageIndex;
				if (imageIndex == selSecret) { selSecret = -1; }
			}
			break;

		case 'c': //select secret
		case 'C':
			if (!validIndex(imageIndex)) {break;}
			if (!opMode) {
				selSecret = imageIndex;
				if (imageIndex == selTarget) { selTarget = -1; }
			}
			break;
		
		//settings commands
		case 'a': //scale secret before encoding
		case 'A':
			if (validIndex(selSecret)) {
				if (imageCache[selSecret].isRaw) {
					cout << "<x> raw data secrets cannot be scaled" << endl;
					break;
				}
				else if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) { 
					cout << "enter secret horizontal scale factor: ";
					cin >> scaleX;
					cout << "enter secret vertical scale factor: ";
					cin >> scaleY;
				}
				else {
					cout << "enter secret scale factor (0 to reset): ";
					cin >> scaleX;
					scaleY = scaleX;
				}

				if (scaleX <= 0 || scaleY <= 0) {
					cout << "<i> scaling disabled" << endl;
					break;
				}
				int resultX = imageCache[selSecret].image.spec.width*scaleX;
				int resultY = imageCache[selSecret].image.spec.height*scaleY;
				cout << "secret will be " << resultX << "x" << resultY << " when encoded" << endl;
				if (validIndex(selTarget)) {
					int covX = imageCache[selTarget].image.spec.width;
					int covY = imageCache[selTarget].image.spec.height;
					if (resultX > covX || resultY > covY) {
						cout << "<!> secret will not fit in the current cover image (" << covX << "x" << covY << ")" << endl;
					}
				}
			}
			break;

		case 's': //set number of bits to hide in or extract
		case 'S':
			cout << "enter # of bits used by secret: ";
			cin >> bits;
			if (bits <= 0 || bits >= 8) {
				cout << "<x> invalid # of bits, defaulting to 3" << endl;
				bits = 3;
			}
			break;

		case 'd': //switch channels used for hiding (grayscale mode)
		case 'D':
			//TODO pain in the ass
			break;

		case 'f': //enable/disable compression of secret
		case 'F':
			//TODO lowest priority i'm most likely not adding this
			break;

		//confirm and perform operation
		case '\n':
			//TODO big prints big checks
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
		case GLUT_KEY_F1:
			//TODO holy mother of help text
			break;
		case GLUT_KEY_LEFT:
			//cout << "was displaying image " << imageIndex+1 << " of " << imageCache.size() << endl;
			if (imageIndex > 0) {
				imageIndex--;
				cout << "<i> image " << imageIndex+1 << " of " << imageCache.size() << endl;
			}
			break;
		case GLUT_KEY_RIGHT:
			//cout << "was displaying image " << imageIndex+1 << " of " << imageCache.size() << endl;
			if (imageIndex < imageCache.size()-1 && imageCache.size() > 0) {
				imageIndex++;
				cout << "<i> image " << imageIndex+1 << " of " << imageCache.size() << endl;
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
