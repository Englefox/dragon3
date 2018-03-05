#ifndef MATH_TRIGONOMETRY_SPHERICAL_H
#define MATH_TRIGONOMETRY_SPHERICAL_H

#include "../linear_algebra/vector.h"

namespace math
{
    /* NOTE(Joey):

      Spherical coordinates
    
    */
    struct spherical
    {
        float rho;   // distance coordinate from origin.
        float phi;   // counter-clockwise rotation from xy plane to z (z being up-axis in conventional math).
        float theta; // counter-clockwise rotation from x axis on xy plane.

        spherical(const float rho, const float phi, const float theta);
        // convert from cartesian.
        spherical(const vec3& cartesian); 

        // convert to cartesian.
        vec3 ToCartesian() const;
    };
} // namespace math

#endif