#include "gloiioFuncs.h"

//memory of loaded files/data
vector<ImageRGBA> imageCache;
vector<RawFilter> filtCache;
//current index in vector to draw/use/modify
static int imageIndex = 0;
static int filtIndex = 0;

/** UTILITY FUNCTIONS **/
/*	clean up memory of unneeded ImageRGBA
 *	note this will not deallocate the ImageSpec i don't think? */
void discardImage(ImageRGBA image) {
	delete[] image.pixels;
}
/*	clean up memory of unneeded RawFilter */
void discardRawFilter(RawFilter filt) {
	delete[] filt.kernel;
}

/* math functions i wasn't sure i could do as preprocessor macros */
int clampInt(int x, int min, int max) {
	if (x < min) { return min; }
	if (x > max) { return max; }
	return x;
}
double clampDouble(double x, double min, double max) {
	if (x < min) { return min; }
	if (x > max) { return max; }
	return x;
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

/* get percents of maximum from pxRGBA as a flRGBA */
flRGBA percentify(pxRGBA px) {
	flRGBA pct;
	pct.red = double(px.red)/MAX_VAL;
	pct.green = double(px.green)/MAX_VAL;
	pct.blue = double(px.blue)/MAX_VAL;
	pct.alpha = double(px.alpha)/MAX_VAL;
	return pct;
}

/* get premultiplied/associated version of a pxRGBA */
pxRGBA premult(pxRGBA pure) {
	double a = double(pure.alpha)/MAX_VAL; //alpha 0~1
	unsigned char r = pure.red*a;
	unsigned char g = pure.green*a;
	unsigned char b = pure.blue*a;
	return linkRGBA(r,g,b,pure.alpha);
}

/*convert RGB values into HSV values*/
pxHSV RGBtoHSV(pxRGB rgb) {
	pxHSV hsv;
	double huer, hueg, hueb, max, min, delta;
	/* convert from 0-MAX_VAL to 0~1 */
	huer = rgb.red / double(MAX_VAL);
	hueg = rgb.green / double(MAX_VAL);
	hueb = rgb.blue / double(MAX_VAL);

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
				image.pixels[i].alpha = MAX_VAL;
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
				image.pixels[i].alpha = MAX_VAL;
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
	imageIndex = imageCache.size()-1;
}

/* writes currently dixplayed pixmap (as RGBA) to a file
	(mostly the same as sample code) */
void writeImage(string filename){
	int xr = imageCache[imageIndex].spec.width;
	int yr = imageCache[imageIndex].spec.height;
	int channels = 4;
	//temporary 1d array to stick all the pxRGBA data into
	//write_image does not like my structs >:(
	unsigned char temp_px[xr*yr*channels];
	for (int i=0; i<xr*yr; i++) {
		temp_px[(4*i)] = imageCache[imageIndex].pixels[i].red;
		temp_px[(4*i)+1] = imageCache[imageIndex].pixels[i].green;
		temp_px[(4*i)+2] = imageCache[imageIndex].pixels[i].blue;
		temp_px[(4*i)+3] = imageCache[imageIndex].pixels[i].alpha;
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


/*  reads in filter data from specified filename as RawFilter
	puts a new RawFilter in the filtCache vector if successful */
void readFilter(string filename) {
	ifstream data(filename);
	if (!data) {
		cerr << "could not open .filt file!" << endl;
		return;
	}
	
	RawFilter filt;
	//read in size & allocate accordingly
	data >> filt.size;
	int n = filt.size;
	filt.kernel = new double[filt.size*filt.size];

	//read in weights until kernel is full
	for (int r=0; r<n; r++) {
		for (int c=0; c<n; c++) {
			data >> filt.kernel[contigIndex(r,c,n)];
		}
	}

	//find max magnitude of positive and negative sums
	int ind = 0;
	double posMag = 0.0;
	double negMag = 0.0;
	while (ind < n*n) {
		double num = filt.kernel[ind++];
		if (num > 0.0) { posMag += num; }
		else { negMag -= num; }
	}
	double max = (posMag>negMag)? posMag:negMag;
	filt.scale = max;
	
	filtCache.push_back(filt);
	//select this filter
	filtIndex = filtCache.size()-1;
}

/* makes a copy of image in imageCache[index] and adds it to the imageCache
 * useful to support reverting changes at the cost of extra memory usage 
 * if you don't like that, call readImage() again to get it from disk instead 
 * returns last index of imageCache after copy is made */
int cloneImage(int index) {
	ImageRGBA copyImage;
	ImageRGBA origImage = imageCache[index];
	int h = origImage.spec.height;
	int w = origImage.spec.width;

	copyImage.spec = origImage.spec;
	copyImage.pixels = new pxRGBA[h*w];
	for (int i=0; i<h*w; i++) {
		copyImage.pixels[i] = origImage.pixels[i];
	}
	imageCache.push_back(copyImage);
	return imageCache.size()-1;
}

/* removes an image from the imageCache */
int removeImage(int index) {
	if (imageCache.size() > index) {
		discardImage(imageCache[index]);
		imageCache.erase(imageCache.begin()+index);
	}
	else {
		return -1; //cannot remove from empty cache
	}
	return imageCache.size();
}

/* removes a filter from the filtCache */
int removeFilt(int index) {
	if (filtCache.size() > index) {
		discardRawFilter(filtCache[index]);
		filtCache.erase(filtCache.begin()+index);
	}
	else {
		return -1; //cannot remove from empty cache
	}
	return filtCache.size();
}


/* apply convolution filter to current image
 * this function currently uses zero padding for boundaries
 * and clamps final values between 0 and MAX_VAL */
//TODO make sure the shift & darken is because of the subpar boundary condition and not an actual bug
void convolve(RawFilter filt) {
	/* REMEMBER THE PIXMAPS ARE VERTICALLY FLIPPED - PIXEL 0 IS AT BOTTOM LEFT */
	//flip the kernel horizontally and vertically before applying (read backwards)
	int n = filt.size;
	int nind = n-1; //IM STUPID AND SO ARE ORDINALS
	double* tempkern = new double[n*n];
	for (int row=0; row<n; row++) {
		for (int col=0; col<n; col++) {
			tempkern[contigIndex(row,col,n)] = filt.kernel[contigIndex(nind-row,nind-col,n)];
		}
	}

	ImageRGBA victim = imageCache[imageIndex];
	int iheight = victim.spec.height;
	int iwidth = victim.spec.width;
	int boundary = iheight*iwidth; //at this value, past the pixel array
	//cout << "this image is " << iwidth << "x" << iheight << endl;
	pxRGBA* result = new pxRGBA[iheight*iwidth]; //do not do this in-place
	//per pixel...
	for (int irow=0; irow<iheight; irow++) {
		for (int icol=0; icol<iwidth; icol++) {
			int iindex = contigIndex(irow,icol,iwidth);
			pxRGBA itarget = (victim.pixels[iindex]); //image index
			//bool debugCond = (iindex > boundary-(n) || iindex < n || iindex%iwidth == 0);

			//per RGB channel...
			double totalRed = 0.0;
			double totalGreen = 0.0;
			double totalBlue = 0.0;
			
			//per filter data point...
			for (int frow=0; frow<n; frow++) {
				for (int fcol=0; fcol<n; fcol++) {
					int targetRow = irow+(frow-(n/2));
					int targetCol = icol+(fcol-(n/2));
					double weight = tempkern[contigIndex(frow,fcol,n)];
					int tindex = contigIndex(targetRow,targetCol,iwidth); //target (filter) index
					//boundary check
					//targetCol checks are to prevent bleeding of edge of last row or beginning of next row
					if (tindex > 0 && tindex < boundary
						&& targetCol >= 0 && targetCol < iwidth) {
						pxRGBA ftarget = (victim.pixels[tindex]);
						totalRed += (double)(ftarget.red) * weight;
						totalGreen += (double)(ftarget.green) * weight;
						totalBlue += (double)(ftarget.blue) * weight;
						//if (debugCond) {
						//	cout << "from " << targetRow << "," << targetCol << " (" << tindex << ", offset " << frow-(n/2) << "," << fcol-(n/2) << "): ";
						//	cout << (double)(ftarget.red) << " " << (double)ftarget.green << " " << (double)ftarget.blue << ", " << weight << endl;
						//}
					}
					else {
						//if OOB, pad with values of original pixel
						totalRed += (double)(itarget.red) * weight;
						totalGreen += (double)(itarget.green) * weight;
						totalBlue += (double)(itarget.blue) * weight;
						//if (debugCond) {
						//	cout << "out of bounds! ";
						//	cout << "from " << targetRow << "," << targetCol << "(offset " << frow-(n/2) << "," << fcol-(n/2) << "): ";
						//	cout << (double)itarget.red << " " << (double)itarget.green << " " << (double)itarget.blue << ", " << weight << endl;
						//}
					}
				}
			}
			//scale, clamp, and apply to new pixel
			pxRGBA* npx = &(result[iindex]);
			npx->red = (unsigned char)clampDouble(totalRed/filt.scale, 0, MAX_VAL);
			npx->green = (unsigned char)clampDouble(totalGreen/filt.scale, 0, MAX_VAL);
			npx->blue = (unsigned char)clampDouble(totalBlue/filt.scale, 0, MAX_VAL);
			npx->alpha = victim.pixels[iindex].alpha; //don't touch alpha
			//if (debugCond) {
			//	cout << "pixel: " << irow << "," << icol << " (" << iindex << ")" << endl;
			//	cout << "totals: " << totalRed << " " << totalGreen << " " << totalBlue << ", scale " << filt.scale << endl;
			//	cout << "final value: " << (int)npx->red << " " << (int)npx->green << " " << (int)npx->blue << " " << (int)npx->alpha << endl;
			//}
		}
	}

	//copy result over victim.pixels
	for (int i=0; i<iheight; i++) {
		for (int j=0; j<iwidth; j++) {
			int index = contigIndex(i,j,iwidth);
			victim.pixels[index].red = result[index].red;
			victim.pixels[index].green = result[index].green;
			victim.pixels[index].blue = result[index].blue;
		}
	}
	delete[] tempkern;
	delete[] result;
}