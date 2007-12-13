// illum-sample.h -- Directional sample used during illumination
//
//  Copyright (C) 2005, 2006, 2007  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __ILLUM_SAMPLE_H__
#define __ILLUM_SAMPLE_H__

#include <vector>

#include "vec.h"
#include "color.h"

namespace snogray {

class Light;

// A single directional sample.  The origin is implicit, because illums
// are typically taken from a single point, so only the direction is
// included.
//
class IllumSample
{
public:

  // Generated by a light (BRDF fields initially zero).
  //
  IllumSample (const Vec &_dir, const Color &_val, float _light_pdf,
	       dist_t _dist, const Light *_light)
    : dir (_dir), val (_val), refl (0),
      light_pdf (_light_pdf), brdf_pdf (0), dist (_dist),
      light (_light), specular (false), invalid (false)
  { }

  // Generated by a BRDF (light fields initially zero).
  //
  IllumSample (const Vec &_dir, const Color &_refl, float _brdf_pdf)
    : dir (_dir), val (0), refl (_refl),
      light_pdf (0), brdf_pdf (_brdf_pdf), dist (0),
      light (0), specular (false), invalid (false)
  { }

  // The sample direction (the origin is implicit), in the
  // surface-normal coordinate system (where the surface normal is
  // (0,0,1)).
  //
  Vec dir;

  // The amount of light from this sample.  Note that the value for a
  // single sample "represents" the entire power of the light; if multiple
  // samples are used, they are averaged later.
  //
  // This has different values at different times:
  //
  //   1) For light-generated samples before filtering through the BRDF,
  // 	  this is the intensity at the light's surface, not yet adjusted by
  // 	  the light's pdf.
  //
  //   2) For light-generated samples after filtering through the BRDF, or
  // 	  for BRDF-generated samples after filtering by the light,
  // 	  this is the emitted radiance (outgoing).
  //
  //   3) For BRDF-generated samples before filtering by the light, it
  //      is undefined.
  //
  Color val;

  // The reflectivity of the BRDF for this sample.  Only valid for
  // BRDF-generated samples or for light-generated samples after filtering
  // by the BRDF.
  //
  Color refl;

  // The value of the "probability density function" for this sample in the
  // light's sample distribution.  Only valid for light-generated samples
  // or for BRDF-generated samples after filtering by the light.
  //
  // As a special case, a value of (exactly) zero means that this sample
  // was generated by a point light, whose sample distribution is a delta
  // function.
  //
  float light_pdf;

  // The value of the "probability density function" for this sample in the
  // BRDF's sample distribution.  Only valid for BRDF-generated samples
  // or for light-generated samples after filtering by the BRDF.
  //
  // As a special case, a value of (exactly) zero means that this sample
  // was generated by a specular BRDF, whose sample distribution is a delta
  // function.
  //
  float brdf_pdf;

  // The distance to the light or surface which this ray strikes
  // (zero means "strikes nothing").
  //
  float dist;

  // The light which this sample hits, or zero.
  //
  const Light *light;

  // True if this sample was generated by a specular or "near specular"
  // BRDF.  We use full subtraces for such samples.
  //
  bool specular : 1;

  // True if this sample shouldn't be used (set if it's discovered to be
  // invalid after being generated).
  //
  bool invalid : 1;
};

// Vectors of IllumSamples
//
typedef std::vector<IllumSample> IllumSampleVec;


}

#endif /* __ILLUM_SAMPLE_H__ */

// arch-tag: e0f3f775-1fab-4835-8d9f-4cd8b9729731
