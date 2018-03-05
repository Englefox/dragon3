#ifndef MATH_TRIGONOMETRY_POLAR_H
#define MATH_TRIGONOMETRY_POLAR_H

#include "../linear_algebra/vector.h"

namespace math
{
    /*  
      Polar coordinates w/ relevant conversion for
      converting to and from cartesian coordinates.
    */
    struct polar
    {
        float r;     //distance coordinate from origin.
        float theta; //counter-clockwise rotation from x-axis.

        polar(float r, float theta);
        // convert from cartesian.
        polar(vec2 cartesian); 

        // convert to cartesian.
        vec2 ToCartesian();
    };
}

#endif