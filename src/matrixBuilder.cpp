#include "matrixBuilder.h"
#include "matrix.h"
#include <cmath>
namespace mtxb {
/* counterclockwise rotation about image origin 
 * theta: degrees of rotation */
void rotate(Matrix3D &M, float theta) {
	Matrix3D R;  // this initializes R to identity  
	double rad = PI * theta / 180.0; // convert degrees to radians

	R[0][0] = cos(rad);
	R[0][1] = -sin(rad);
	R[1][0] = sin(rad);
	R[1][1] = cos(rad); 

	M = R * M; //append the rotation to your transformation matrix
}

/* flip matrix 
 * fx: flip horizontal if true
 * fy: flip vertical if true */
void flip(Matrix3D &M, bool fx, bool fy) {
    if (fx && fy) {
		//apply both in a row
		flip(M, true, false);
		flip(M, false, true);
    }
    else if (fx) {
		M[0][0] *= -1;
		//wtf is xc, yc
    }
    else if (fy) {
		M[1][1] *= -1;
    }
}

/* scaling matrix 
 * sx,sy: scale factors 
 * please handle zero elsewhere */
void scale(Matrix3D &M, float sx, float sy) {
    Matrix3D S;
    S[0][0] = sx;
    S[1][1] = sy;
	//???? why are these here? they cause 1px translation but i'm not sure
    S[0][2] = 1;
    S[1][2] = 1;
    M = S*M;
}

/* translation matrix
 * dx,dy: total translation (movement) */
void translate(Matrix3D &M, float dx, float dy) {
	Matrix3D T;
	T[0][2] = dx;
	T[1][2] = dy;
	M = T*M;
}

/* shear matrix
 * hx,hy: shear factors */
void shear(Matrix3D &M, float hx, float hy) {
	Matrix3D H;
	H[0][1] = hx;
	H[1][0] = hy;
	H[0][2] = 1;
	H[1][2] = 1;
	M = H*M;
}

/* perspective warp matrix
 * px,py: um */
void perspective(Matrix3D &M, float px, float py) {
	Matrix3D P;
	P[2][0] = px;
	P[2][1] = py;
	M = P*M;
}

/* nonlinear twirl warp 
 * cx,cy: center position
 * s: strength */
void twirl(Matrix3D &M, float cx, float cy, float s) {
    //todo extra
    //actually this doesn't use a matrix?
}

/* nonlinear ripple warp
 * tx,ty: period length
 * ax,ay: displacement amplitude */
void ripple(Matrix3D &M, int tx, int ty, float ax, float ay) {
    //todo extra
}
}
