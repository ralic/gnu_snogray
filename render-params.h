// render-params.h -- Rendering parameters
//
//  Copyright (C) 2005, 2006, 2007, 2008, 2010  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __RENDER_PARAMS_H__
#define __RENDER_PARAMS_H__

#include "coords.h"
#include "val-table.h"

namespace snogray {

class RenderParams
{
public:

  // Number of BSDF samples to take, per intersection.
  //
  static const unsigned DEFAULT_BSDF_SAMPLES = 16;

  // Maximum BSDF samples per eye-ray (this number is approximate), as a
  // multiple of the number of (per-intersection) BSDF samples.
  //
  static const unsigned DEFAULT_MAX_BSDF_SAMPLE_MULT = 4;

  // Number of light samples.
  //
  static const unsigned DEFAULT_LIGHT_SAMPLES = 16;

  // Maximum light samples per eye-ray (this number is approximate), as a
  // multiple of the number of (per-intersection) light samples.
  //
  static const unsigned DEFAULT_MAX_LIGHT_SAMPLE_MULT = 4;

  // The minimum ray-length that will be accepted when tracing a ray; any
  // intersection closer than this to the ray origin will be ignored.
  //
  // If we're using single-precision coordinates, we have to allow a lot of
  // margin for imprecision to avoid self-intersection problems.  We do
  // this by bumping up the minimum media distance to something fairly large.
  //
#if USE_FLOAT_COORDS
  static const dist_t DEFAULT_MIN_TRACE = 1e-3;
#else
  static const dist_t DEFAULT_MIN_TRACE = 1e-10;
#endif

  static const float DEFAULT_ENVLIGHT_INTENS_FRAC = 0.5;

  RenderParams ()
    : num_bsdf_samples (DEFAULT_BSDF_SAMPLES),
      max_bsdf_samples (num_bsdf_samples * DEFAULT_MAX_BSDF_SAMPLE_MULT),
      num_light_samples (DEFAULT_LIGHT_SAMPLES),
      max_light_samples (num_light_samples * DEFAULT_MAX_LIGHT_SAMPLE_MULT),
      min_trace (DEFAULT_MIN_TRACE),
      envlight_intens_frac (DEFAULT_ENVLIGHT_INTENS_FRAC)
  { }
  RenderParams (const ValTable &params)
    : num_bsdf_samples (
	params.get_uint ("bsdf-samples", DEFAULT_BSDF_SAMPLES)),
      max_bsdf_samples (
	params.get_uint ("max-bsdf-samples",
			 num_bsdf_samples * DEFAULT_MAX_BSDF_SAMPLE_MULT)),
      num_light_samples (
	params.get_uint ("light-samples", DEFAULT_LIGHT_SAMPLES)),
      max_light_samples (
	params.get_uint ("max-light-samples",
			 num_light_samples * DEFAULT_MAX_LIGHT_SAMPLE_MULT)),
      min_trace (
	params.get_float ("min-media", DEFAULT_MIN_TRACE)),
      envlight_intens_frac (
	params.get_float ("envlight-intens-frac", DEFAULT_ENVLIGHT_INTENS_FRAC))
  { }

  // Number of BSDF samples to take, per intersection.
  //
  unsigned num_bsdf_samples;

  // Maximum BSDF samples per eye-ray (this number is approximate).
  //
  unsigned max_bsdf_samples;

  // Number of light samples.
  //
  unsigned num_light_samples;

  // Maximum light samples per eye-ray (this number is approximate).
  //
  unsigned max_light_samples;

  // Minimum length of a mediad ray; any objects closer than this to the
  // ray origin are ignored.  This doesn't apply to ordinary (opaque)
  // shadow rays, just recursive medias such as used by reflection or
  // refraction, and non-opaque shadow rays.  As other mechanisms avoid
  // hitting the surface of origin when tracing such rays, min_trace
  // really only helps if the model has multiple surfaces precisely
  // located on top of each other (which does does happen in practice,
  // though usually in weird cases like models with multiple "optional"
  // parts which are all enabled by default).
  //
  dist_t min_trace;

  float envlight_intens_frac;
};

}

#endif /* __RENDER_PARAMS_H__ */

// arch-tag: eeaa2e45-8ad3-4174-bac9-6814468841ee
