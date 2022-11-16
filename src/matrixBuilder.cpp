#include "matrixBuilder.h"

/* counterclockwise rotation about image origin 
 * theta: degrees of rotation */
void mtxb::rotate(Matrix3D &M, float theta) {
	Matrix3D R;  // this initializes R to identity  
	double rad = PI * theta / 180.0; // convert degrees to radians

	// todo: populate the rotation matrix
	/*R[0][0] = ;
	R[0][1] = ;
	R[1][0] = ;
	R[1][1] = ; 
	*/

	M = R * M; //append the rotation to your transformation matrix
}

/* flip matrix 
 * fx: flip horizontal if true
 * fy: flip vertical if true */
void mtxb::flip(Matrix3D &M, bool fx, bool fy) {
    //todo
}

/* scaling matrix 
 * sx,sy: scale factors 
 * please handle zero elsewhere */
void mtxb::scale(Matrix3D &M, float sx, float sy) {
    //todo
}

/* translation matrix
 * dx,dy: total translation (movement) */
void mtxb::translate(Matrix3D &M, float dx, float dy) {
    //todo
}

/* shear matrix
 * hx,hy: shear factors */
void mtxb::shear(Matrix3D &M, float hx, float hy) {
    //todo
}

/* perspective warp matrix
 * px,py: um */
void mtxb::perspective(Matrix3D &M, float px, float py) {
    //todo
}

/* nonlinear twirl warp 
 * cx,cy: center position
 * s: strength */
void mtxb::twirl(Matrix3D &M, float cx, float cy, float s) {
    //todo extra
}

/* nonlinear ripple warp
 * tx,ty: period length
 * ax,ay: displacement amplitude */
void mtxb::ripple(Matrix3D &M, int tx, int ty, float ax, float ay) {
    //todo extra
}