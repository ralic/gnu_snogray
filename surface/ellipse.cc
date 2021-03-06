// ellipse.cc -- Ellipse surface
//
//  Copyright (C) 2007-2013  Miles Bader <miles@gnu.org>
//
// This source code is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 3, or (at
// your option) any later version.  See the file COPYING for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "geometry/disk-sample.h"
#include "render/intersect.h"
#include "surface-sampler.h"

#include "ellipse.h"


using namespace snogray;



// Ellipse::IsecInfo

class Ellipse::IsecInfo : public Surface::Renderable::IsecInfo
{
public:

  IsecInfo (const Ray &ray, const Ellipse &_ellipse, const UV &_uv)
    : Surface::Renderable::IsecInfo (ray), ellipse (_ellipse), uv (_uv)
  { }

  virtual Intersect make_intersect (const Media &media,
				    RenderContext &context)
    const;
  virtual Vec normal () const;

private:

  const Ellipse &ellipse;

  UV uv;
};

// Create an Intersect object for this intersection.
//
Intersect
Ellipse::IsecInfo::make_intersect (const Media &media, RenderContext &context)
  const
{
  const Pos &point = ray.end ();

  // The ellipse's two "radii".
  //
  Vec rad1 = ellipse.edge1 / 2;
  Vec rad2 = ellipse.edge2 / 2;
  dist_t inv_rad1_len = 1 / rad1.length ();
  dist_t inv_rad2_len = 1 / rad2.length ();

  // Tangent vectors.
  //
  Vec s = rad1 * inv_rad1_len;
  Vec t = cross (s, ellipse.normal);

  // Normal frame.
  //
  Frame norm_frame = Frame (point, s, t, ellipse.normal);

  // Calculate partial derivatives of texture coordinates dTds and dTdt,
  // where T is the texture coordinates (for bump mapping).
  //
  UV dTds = UV (0.5f * float (inv_rad1_len), 0);
  UV dTdt = UV (0, 0.5f * float (inv_rad2_len));

  return Intersect (ray, media, context, *ellipse.material,
		    norm_frame, uv, dTds, dTdt);
}

// Return the normal of this intersection (in the world frame).
//
Vec
Ellipse::IsecInfo::normal () const
{
  return ellipse.normal;
}



// intersection

// If this surface intersects RAY, change RAY's maximum bound
// (Ray::t1) to reflect the point of intersection, and return a
// Surface::Renderable::IsecInfo object describing the intersection
// (which should be allocated using placement-new with CONTEXT);
// otherwise return zero.
//
const Surface::Renderable::IsecInfo *
Ellipse::intersect (Ray &ray, RenderContext &context) const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    {
      ray.t1 = t;
      return new (context) IsecInfo (ray, *this, UV (u,v));
    }
  return 0;
}

// Return true if this surface intersects RAY.
//
bool
Ellipse::intersects (const Ray &ray, RenderContext &) const
{
  dist_t t, u, v;
  return intersects (ray, t, u, v);
}

// Return true if this surface completely occludes RAY.  If it does
// not completely occlude RAY, then return false, and multiply
// TOTAL_TRANSMITTANCE by the transmittance of the surface in medium
// MEDIUM.
//
// Note that this method does not try to handle non-trivial forms of
// transparency/translucency (for instance, a "glass" material is
// probably considered opaque because it changes light direction as
// well as transmitting it).
//
bool
Ellipse::occludes (const Ray &ray, const Medium &medium,
		   Color &total_transmittance, RenderContext &)
  const
{
  dist_t t, u, v;
  if (intersects (ray, t, u, v))
    {
      // Avoid unnecessary calculation if possible.
      if (material->fully_occluding ())
	return true;

      IsecInfo isec_info (Ray (ray, t), *this, UV (u, v));
      if (material->occlusion_requires_tex_coords ())
	return material->occludes (isec_info, TexCoords (ray (t), UV (u, v)),
				   medium, total_transmittance);
      else
	return material->occludes (isec_info, medium, total_transmittance);
    }
  return false;
}



// misc Ellipse methods

// Return a bounding box for this surface.
//
BBox
Ellipse::bbox () const
{
  // This could be a bit more tight...
  //
  BBox bbox (corner);
  bbox += corner + edge1;
  bbox += corner + edge2;
  bbox += corner + edge1 + edge2;
  return bbox;
}

// Transform the geometry of this surface by XFORM.
//
void
Ellipse::transform (const Xform &xform)
{
  corner.transform (xform);
  edge1.transform (xform);
  edge2.transform (xform);
}



// Ellipse::Sampler

// Ellipse Sampler interface.
//
class Ellipse::Sampler : public Surface::Sampler
{
public:

  Sampler (const Ellipse &_ellipse)
    : ellipse (_ellipse),
      pdf (4 * INV_PIf
	   / float (cross (ellipse.edge2, ellipse.edge1).length ()))
  { }

  // Return a sample of this surface.
  //
  virtual AreaSample sample (const UV &param) const;

  // If a ray from VIEWPOINT in direction DIR intersects this
  // surface, return an AngularSample as if the
  // Surface::Sampler::sample_from_viewpoint method had returned a
  // sample at the intersection position.  Otherwise, return an
  // AngularSample with a PDF of zero.
  //
  virtual AngularSample eval_from_viewpoint (const Pos &viewpoint,
					     const Vec &dir)
    const;

private:

  const Ellipse &ellipse;

  // Cache of PDF, which is just 1 / area.
  //
  float pdf;
};

// Return a sample of this surface.
//
Surface::Sampler::AreaSample
Ellipse::Sampler::sample (const UV &param) const
{
  dist_t dx, dy;
  disk_sample (dist_t (0.5), param, dx, dy);
  dx += dist_t (0.5);
  dy += dist_t (0.5);
  Pos pos = ellipse.corner + ellipse.edge1 * dx + ellipse.edge2 * dy;

  return AreaSample (pos, ellipse.normal, pdf);
}

// If a ray from VIEWPOINT in direction DIR intersects this
// surface, return an AngularSample as if the
// Surface::Sampler::sample_from_viewpoint method had returned a
// sample at the intersection position.  Otherwise, return an
// AngularSample with a PDF of zero.
//
Surface::Sampler::AngularSample
Ellipse::Sampler::eval_from_viewpoint (const Pos &viewpoint, const Vec &dir)
  const
{
  dist_t t;
  dist_t u, v;
  if (ellipse.intersects (viewpoint, dir, t, u, v))
    {
      Pos pos = viewpoint + t * dir;
      return AngularSample (AreaSample (pos, ellipse.normal, pdf), viewpoint);
    }
  return AngularSample ();
}


// Return a sampler for this surface, or zero if the surface doesn't
// support sampling.  The caller is responsible for destroying
// returned samplers.
//
Surface::Sampler *
Ellipse::make_sampler () const
{
  return new Sampler (*this);
}
