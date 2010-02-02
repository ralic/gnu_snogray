// plastic.h -- Plastic (thin, transmissive, reflective) material
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "intersect.h"
#include "media.h"
#include "fresnel.h"
#include "bsdf.h"

#include "plastic.h"

using namespace snogray;


// Common information used for refraction methods.
//
class PlasticBsdf : public Bsdf
{
public:

  PlasticBsdf (const Plastic &_plastic, const Intersect &isec)
    : Bsdf (isec), plastic (_plastic)
  { }

  // Return a sample of this BSDF, based on the parameter PARAM.
  //
  virtual Sample sample (const UV &param, unsigned flags) const
  {
    if (flags & SPECULAR)
      {
	// Clear all but the direction flags.  This means it will be
	// either REFLECTIVE, TRANSMISSIVE, or REFLECTIVE|TRANSMISSIVE.
	//
	flags &= SAMPLE_DIR;

	// Calculate fresnel surface reflection at the ray angle
	//
	float cos_xmit_angle = isec.cos_n (isec.v);
	float medium_ior = isec.media.medium.ior;
	float refl
	  = Fresnel (medium_ior, plastic.ior).reflectance (cos_xmit_angle);

	// Render transmitted light (some light is lost due to fresnel
	// reflection from the back surface).
	//
	Color xmit = plastic.color * (1 - refl);

	// If we're only allowed to choose a single direction, always
	// return that, otherwise choose between them based on their
	// relative strengths.
	//
	if (flags == TRANSMISSIVE || param.u < (xmit / (xmit + refl)))
	  // Transmitted sample.
	  return Sample (xmit, 1, -isec.v, SPECULAR|TRANSMISSIVE);
	else
	  // Reflected sample.
	  return Sample (refl, 1, isec.v.mirror (Vec (0, 0, 1)),
			 SPECULAR|REFLECTIVE);
      }

    return Sample ();
  }

  // Evaluate this BSDF in direction DIR, and return its value and pdf.
  //
  virtual Value eval (const Vec &) const
  {
    return Value ();		// we're specular, so all samples fail
  }

private:

  const Plastic &plastic;
};


// Return a new BSDF object for this material instantiated at ISEC.
//
Bsdf *
Plastic::get_bsdf (const Intersect &isec) const
{
  return new (isec) PlasticBsdf (*this, isec);
}


// arch-tag: cd843fe9-2c15-4212-80d7-7e302850c1a7
