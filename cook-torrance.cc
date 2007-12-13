// cook-torrance.cc -- Cook-Torrance material
//
//  Copyright (C) 2005, 2006, 2007  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <list>

#include "snogmath.h"
#include "vec.h"
#include "trace.h"
#include "intersect.h"
#include "ward-dist.h"
#include "cos-dist.h"
#include "grid-iter.h"
#include "brdf.h"

#include "cook-torrance.h"


using namespace snogray;



// The details of cook-torrance evaluation are in this class.
//
class CookTorranceBrdf : public Brdf
{
public:

  CookTorranceBrdf (const CookTorrance &_ct, const Intersect &_isec)
    : Brdf (_isec), ct (_ct),
      spec_dist (ct.m), diff_dist (),
      diff_weight (ct.color.intensity ()),
      inv_diff_weight (diff_weight == 0 ? 0 : 1 / diff_weight),
      inv_spec_weight (diff_weight == 1 ? 0 : 1 / (1 - diff_weight)),
      fres (isec.trace.medium ? isec.trace.medium->ior : 1, ct.ior),
      nv (isec.cos_n (isec.v)), inv_pi_nv (nv == 0 ? 0 : INV_PIf / nv)
  { }

  // Generate around NUM samples of this BRDF and add them to SAMPLES.
  // Return the actual number of samples (NUM is only a suggestion).
  //
  virtual unsigned gen_samples (unsigned num, IllumSampleVec &samples) const
  {
    GridIter grid_iter (num);

    float u, v;
    while (grid_iter.next (u, v))
      gen_sample (u, v, samples);

    return grid_iter.num_samples ();
  }

  // Add reflectance information for this BRDF to samples from BEG_SAMPLE
  // to END_SAMPLE.
  //
  virtual void filter_samples (const IllumSampleVec::iterator &beg_sample,
			       const IllumSampleVec::iterator &end_sample)
    const
  {
    for (IllumSampleVec::iterator s = beg_sample; s != end_sample; s++)
      if (! s->invalid)
	filter_sample (s);
  }

private:

  // Calculate D (microfacet distribution) term.  Traditionally
  // Cook-torrance uses a Beckmann distribution for this, but we use the
  // Ward Isotropic distribution becuase it's easy to sample.
  //
  float D (float nh) const
  {
    return spec_dist.pdf (nh);
  }
  float D_pdf (float nh, float vh) const
  {
    // The division by 4 * VH here is intended to compensate for the fact
    // that the underlying distribution SPEC_DIST is actually that of the
    // half-vector H, whereas the pdf we want should be the distribution of
    // the light-vector L.  I don't really understand why it works, but
    // it's in the PBRT book, and seems to have good results.
    //
    return spec_dist.pdf (nh) / (4 * vh);
  }

  // Calculate F (fresnel) term
  //
  float F (float vh) const { return fres.reflectance (vh); }

  // Calculate G (microfacet masking/shadowing) term
  //
  //    G = min (1,
  //             2 * (N dot H) * (N dot V) / (V dot H),
  //             2 * (N dot H) * (N dot L) / (V dot H))
  //
  float G (float vh, float nh, float nl) const
  {
    return min (2 * nh * ((nv > nl) ? nl : nv) / vh, 1.f);
  }

  // Return the CT reflectance for the sample in direction L, where H is
  // the half-vector.  The pdf is returned in PDF.
  //
  Color val (const Vec &l, const Vec &h, float &pdf) const
  {
    float nh = isec.cos_n (h), nl = isec.cos_n (l);

    // Angle between view angle and half-way vector (also between
    // light-angle and half-way vector -- lh == vh).
    //
    float vh = isec.cos_v (h);

    // The Cook-Torrance specular term is:
    //
    //    f_s = (F / PI) * D * G / (N dot V)
    //
    // We sample the specular term using the D component only, so the pdf
    // is only based on that.
    //
    float spec = F (vh) * D (nh) * G (vh, nh, nl) * inv_pi_nv;
    float spec_pdf = D_pdf (nh, vh);

    // Diffuse term is a simple lambertian (cosine) distriution, and its
    // pdf is exact.
    //
    float diff = diff_dist.pdf (nl);
    float diff_pdf = diff;	// identical

    pdf = diff_pdf * diff_weight + spec_pdf * (1 - diff_weight);

    return ct.color * diff + ct.specular_color * spec;
  }

  void gen_sample (float u, float v, IllumSampleVec &samples) const
  {
    Vec l, h;
    if (u < diff_weight)
      {
	float scaled_u = u * inv_diff_weight;
	l = diff_dist.sample (scaled_u, v);
	h = (isec.v + l).unit ();
      }
    else
      {
	float scaled_u = (u - diff_weight) * inv_spec_weight;
	h = spec_dist.sample (scaled_u, v);
	if (isec.cos_v (h) < 0)
	  h = -h;
	l = isec.v.mirror (h);
      }

    if (isec.cos_n (l) > Eps)
      {
	float pdf;
	Color f = val (l, h, pdf);

	samples.push_back (IllumSample (l, f, pdf));
      }
  }

  void filter_sample (const IllumSampleVec::iterator &s) const
  {
    const Vec &l = s->dir;
    const Vec h = (isec.v + l).unit ();
    s->refl = val (l, h, s->brdf_pdf);
  }

  const CookTorrance &ct;

  // Sample distributions for specular and diffuse components.
  //
  const WardDist spec_dist;
  const CosDist diff_dist;

  // Weight used for sampling diffuse component (0 = don't sample
  // diffuse at all, 1 = only sample diffuse).  The "specular" component
  // has a weight of (1 - DIFF_WEIGHT).
  //
  float diff_weight;

  // 1 / DIFF_WEIGHT, and 1 / (1 - DIFF_WEIGHT).
  //
  float inv_diff_weight, inv_spec_weight;

  // Info for calculating the Fresnel term.
  //
  const Fresnel fres;

  // (N dot V) and 1 / (PI * N dot V).
  //
  const float nv, inv_pi_nv;
};


// Return a new BRDF object for this material instantiated at ISEC.
//
Brdf *
CookTorrance::get_brdf (const Intersect &isec) const
{
  return new (isec) CookTorranceBrdf (*this, isec);
}


// arch-tag: a0a0049e-9af6-4438-ab58-081630151122
