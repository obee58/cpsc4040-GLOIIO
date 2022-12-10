#include "gloiioFuncs.h"

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

/* clean replacement of lower bits */
ch_uint overwriteBits(int bits, ch_uint upper, ch_uint lower) {
	//TODO powers of 2?
	return 0;
}

/* functions to quickly create pxRGB, pxRGBA, pxHSV,... from arbitrary values*/
pxRGB linkRGB(ch_uint r, ch_uint g, ch_uint b) {
	pxRGB px;
	px.red = r;
	px.green = g;
	px.blue = b;
	return px;
}
pxRGBA linkRGBA(ch_uint r, ch_uint g, ch_uint b, ch_uint a) {
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
	ch_uint r = pure.red*a;
	ch_uint g = pure.green*a;
	ch_uint b = pure.blue*a;
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
	returns an ImageRGBA if successful
	THROWS EXCEPTION ON IO FAIL - place in trycatch block if called outside of init */
ImageRGBA readImage(string filename) {
	std::unique_ptr<ImageInput> in = ImageInput::open(filename);
	if (!in) {
		std::cerr << "could not open input file! " << geterror();
		//cancel routine
		throw runtime_error("image input fail");
	}

	//store spec and get metadata from it
	ImageRGBA image;
	image.spec = in->spec();
	int xr = image.spec.width;
	int yr = image.spec.height;
	int channels = image.spec.nchannels;

	//declare temp memory to read raw image data
	ch_uint temp_px[xr*yr*channels];

	// read the image into the temp_px from the input file, flipping it upside down using negative y-stride,
	// since OpenGL pixmaps have the bottom scanline first, and 
	// oiio expects the top scanline first in the image file.
	int scanlinesize = xr * channels * sizeof(ch_uint);
	if(!in->read_image(TypeDesc::UINT8, temp_px+((yr-1)*scanlinesize), AutoStride, -scanlinesize)){
		cerr << "Could not read image from " << filename << ", error = " << geterror() << endl;
		//cancel routine
		throw runtime_error("image input fail");
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
	return image;
}

/*	forcefully reads a file as an """image""" regardless of what it is
	does not actually use OIIO, but creates a partial ImageSpec for later handling
	dimensions are not defined, draw() & steno functions apply a sort of "word wrap"
	THROWS EXCEPTION ON IO FAIL - place in trycatch block if called outside of init */
ImageRaw readRaw(string filename) {
	fstream in;
	in.open(filename, fstream::in);
	if (in.fail() || !in) {
		std::cerr << "could not open input file! " << endl;
		//cancel routine
		throw runtime_error("raw input fail");
	}

	//find size of file (OS-agnostic i think)
	in.seekg(0, ios::end);
	unsigned int filesize = in.tellg(); //please do not give this program a 4.294 GB file
	if (filesize > RAW_MAX || in.fail()) {
		std::cerr << "input file too large! got" << filesize << "bytes, limit is " << RAW_MAX << endl;
		//cancel routine
		throw runtime_error("raw size fail");
	}
	in.seekg(0, ios::beg);

	//declare memory to store data in
	ImageRaw image;
	image.array = new ch_uint[filesize];
	image.size = filesize;

	//read by byte
	for (unsigned int i=0; i<filesize; i++) {
		image.array[i] = in.get();
		if (in.eof() || !in) { break; } //TODO maybe clean error
	}

	in.close();
	return image;
}

/* writes currently dixplayed pixmap (as RGBA) to a file
	(mostly the same as sample code) */
void writeImage(string filename, ImageRGBA image){
	int xr = image.spec.width;
	int yr = image.spec.height;
	int channels = 4;
	//temporary 1d array to stick all the pxRGBA data into
	//write_image does not like my structs >:(
	ch_uint temp_px[xr*yr*channels];
	for (int i=0; i<xr*yr; i++) {
		temp_px[(4*i)] = image.pixels[i].red;
		temp_px[(4*i)+1] = image.pixels[i].green;
		temp_px[(4*i)+2] = image.pixels[i].blue;
		temp_px[(4*i)+3] = image.pixels[i].alpha;
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
	// type ch_uint
	ImageSpec spec(xr, yr, channels, TypeDesc::UINT8);
	if(!outfile->open(filename, spec)){
		cerr << "could not open output file! " << geterror() << endl;
		return;
	}

	// write the image to the file. All channel values in the pixmap are taken to be
	// ch_uints. flip using stride to undo same effort in readImage
	int scanlinesize = xr * channels * sizeof(ch_uint);
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

/*	writes non-image data to a file without OIIO */
void writeRaw(string filename, ImageRaw data) {
	//TODO more fstream stuff
	fstream out;
	out.open(filename, fstream::out);
	if (out.fail() || !out) {
		std::cerr << "could not open output file! " << endl;
		//cancel routine
		return;
	}

	//write by byte
	for (unsigned int i=0; i<data.size; i++) {
		out.put(data.array[i]);
		if (!out) { break; } //IDK
	}
	out.close();
}

/*  reads in filter data from specified filename as RawFilter
	returns a RawFilter for the filtCache if successful 
	THROWS EXCEPTION ON IO FAIL - place in trycatch block if called outside of init */
RawFilter readFilter(string filename) {
	ifstream data(filename);
	if (!data) {
		cerr << "could not open .filt file!" << endl;
		throw runtime_error("filter input fail");
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
	
	return filt;
}

/* makes a copy of an image
 * useful to support reverting changes at the cost of extra memory usage 
 * if you don't like that, call readImage() again to get it from disk instead */
ImageRGBA cloneImage(ImageRGBA origImage) {
	ImageRGBA copyImage;
	int h = origImage.spec.height;
	int w = origImage.spec.width;

	copyImage.spec = origImage.spec;
	copyImage.pixels = new pxRGBA[h*w];
	for (int i=0; i<h*w; i++) {
		copyImage.pixels[i] = origImage.pixels[i];
	}
	return copyImage;
}

/** IMAGE MODIFICATION FUNCTIONS **/
/* inverts all colors of the currently loaded image
	ignores alpha channel */
void invert(ImageRGBA image) {
	//wow!! this is a lot easier now
	int xr = image.spec.width;
	int yr = image.spec.height;
	for (int i=0; i<xr*yr; i++) {
		image.pixels[i].red = MAX_VAL - image.pixels[i].red;
		image.pixels[i].green = MAX_VAL - image.pixels[i].green;
		image.pixels[i].blue = MAX_VAL - image.pixels[i].blue;
	}
}

/* randomly replaces pixels with black
	chance defined by 1/noiseDenom */
void noisify(ImageRGBA image, int noiseDenom, int seed) {
	default_random_engine gen(time(NULL)+seed);
	uniform_int_distribution<int> dist(1,noiseDenom);
	auto rando = bind(dist,gen);
	
	int xr = image.spec.width;
	int yr = image.spec.height;
	for (int i=0; i<xr*yr; i++) {
		if (rando() == 1) {
			//set the color values to 0 and alpha to max
			image.pixels[i].red = 0;
			image.pixels[i].green = 0;
			image.pixels[i].blue = 0;
			image.pixels[i].alpha = MAX_VAL;
		}
	}
}

/* chroma-key image to create alphamask using HSV differences (overwrites)
	"fuzz" arguments determine max difference for each value to keep */
void chromaKey(ImageRGBA image, pxHSV target, double huefuzz, double satfuzz, double valfuzz) {
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
			image.pixels[i].alpha = (ch_uint)MAX_VAL*maskalpha;
		}
	}
}

/* slap image A over image B, compositing them into just image B (overwrites)
	both indices must be in bounds, A must be same size or smaller than B */
/** IN MEMORIAM OF int Bindex, 2022-2022 */
void compose(ImageRGBA imgA, ImageRGBA imgB) {
	ImageRGBA* A = &imgA;
	ImageRGBA* B = &imgB;
	ImageSpec* specA = &(A->spec);
	ImageSpec* specB = &(B->spec);
	if (specA->height > specB->height || specA->width > specB->width) {
		cerr << "foreground is too big to fit on background!" << endl;
		return;
	}
	for (int i=0; i<specB->height*specB->width; i++) {
		//make 0~1 value copies of pixels & their premultiplied versions
		flRGBA pxpctA = percentify(A->pixels[i]);
		flRGBA pxpctB = percentify(B->pixels[i]);
		flRGBA prmpctA = percentify(premult(A->pixels[i]));
		flRGBA prmpctB = percentify(premult(B->pixels[i]));
		//apply over to each channel, overwriting background B
		B->pixels[i].red = 255*(prmpctA.red + (1.0-pxpctA.alpha)*prmpctB.red);
		B->pixels[i].green = 255*(prmpctA.green + (1.0-pxpctA.alpha)*prmpctB.green);
		B->pixels[i].blue = 255*(prmpctA.blue + (1.0-pxpctA.alpha)*prmpctB.blue);
		B->pixels[i].alpha = 255*(pxpctA.alpha + (1.0-pxpctA.alpha)*pxpctB.alpha);
	}
}

/* apply convolution filter to current image, overwriting it when done
 * this function currently uses zero padding for boundaries
 * and clamps final values between 0 and MAX_VAL */
void convolve(RawFilter filt, ImageRGBA victim) {
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

	int iheight = victim.spec.height;
	int iwidth = victim.spec.width;
	int boundary = iheight*iwidth; //at this value, past the pixel array
	//cout << "this image is " << iwidth << "x" << iheight << endl;
	pxRGBA* result = new pxRGBA[iheight*iwidth]; //let's not do this entirely in-place
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
			npx->red = (ch_uint)clampDouble(totalRed/filt.scale, 0, MAX_VAL);
			npx->green = (ch_uint)clampDouble(totalGreen/filt.scale, 0, MAX_VAL);
			npx->blue = (ch_uint)clampDouble(totalBlue/filt.scale, 0, MAX_VAL);
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

/* use a simple warp to scale down/up the image to fit desired size 
 * can scale width and height independently */
ImageRGBA scale(ImageRGBA image, double factorx, double factory) {
	/* target image size */
	int oxRes = image.spec.width; //original resolution
	int oyRes = image.spec.height;
	//NOTE maybe round/ceil/floor these?
	int txRes = image.spec.width*factorx; //target resolution
	int tyRes = image.spec.height*factory;
	
	/* allocate output */
	cout << "allocating space " << txRes << "x" << tyRes << endl; //debug
	pxRGBA* output = new pxRGBA[txRes*tyRes];

	/* apply inverse map */
	cout << txRes << "," << tyRes << endl;
	for (int iny=0; iny<tyRes; iny++) {
		for (int inx=0; inx<txRes; inx++) {
			//TODO reference okwarp to make sure this logic is correct
			//definitions of "input" and "output" in inverse map are really confusing!!!

			//normalize x,y to 0...1 ("output pixel")
			double outx = (double)inx/txRes;
			double outy = (double)iny/tyRes;
			//calculate u,v and scale back to pixel coords
			double u = outx*oxRes;
			double v = outy*oyRes;
			//TODO better interpolation can go here
			int inu = floor(u);
			int inv = floor(v);
			//apply warp by copying pixel
			if (inx%100==0) {cout << inx << "," << iny << " <-- " << inu << "," << inv << endl;}
			output[contigIndex(iny,inx,txRes)] = image.pixels[contigIndex(inv,inu,oxRes)];
		}
	}

	/* redefine ImageRGBA */
	ImageRGBA result;
	result.pixels = output;
	result.spec = image.spec; //i can only pray that this works fine for writing to file
	result.spec.height = tyRes;
	result.spec.width = txRes;
	return result;
}

/* encodes a secret image in the last few bits of a cover image */
ImageRGBA encode(ImageRGBA cover, StenoImage secret, int bits) {
	//TODO major
	return cover;
}

/* decodes a secret image by removing all but the last few bits of it and amplifying them */
ImageRGBA decode(ImageRGBA target, int bits) {
	//TODO major
	return target;
}