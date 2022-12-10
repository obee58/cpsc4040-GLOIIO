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
//width of background that stays around image
#define BORDER_WIDTH 24
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
static double scaleX = 1;
static double scaleY = 1;
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
		glRasterPos2i(BORDER_WIDTH,BORDER_WIDTH); //draw from bottom left
		if (item->isRaw) {
			//TODO determine ideal wrap width and fit to screen
			int wrapWidth = 100;
			glPixelZoom(1.0,1.0);
			glDrawPixels(wrapWidth,item->data.size/wrapWidth,GL_RGBA,GL_UNSIGNED_BYTE,&(item->data.array[0]));
		}
		else {
			//draw based on image spec
			ImageSpec* spec = &(item->image.spec);
			double zWidth = (double)(glutGet(GLUT_WINDOW_WIDTH)-(BORDER_WIDTH*2))/spec->width;
			double zHeight = (double)(glutGet(GLUT_WINDOW_HEIGHT)-(BORDER_WIDTH*2))/spec->height;
			double zoom = min(zWidth, zHeight); //good enough
			glPixelZoom(zoom, zoom);
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
		glutReshapeWindow(spec->width+(BORDER_WIDTH*2), spec->height+(BORDER_WIDTH*2));
	}
}

/*
   Keyboard Callback Routine
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y) {
	//handle jump number keys less awfully
	if (isdigit(key)) {
		string keystr;
		keystr = key;
		int num = stoi(keystr);
		if (num == 0) { num = 10; } //0 goes to 10th position
		if (validIndex(num-1)) {
			imageIndex = num-1;
			cout << "<i> image " << num << " of " << imageCache.size() << endl;
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
			//move backwards unless the cache is now empty
			if (!imageCache.empty()) {imageIndex--;}
			cout << "<i> deleted image " << imageIndex+2 << endl;
			break;
		
		case 'r': //read
		case 'R':
			cout << "enter input filename: ";
			cin >> fn;
			if (glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				//load file data without OIIO no matter what
				cout << "<i> reading as raw data" << endl;
				try {
					newimg.isRaw = true;
					newimg.data = readRaw(fn);
				}
				catch (exception &e) {break;}
			}
			else {
				//load image with OIIO
				try {
					newimg.isRaw = false;
					newimg.image = readImage(fn);
				}
				catch (exception &e) {
					//try to load it without OIIO
					cout << "<i> trying as raw data" << endl;
					try {
						newimg.isRaw = true;
						newimg.data = readRaw(fn);
					}
					catch (exception &e) {break;} 
				}
			}
				
			if (imageCache.size() >= CACHE_COUNT) {
				imageCache.back() = newimg; //replace
				imageIndex = CACHE_COUNT-1;
			}
			else {
				imageCache.push_back(newimg);
				imageIndex = imageCache.size()-1;
			}
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
			if (!opMode && !validIndex(selSecret)) { //auto select current image if no secret selected
				selSecret = imageIndex;
				if (imageIndex == selTarget) { selTarget = -1; }
			}
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
					scaleX = 1.0;
					scaleY = 1.0;
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

		//confirm and perform operation
		case 13: //enter
		case 32: //space
			if (!validIndex(imageIndex)) {break;}
			else if (!validIndex(selTarget)) {
				cout << "<x> no target selected" << endl;
				break;
			}

			if (!opMode && validIndex(selSecret)) {
				StenoImage result;
				result.isRaw = false;
				if (imageCache[selSecret].isRaw) {
					result.image = encodeData(imageCache[selTarget].image, imageCache[selSecret].data, bits);
				}
				else {
					int secX = imageCache[selSecret].image.spec.width;
					int secY = imageCache[selSecret].image.spec.height;
					int covX = imageCache[selTarget].image.spec.width;
					int covY = imageCache[selTarget].image.spec.height;
					if (scaleX != 1.0 || scaleY != 1.0) {
						int resultX = secX*scaleX;
						int resultY = secY*scaleY;
						if (resultX > RES_MAX || resultY > RES_MAX) {
							cout << "<x> can't scale secret (resolution limit is " << RES_MAX << "x" << RES_MAX << ")" << endl;
							break;
						}
						else if (resultX > covX || resultY > covY) {
							cout << "<x> secret will not fit in the current cover image (" << covX << "x" << covY << ")" << endl;
							break;
						}
						result.image = encodeImage(imageCache[selTarget].image, scale(imageCache[selSecret].image,scaleX,scaleY), bits);
					}
					else {
						if (secX > covX || secY > covY) {
							cout << "<x> secret will not fit in the current cover image (" << covX << "x" << covY << ")" << endl;
							break;
						}
						result.image = encodeImage(imageCache[selTarget].image, imageCache[selSecret].image, bits);
						cout << "<i> displaying encoded image" << endl;
					}
				}

				if (validIndex(9)) {
					imageCache.back() = result; //replace
				}
				else {
					imageCache.push_back(result);
				}
				selOutput = imageCache.size()-1;
				imageIndex = selOutput;
				break;
			}
			else if (opMode) {
				StenoImage result;
				result.isRaw = false;
				result.image = decodeImage(imageCache[selTarget].image, bits);
				cout << "<i> displaying decoded image" << endl;
				//TODO data decode condition

				if (validIndex(9)) {
					imageCache.back() = result; //replace
				}
				else {
					imageCache.push_back(result);
				}
				selOutput = imageCache.size()-1;
				imageIndex = selOutput;
				break;
			}
			else {
				cout << "<x> no secret selected" << endl;
				break;
			}
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
			if (imageIndex > 0) {
				imageIndex--;
				cout << "<i> image " << imageIndex+1 << " of " << imageCache.size() << endl;
			}
			break;
		case GLUT_KEY_RIGHT:
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
int main(int argc, char* argv[]) {
	if (argc >= 2) {
		cout << argv[1] << endl;
		if (argv[1] == "instant") {
			//TODO instant mode (last thing to add, options hard)
		}
		else if (strcmp(argv[1],"encode") == 0) {
			opMode = false;
			if (argc >= 3) {
				//get cover image
				StenoImage cover;
				try {
					cover.isRaw = false;
					cover.image = readImage(argv[2]);
				}
				catch (exception &e) {
					cerr << "could not read image " << argv[2] << endl;
					exit(1);
				}
				imageCache.push_back(cover);
				selTarget = imageCache.size()-1;

				if (argc >= 4) {
					//get secret
					StenoImage secret;
					try {
						secret.isRaw = false;
						secret.image = readImage(argv[3]);
					}
					catch (exception &e) {
						cout << "trying as raw data... ";
						try {
							secret.isRaw = true;
							secret.data = readRaw(argv[3]);
						}
						catch (exception &e) {
							cerr << "could not read file " << argv[3] << endl;
							exit(1);
						}
						cout << endl;
					}
					imageCache.push_back(secret);
					selSecret = imageCache.size()-1;
				}
			}
		}
		else if (strcmp(argv[1],"decode") == 0) {
			opMode = true;
			if (argc >= 3) {
				StenoImage newimg;
				try {
					newimg.isRaw = false;
					newimg.image = readImage(argv[2]);
				}
				catch (exception &e) {
					cerr << "could not read image " << argv[2] << endl;
					exit(1);
				}
				imageCache.push_back(newimg);
				selTarget = imageCache.size()-1;
			}
		}
		else {
			//assume all arguments are filenames and open w/o any settings
			for (int i=1; i<argc; i++) {
				StenoImage newimg;
				try {
					newimg.isRaw = false;
					newimg.image = readImage(argv[i]);
				}
				catch (exception &e) {
					cout << "trying as raw data... ";
					try {
						newimg.isRaw = true;
						newimg.data = readRaw(argv[i]);
					}
					catch (exception &e) {
						cerr << "could not read file " << argv[i] << endl;
						exit(1);
					}
					cout << endl;
				}
				imageCache.push_back(newimg);
			}
		}
	}

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
