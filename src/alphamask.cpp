//	OpenGL/GLUT Program to create alpha masks for any greenscreen image
//	Displays resulting image when done & exports to file
//
//	Usage: alphamask input.(img) output.png [3 floats HSV of target] [3 floats HSV of tolerance]
//	Input can be any image type, output will be png
//
//	CPSC 4040 | Owen Book | October 2022

#include <OpenImageIO/imageio.h>
#include <iostream>
#include <string>
#include <cmath>

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
//preprocess macros
#define maximum(x,y,z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x,y,z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

//utility struct that holds RGBA 4-tuple of chars
typedef struct pixel_rgba_t {
	unsigned char red, green, blue, alpha;
} pxRGBA;
//utility struct that holds RGB 3-tuple of chars
typedef struct pixel_rgb_t {
	unsigned char red, green, blue;
} pxRGB;
//utility struct that holds HSV 3-tuple of doubles
typedef struct pixel_hsv_t {
	double hue, saturation, value;
} pxHSV;

//struct to tie image spec and pixels together
typedef struct image_rgba_uint8_t {
	ImageSpec spec;
	pxRGBA* pixels;
} ImageRGBA;


/** CONTROL & GLOBAL STATICS **/
//list of read images for multi-image viewing mode
static vector<ImageRGBA> imageCache;
//current index in vector to attempt to load (probably won't be touched in this program)
static int cacheIndex = 0;

/** UTILITY FUNCTIONS **/
/*	clean up memory of unneeded ImageRGBA
	don't forget to remove it from imageCache first! */
void discardImage(ImageRGBA image) {
	delete image.pixels;
}

/* functions to quickly create pxRGB, pxRGBA, pxHSV,... from arbitrary values*/
pxRGB linkRGB(unsigned char r, unsigned char g, unsigned char b) {
	pxRGB px;
	px.red = r;
	px.green = g;
	px.blue = b;
	return px;
}
pxRGBA linkRGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
	pxRGBA px;
	px.red = r;
	px.green = g;
	px.blue = b;
	px.alpha = a;
	return px;
}
pxHSV linkHSV(double h, double s, double v) {
	pxHSV px;
	px.hue = h;
	px.saturation = s;
	px.value = v;
	return px;
}

/*convert RGB values into HSV values*/
pxHSV RGBtoHSV(pxRGB rgb) {
	pxHSV hsv;
	double huer, hueg, hueb, max, min, delta;
	/* convert from 0-255 to 0~1 */
	huer = rgb.red / 255.0;
	hueg = rgb.green / 255.0;
	hueb = rgb.blue / 255.0;

	max = maximum(huer, hueg, hueb);
	min = minimum(huer, hueg, hueb);
	hsv.value = max;

	if (max==0) {
		hsv.hue = 0;
		hsv.value = 0;
	}
	else {
		delta = max-min;
		hsv.saturation = delta/max; //https://youtu.be/QYbG1AMSdiY
		if (delta == 0) {
			hsv.hue = 0;
		}
		else {
			/* determine hue */
			if (huer == max) {
				hsv.hue = (hueg - hueb) / delta;
			}
			else if (hueg == max) {
				hsv.hue = 2.0 + (hueb - huer) / delta;
			}
			else {
				hsv.hue = 4.0 + (huer - hueg) / delta;
			}
			hsv.hue *= 60.0;
			if (hsv.hue < 0) {
				hsv.hue += 360.0;
			}
		}
	}

	return hsv;
}

/* shorthand function that double-converts RGBA to HSV, ignoring alpha channel */
pxHSV RGBAtoHSV(pxRGBA rgba) {
	return RGBtoHSV(linkRGB(rgba.red, rgba.green, rgba.blue));
}

/** PROCESSING FUNCTIONS **/
/*  reads in image from specified filename as RGBA 8bit pixmap
	puts a new ImageRGBA in the imageCache vector if successful */
