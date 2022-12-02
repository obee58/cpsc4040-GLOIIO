/*
  Example inverse warps
*/

#include <cmath>

/*
  Routine to inverse map (x, y) output image spatial coordinates
  into (u, v) input image spatial coordinates

  Call routine with (x, y) spatial coordinates in the output
  image. Returns (u, v) spatial coordinates in the input image,
  after applying the inverse map. Note: (u, v) and (x, y) are not 
  rounded to integers, since they are true spatial coordinates.
 
  inwidth and inheight are the input image dimensions
  outwidth and outheight are the output image dimensions
*/
void inv_map(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = x/2;
  v = y/2; 
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}

void inv_map2(float x, float y, float &u, float &v,
	int inwidth, int inheight, int outwidth, int outheight){
  
  x /= outwidth;		// normalize (x, y) to (0...1, 0...1)
  y /= outheight;

  u = 0.5 * (x * x * x * x + sqrt(sqrt(y)));
  v = 0.5 * (sqrt(sqrt(x)) + y * y * y * y);
  
  u *= inwidth;			// scale normalized (u, v) to pixel coords
  v *= inheight;
}
