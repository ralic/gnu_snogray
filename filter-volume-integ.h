// filter-volume-integ.h -- Volume integrator which only handles attenuation
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

#ifndef __FILTER_VOLUME_INTEG_H__
#define __FILTER_VOLUME_INTEG_H__

#include "volume-integ.h"


namespace snogray {

class Intersect;


// FilterVolumeInteg is a volume integrator which only handles simple
// attentuation by a constant medium (and not, for instance, scattering).
//
class FilterVolumeInteg : public VolumeInteg
{
public:

  // Global state for this integrator, for rendering an entire scene.
  //
  class GlobalState : public VolumeInteg::GlobalState
  {
  public:

    GlobalState (const Scene &_scene) : VolumeInteg::GlobalState (_scene) { }

    // Return a new volume integrator, allocated in context.
    //
    virtual VolumeInteg *make_integrator (RenderContext &context)
    {
      return new FilterVolumeInteg (context);
    }
  };

  // Return light generated by the volume between the endpoint of RAY and
  // its origin and arriving at its origin.  MEDIUM is the medium which the
  // ray travels through.  SAMPLE_NUM is the sample to use.
  //
  // "Li" means "Light incoming" (to ray).
  //
  virtual Tint Li (const Ray &, const Medium &, const SampleSet::Sample &) const
  {
    return 0;
  }

  // Return the amount by which light travelling from the endpoint of RAY
  // to its origin is attenuated by the volume intervening volume.  MEDIUM
  // is the medium which the ray travels through.
  //
  virtual Color transmittance (const Ray &ray, const Medium &medium) const
  {
    return medium.transmittance (ray.length ());
  }

protected:

  FilterVolumeInteg (RenderContext &_context) : VolumeInteg (_context) { }
};


}

#endif // __VOLUME_INTEG_H__
