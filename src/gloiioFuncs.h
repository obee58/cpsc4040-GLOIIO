#ifndef GLOIIO_OB_FUNCS_H
#define GLOIIO_OB_FUNCS_H
#include <OpenImageIO/imageio.h>
#include <iostream>
#include <fstream>
#include <climits>
#include <string>
#include <cmath>
#include <random>
#include <algorithm>
#include <exception>

using namespace std;
OIIO_NAMESPACE_USING;

//assumed channel data sizes and maximum uint value; don't touch unless image formats go crazy in the future
#define MAX_VAL 255
typedef unsigned char ch_uint;
//maximum filesize in bytes before readRaw cuts you off
#define RAW_MAX 1048576 //1 mebibyte
//maximum resolution before you should probably tell the user to stop
#define RES_MAX 4096
//preprocess macros aka math shorthand
#define percentOf(a,max) ((double)(a)/(max)) //ex. 3 is 6% of 50, finds 6%
#define contigIndex(row,col,wid) (((row)*(wid))+(col)) //converts row & column into 1d array index
#define maximum(x,y,z) ((x) > (y)? ((x) > (z)? (x) : (z)) : ((y) > (z)? (y) : (z))) //triple max
#define minimum(x,y,z) ((x) < (y)? ((x) < (z)? (x) : (z)) : ((y) < (z)? (y) : (z))) //triple min

//struct that holds RGBA 4-tuple of uints
typedef struct pixel_rgba_t {
	ch_uint red, green, blue, alpha;
} pxRGBA;
//struct that holds RGB 3-tuple of uints
typedef struct pixel_rgb_t {
	ch_uint red, green, blue;
} pxRGB;
//struct that holds HSV 3-tuple of floats
typedef struct pixel_hsv_t {
	double hue, saturation, value;
} pxHSV;
//struct that holds RGBA 4-tuple of floats (0~1)
typedef struct floating_rgba_t {
	double red, green, blue, alpha;
} flRGBA;

//struct to tie image spec and pixels together
typedef struct image_rgba_uint_t {
	ImageSpec spec;
	pxRGBA* pixels;
} ImageRGBA;
//struct to tie raw data array and its size together
typedef struct image_raw_uint_t {
	unsigned int size;
	ch_uint* array;
} ImageRaw;
//struct representing .filt with calculated scale factor
typedef struct convolve_filt_t {
	int size; //size = N; filter is NxN matrix
	double scale;
	double* kernel;
} RawFilter;
//psuedo-union for raw and rbga
typedef struct steno_idc_image_t {
	ImageRaw data;
	ImageRGBA image;
	bool isRaw;
} StenoImage;

//math & shorthand functions
void discardImage(ImageRGBA);
void discardRawFilter(RawFilter);
int clampInt(int,int,int);
double clampDouble(double,double,double);
ch_uint stepBits(int, int, ch_uint);
ch_uint overwriteBits(int, ch_uint, ch_uint);
//type conversion & struct creation functions
pxRGB linkRGB(ch_uint,ch_uint,ch_uint);
pxRGBA linkRGBA(ch_uint,ch_uint,ch_uint,ch_uint);
pxHSV linkHSV(double,double,double);
flRGBA percentify(pxRGBA);
pxRGBA premult(pxRGBA);
pxHSV RGBtoHSV(pxRGB);
pxHSV RGBAtoHSV(pxRGBA);
ImageRGBA cloneImage(ImageRGBA);
//i/o functions
ImageRGBA readImage(string);
ImageRaw readRaw(string);
void writeImage(string, ImageRGBA);
void writeRaw(string, ImageRaw);
RawFilter readFilter(string);
//manipulation functions
void invert(ImageRGBA);
void noisify(ImageRGBA, int, int);
void chromaKey(ImageRGBA, pxHSV, double, double, double);
void compose(ImageRGBA, ImageRGBA);
void convolve(RawFilter, ImageRGBA);
ImageRGBA scale(ImageRGBA, double, double); //overall scaling factor (main code goes in here)
//maybe loose translation function to place small secret at specific position?
ImageRGBA encodeImage(ImageRGBA, ImageRGBA, int); //cover, secret, bits
ImageRGBA encodeData(ImageRGBA, ImageRaw, int);
ImageRGBA decodeImage(ImageRGBA, int); //target, bits
ImageRaw decodeData(ImageRGBA, int); //target, bits

#endif