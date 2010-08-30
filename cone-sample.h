// cone-sample.h -- Sample a cone
//
//  Copyright (C) 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __CONE_SAMPLE_H__
#define __CONE_SAMPLE_H__

#include "snogmath.h"
#include "vec.h"
#include "uv.h"


namespace snogray {


// Return a sample vector uniformly distributed over a cone centered
// around the z-axis which subtends an angle of ANGLE, which should be
// between 0 and 2*PI.  PARAM is the parameter sample.
//
static inline Vec
cone_sample (float angle, const UV &param)
{
  // Choose a slice on the upper part of a unit cylinder.  The total
  // height of the cylinder is 2 (from -1 to 1), and the height of the
  // sample area corresponds to ANGLE (so that if ANGLE is 2*PI, the
  // entire cylinder surface, from z -1 to 1, will be sampled).
  //
  // Then project the chosen slice onto a unit sphere.  Z is the
  // z-coordinate of the slice, and R is its radius.
  //
  float z = 1 - param.u * angle * INV_PIf;
  float r = sqrt (abs (1 - z * z));

  // Now choose a point around the edge of the radius R disk; X and Y
  // will be the x/y-coordinates of that point.
  //
  float phi = param.v * 2 * PIf;
  float x = r * cos (phi);
  float y = r * sin (phi);

  // Our final vector points from the origin to the chosen point on
  // the edge of the disk.
  //
  return Vec (x, y, z);
}

// Return a parameter for which cone_sample would return DIR.
//
static inline UV
cone_sample_inverse (float angle, const Vec &dir)
{
  float phi = atan2 (dir.y, dir.x);
  float v = phi * INV_PIf * 0.5f;
  if (v < 0)
    v += 1;
  float u = (1 - dir.z) * PIf / angle;
  return UV (clamp01 (u), clamp01 (v));
}


}

#endif // __CONE_SAMPLE_H__