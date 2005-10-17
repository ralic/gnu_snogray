// scene.h -- Scene description datatype
//
//  Copyright (C) 2005  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __SCENE_H__
#define __SCENE_H__

#include <fstream>
#include <list>
#include <vector>

#include "obj.h"
#include "light.h"
#include "intersect.h"
#include "material.h"
#include "voxtree.h"
#include "trace-state.h"
#include "camera.h"

namespace Snogray {

class Scene
{
public:

  typedef std::vector<Light *>::const_iterator light_iterator_t;
  typedef std::list<Obj *>::const_iterator obj_iterator_t;
  typedef std::list<const Material *>::const_iterator material_iterator_t;

  static const unsigned DEFAULT_MAX_DEPTH = 5;
  static const unsigned DEFAULT_HORIZON = 10000;
  static const int DEFAULT_ASSUMED_GAMMA = 1;

  Scene ()
    : max_depth (DEFAULT_MAX_DEPTH), assumed_gamma (DEFAULT_ASSUMED_GAMMA)
  { }
  ~Scene ();

  // Calculate the color perceived by looking along RAY.  This is the
  // basic ray-tracing method.
  //
  Color render (const Ray &ray, TraceState &tstate, const Obj *origin = 0)
    const
  {
    if (tstate.depth > max_depth)
      return background;

    Ray intersected_ray (ray, DEFAULT_HORIZON);

    const Obj *closest = intersect (intersected_ray, tstate, origin);

    if (closest)
      {
	Intersect isec (intersected_ray, closest);

	// Calculate the appearance of the point on the object we hit
	//
	Color result = closest->material()->render (isec, tstate);

	// If we are looking through something other than air, attentuate
	// the surface appearance due to transmission through the current
	// medium.
	//
	if (tstate.medium)
	  result = tstate.medium->attenuate (result, intersected_ray.len);

	return result;
      }
    else
      return background;
  }

  // Return the closest object in this scene which intersects the
  // bounded-ray RAY, or zero if there is none.  RAY's length is shortened
  // to reflect the point of intersection.  If ORIGIN is non-zero, then the
  // _first_ intersection with that object is ignored (meaning that ORIGIN
  // is totally ignored if it is flat).
  //
  const Obj *intersect (Ray &ray, TraceState &tstate, const Obj *origin) const;

  bool shadowed (Light &light, const Ray &light_ray,
		 TraceState &tstate, const Obj *origin = 0)
    const;
  
  // Add various items to a scene.  All of the following "give" the
  // object to the scene -- freeing the scene will free them too.

  // Add an object
  //
  Obj *add (Obj *obj)
  {
    objs.push_back (obj);
    obj->add_to_space (obj_voxtree);
    return obj;
  }

  // Add a light
  Light *add (Light *light)
  {
    light->num = num_lights();	// Give LIGHT an index
    lights.push_back (light);
    return light;
  }

  // Add a material (we actually do nothing with these...)
  //
  const Material *add (const Material *mat)
  {
    materials.push_back (mat); return mat;
  }

  // Scene input
  //
  void load (const char *scene_file_name, const char *fmt, Camera &camera);
  void load (std::istream &stream, const char *fmt, Camera &camera);

  // Specific scene file formats
  //
  void load_aff_file (std::istream &stream, Camera &camera);

  unsigned num_lights () { return lights.size (); }

  void set_background (const Color &col) { background = col; }

  void set_assumed_gamma (float g) { assumed_gamma = g; }

  mutable struct Stats {
    Stats () : scene_closest_intersect_calls (0),
	       obj_closest_intersect_calls (0),
	       scene_shadowed_tests (0),
	       shadow_hint_hits (0), shadow_hint_misses (0),
	       horizon_hint_hits (0), horizon_hint_misses (0),
	       obj_intersects_tests (0)
    { }
    unsigned long long scene_closest_intersect_calls;
    unsigned long long obj_closest_intersect_calls;
    unsigned long long scene_shadowed_tests;
    unsigned long long shadow_hint_hits;
    unsigned long long shadow_hint_misses;
    unsigned long long horizon_hint_hits;
    unsigned long long horizon_hint_misses;
    unsigned long long obj_intersects_tests;
    Voxtree::Stats voxtree_closest_intersect;
    Voxtree::Stats voxtree_shadowed;
  } stats;

  std::list<Obj *> objs;

  std::vector<Light *> lights;

  std::list<const Material *> materials;

  // Background color
  Color background;

  Voxtree obj_voxtree;

  unsigned max_depth;

  float assumed_gamma;
};

// These belong in "trace-state.h", but are here to avoid circular
// dependency problems.
//
inline Color
TraceState::render (const Ray &ray, const Obj *origin)
{
  return scene.render (ray, *this, origin);
}

}

#endif /* __SCENE_H__ */

// arch-tag: 113d6236-471b-4184-92f5-9a03cf3a5221
