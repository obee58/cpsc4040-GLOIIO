//TODO update
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
#include "matrixBuilder.h"
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
using namespace mtxb;
OIIO_NAMESPACE_USING

/** CONSTANTS & DEFINITIONS **/
//default window dimensions
#define DEFAULT_WIDTH 600	
#define DEFAULT_HEIGHT 600

/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ImageRGBA> imageCache;
static int imageIndex = 0; //don't really need to touch it in this one
//current output path target
static string outstr;
//the current transformation matrix
static Matrix3D tmatrix;

/** CONTROL FUNCTIONS **/
/* Build a transformation matrix from input text */
void mtxInput(Matrix3D &M) {
	string cmd;
	/* prompt for user input */
	do {
		cout << "> ";
		cin >> cmd;
		if (cmd.length() != 1){
			cout << "invalid command, enter r, s, t, h, f, p, n, m, d\n";
		}
		else {
			switch (cmd[0]) {
				case 'r':		/* Rotation, accept angle in degrees */
					float theta;
					cin >> theta;
					if (cin) {
						cout << "rotating by " << theta << " degrees" << endl;
						rotate(M, theta);
					}
					else {
						cerr << "invalid rotation angle" << endl;
						cin.clear();
					}						
					break;
				case 's':		/* scale, accept scale factors */
					float sx, sy;
					cin >> sx;
					cin >> sy;
					if (cin && sx != 0.0 && sy != 0.0) {
						cout << "scaling by " << sx << ", " << sy << endl;
						scale(M, sx, sy);
					}
					else {
						cerr << "invalid scale factors" << endl;
						cin.clear();
					}
					break;
				case 't':		/* Translation, accept translations */
					float dx, dy;
					cin >> dx;
					cin >> dy;
					if (cin) {
						cout << "translating by " << dx << ", " << dy << endl;
						translate(M, dx, dy);
					}
					else {
						cerr << "invalid translation" << endl;
						cin.clear();
					}
					break;
				case 'h':		/* Shear, accept shear factors */
					float hx, hy;
					cin >> hx;
					cin >> hy;
					if (cin) {
						cout << "shearing by " << hx << ", " << hy << endl;
						shear(M, hx, hy);
					}
					else {
						cerr << "invalid shear factors" << endl;
						cin.clear();
					}
					break;
				case 'f':		/* Flip, accept flip factors */
					int fx, fy;
					cin >> fx;
					cin >> fy;
					if (cin) {
						bool horiz = fx>0;
						bool vert = fy>0;
						if (horiz && vert) { cout << "flipping horizontally and vertically" << endl; }
						else if (horiz) { cout << "flipping horizontally" << endl; }
						else if (vert) { cout << "flipping vertically" << endl; }
						flip(M, horiz, vert);
					}
					else {
						cerr << "what" << endl;
						cin.clear();
					}
					break;
				case 'p':		/* Perspective, accept perspective factors */
					float px, py;
					cin >> px;
					cin >> py;
					if (cin) {
						cout << "perspective warp by " << px << ", " << py << endl;
						perspective(M, px, py);
					}
					else {
						cerr << "invalid perspective factors" << endl;
						cin.clear();
					}
					break;
				case 'n': // do before or after? not matrix i don't think
					float cx, cy, s;
					cin >> cx;
					cin >> cy;
					cin >> s;
					if (cin) {
						cout << "twirling at " << cx << ", " << cy << " with strength " << s << endl;
						twirl(M, cx, cy, s);
					}
					else {
						cerr << "invalid twirl position/strength" << endl;
						cin.clear();
					}
					break;
				case 'm': // same issue as twirl
					int tx, ty;
					float ax, ay;
					cin >> tx;
					cin >> ty;
					cin >> ax;
					cin >> ay;
					if (cin) {
						cout << "rippling with period " << tx << ", " << ty << " and amplitude " << ax << ", " << ay << endl;
						ripple(M, tx, ty, ax, ay);
					}
					else {
						cerr << "invalid period length/amplitude" << endl;
						cin.clear();
					}
					break;
				case 'd':		/* Done, that's all for now */
					cout << "accumulated matrix: " << endl;
    				M.print();
					break;
				default:
					cout << "invalid command, enter r, s, t, h, f, p, n, m, d\n";
			}
		}
	} while (cmd.compare("d")!=0);
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
   Keyboard Callback Routine
   This routine is called every time a key is pressed on the keyboard
*/
void handleKey(unsigned char key, int x, int y){
	switch(key){
		case 'z':
		case 'Z':
			refitWindow();
			break;
		case 'w':
		case 'W':
			if (outstr.empty() || glutGetModifiers() == GLUT_ACTIVE_SHIFT) {
				cout << "enter output filename: ";
				cin >> outstr;
			}
			writeImage(outstr, imageCache[imageIndex]);
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
	if (argc >= 2) {
		string instr = string(argv[1]);

		//read from file
		imageCache.push_back(readImage(instr));
		imageIndex = imageCache.size()-1;
		//set output if given 3rd filename
		if (argc >= 3) {
			outstr = string(argv[2]);
		}

		//enter matrix building mode
		mtxInput(tmatrix);

		//image warp (oh dear)
		ImageRGBA warped = matrixWarp(imageCache[imageIndex], tmatrix);
		imageCache.push_back(warped);
		discardImage(imageCache[0]);
		imageCache.erase(imageCache.begin());

		//write to file and mention display controls
		if (!outstr.empty()) {
			writeImage(outstr, imageCache[imageIndex]);
		}
		else {
			cout << "press W in window to save, ";
		}
		cout << "press Q or ESC in window to exit" << endl;
	}
	else {
		cerr << "usage: warper [input].png (output).png" << endl;
		exit(1);
	}

	// start up the glut utilities
	glutInit(&argc, argv);

	// create the graphics window, giving width, height, and title text
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(DEFAULT_WIDTH, DEFAULT_HEIGHT);
	glutCreateWindow("warper");

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