void readImage(string filename) {
	std::unique_ptr<ImageInput> in = ImageInput::open(filename);
	if (!in) {
		std::cerr << "could not open input file! " << geterror();
		//cancel routine
		return;
	}

	//store spec and get metadata from it
	ImageRGBA image;
	image.spec = in->spec();
	int xr = image.spec.width;
	int yr = image.spec.height;
	int channels = image.spec.nchannels;

	//declare temp memory to read raw image data
	unsigned char temp_px[xr*yr*channels];

	// read the image into the temp_px from the input file, flipping it upside down using negative y-stride,
	// since OpenGL pixmaps have the bottom scanline first, and 
	// oiio expects the top scanline first in the image file.
	int scanlinesize = xr * channels * sizeof(unsigned char);
	if(!in->read_image(TypeDesc::UINT8, temp_px+((yr-1)*scanlinesize), AutoStride, -scanlinesize)){
		cerr << "Could not read image from " << filename << ", error = " << geterror() << endl;
		//cancel routine
		return;
  	}
	
	//allocate data for converted pxRGBA version
	image.pixels = new pxRGBA[xr*yr];
	//convert and store raw image data as pxRGBAs
	for (int i=0; i<yr*xr; i++) {
		switch(channels) {
			//this could be more cleanly programmed but the less convoluted the better
			case 1: //grayscale
				image.pixels[i].red = temp_px[i];
				image.pixels[i].green = temp_px[i];
				image.pixels[i].blue = temp_px[i];
				image.pixels[i].alpha = 255;
				break;
			case 2: //weird grayscale with alpha (just covering my ass here)
				image.pixels[i].red = temp_px[2*i];
				image.pixels[i].green = temp_px[2*i];
				image.pixels[i].blue = temp_px[2*i];
				image.pixels[i].alpha = temp_px[(2*i)+1];
				break;
			case 3: //RGB
				image.pixels[i].red = temp_px[(3*i)];
				image.pixels[i].green = temp_px[(3*i)+1];
				image.pixels[i].blue = temp_px[(3*i)+2];
				image.pixels[i].alpha = 255;
				break;
			case 4: //RGBA
				image.pixels[i].red = temp_px[(4*i)];
				image.pixels[i].green = temp_px[(4*i)+1];
				image.pixels[i].blue = temp_px[(4*i)+2];
				image.pixels[i].alpha = temp_px[(4*i)+3];
				break;
			default: //something weird, just do nothing
				break;
		}
	}

	//close input
	in->close();
	//store struct in cache and switch to display this image
	imageCache.push_back(image);
	cacheIndex = imageCache.size()-1;
}

/* writes currently dixplayed pixmap (as RGBA) to a file
	(mostly the same as sample code) */
void writeImage(string filename){
	int xr = imageCache[cacheIndex].spec.width;
	int yr = imageCache[cacheIndex].spec.height;
	int channels = 4;
	//temporary 1d array to stick all the pxRGBA data into
	//write_image does not like my structs >:(
	unsigned char temp_px[xr*yr*channels];
	for (int i=0; i<xr*yr; i++) {
		temp_px[(4*i)] = imageCache[cacheIndex].pixels[i].red;
		temp_px[(4*i)+1] = imageCache[cacheIndex].pixels[i].green;
		temp_px[(4*i)+2] = imageCache[cacheIndex].pixels[i].blue;
		temp_px[(4*i)+3] = imageCache[cacheIndex].pixels[i].alpha;
	}

	// create the oiio file handler for the image
	std::unique_ptr<ImageOutput> outfile = ImageOutput::create(filename);
	if(!outfile){
		cerr << "could not create output file! " << geterror() << endl;
		//cancel routine
		return;
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
	// unsigned chars. flip using stride to undo same effort in readImage
	int scanlinesize = xr * channels * sizeof(unsigned char);
	if(!outfile->write_image(TypeDesc::UINT8, temp_px+((yr-1)*scanlinesize), AutoStride, -scanlinesize)){
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

/* chroma-key image to create alphamask using HSV differences
	"fuzz" arguments determine max difference for each value to keep */
void chromaKey(pxHSV target, double huefuzz, double satfuzz, double valfuzz) {
	ImageRGBA image = imageCache[cacheIndex];
	int xr = image.spec.width;
	int yr = image.spec.height;
	for (int i=0; i<xr*yr; i++) {
		pxHSV comp = RGBAtoHSV(image.pixels[i]);
		//cut out based on absolute distance from target values
		//if all three are in range, hide it!
		double huediff = fabs(comp.hue - target.hue);
		double satdiff = fabs(comp.saturation - target.saturation);
		double valdiff = fabs(comp.value - target.value);
		if (huediff < huefuzz && satdiff < satfuzz && valdiff < valfuzz) {
			//some smoothing function for pixels way less close
			double maskalpha = (0.2*(huediff/huefuzz) + 0.4*(satdiff/satfuzz) + 0.4*(valdiff/valfuzz)) - 0.2;
			//clamp to 0.0~1.0; if the result is like 0.012 don't bother with it
			maskalpha = (maskalpha < 0.02? 0.0 : maskalpha);
			maskalpha = (maskalpha > 1.0? 1.0 : maskalpha);
			
			//cout << "masking pixel: " << comp.hue << " " << comp.saturation << " " << comp.value << " to alpha value " << maskalpha << endl;
			image.pixels[i].alpha = (unsigned char)255*maskalpha;
		}
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
		readImage(instr);
		chromaKey(target,fuzz.hue,fuzz.saturation,fuzz.value);
		cout << "writing alphamask to file " << outstr << endl;
		writeImage(outstr);
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
