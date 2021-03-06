// image-mitchell-filt.h -- Mitchell filter for image output
//
//  Copyright (C) 2006, 2007, 2010-2012  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef SNOGRAY_IMAGE_MITCHELL_FILT_H
#define SNOGRAY_IMAGE_MITCHELL_FILT_H

#include "image-filter.h"


namespace snogray {


// 2D mitchell filter
//
class ImageMitchellFilt : public ImageFilter
{
public:

  // These should be simple named constants, but C++ (stupidly)
  // disallows non-integral named constants.  Someday when "constexpr"
  // support is widespread, that can be used instead.
  static float default_b () { return 1.f / 3.f; }
  static float default_c () { return 1.f / 3.f; }
  static float default_radius () { return 2; }

  ImageMitchellFilt (float _b = default_b(), float _c = default_c(),
		     float _radius = default_radius())
    : ImageFilter (_radius), b (_b), c (_c)
  { }
  ImageMitchellFilt (const ValTable &params)
    : ImageFilter (params, default_radius()),
      b (params.get_float ("b", default_b())),
      c (params.get_float ("c", default_c()))
  { }

  virtual float val (float x, float y) const
  {
    return mitchell1 (x, inv_x_radius) * mitchell1 (y, inv_y_radius);
  }

  float mitchell1 (float x, float inv_radius) const
  {
    x = abs (2.f * x * inv_radius);

    // This formula is from the PBRT book
    //
    if (x > 2.f)
      return 0;
    else if (x > 1.f)
      return ((-b - 6*c) * x*x*x + (6*b + 30*c) * x*x +
	      (-12*b - 48*c) * x + (8*b + 24*c)) * (1.f/6.f);
    else
      return ((12 - 9*b - 6*c) * x*x*x +
	      (-18 + 12*b + 6*c) * x*x +
	      (6 - 2*b)) * (1.f/6.f);
  }

  float b, c;
};


}

#endif // SNOGRAY_IMAGE_MITCHELL_FILT_H


// arch-tag: 7fd2ef94-7377-449a-a24a-dfbea8c016bc
