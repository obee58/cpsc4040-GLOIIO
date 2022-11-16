/* put all the transformation matrix building/manip functions in here 
 * these all create the warp matrices before forward/inverse mapping of the image */
#ifndef GLOIIO_OB_MATRIX_H
#define GLOIIO_OB_MATRIX_H

#include "matrix.h"
#include <cmath>
namespace mtxb {
    void rotate(Matrix3D, float);
    void flip(Matrix3D, bool, bool);
    void scale(Matrix3D, float, float);
    void translate(Matrix3D, float, float);
    void shear(Matrix3D, float, float);
    void perspective(Matrix3D, float, float);
    //extra credit
    void twirl(Matrix3D, float, float, float);
    void ripple(Matrix3D, int, int, float, float);
};

#endif