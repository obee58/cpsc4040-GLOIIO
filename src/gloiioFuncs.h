#ifndef GLOIIO_OB_FUNCS_H
#define GLOIIO_OB_FUNCS_H
#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <exception>

using namespace std;
OIIO_NAMESPACE_USING;

//assumed maximum value of pixel data - don't touch it kiddo
#define MAX_VAL 255
//preprocess macros aka math shorthand
#define percentOf(a,max) ((double)(a)/(max))
#define contigIndex(row,col,wid) (((row)*(wid))+(col)) //converts row & column into 1d array index
#define maximum(x,y,z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z)))
#define minimum(x,y,z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z)))

//struct that holds RGBA 4-tuple of chars
typedef struct pixel_rgba_t {
	unsigned char red, green, blue, alpha;
} pxRGBA;
//struct that holds RGB 3-tuple of chars
typedef struct pixel_rgb_t {
	unsigned char red, green, blue;
} pxRGB;
//struct that holds HSV 3-tuple of doubles
typedef struct pixel_hsv_t {
	double hue, saturation, value;
} pxHSV;
//struct that holds RGBA 4-tuple of doubles (0~1)
typedef struct floating_rgba_t {
	double red, green, blue, alpha;
} flRGBA;

//struct to tie image spec and pixels together
typedef struct image_rgba_uint8_t {
	ImageSpec spec;
	pxRGBA* pixels;
} ImageRGBA;
//struct representing .filt with calculated scale factor
typedef struct convolve_filt_t {
	int size; //NxN
	double scale;
	double* kernel;
} RawFilter;

void discardImage(ImageRGBA);
void discardRawFilter(RawFilter);
int clampInt(int,int,int);
double clampDouble(double,double,double);
pxRGB linkRGB(unsigned char,unsigned char,unsigned char);
pxRGBA linkRGBA(unsigned char,unsigned char,unsigned char,unsigned char);
pxHSV linkHSV(double,double,double);
flRGBA percentify(pxRGBA);
pxRGBA premult(pxRGBA);
pxHSV RGBtoHSV(pxRGB);
pxHSV RGBAtoHSV(pxRGBA);
ImageRGBA readImage(string);
void writeImage(string, ImageRGBA);
RawFilter readFilter(string);
ImageRGBA cloneImage(ImageRGBA);
void convolve(RawFilter, ImageRGBA);

#endif