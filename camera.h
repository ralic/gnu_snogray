// camera.h -- Camera datatype
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __CAMERA_H__
#define __CAMERA_H__

#include "snogmath.h"
#include "rand.h"
#include "coords.h"
#include "ray.h"
#include "xform.h"

namespace Snogray {

class Camera {
public:

  class Format {
  public:

    Format (float width, float height)
      : film_width (width), film_height (height)
    { }

    // Return the horizontal field-of-view of a lens with the given focal length
    //
    float horiz_fov (float focal_length)
    {
      return atan2 (film_width / 2, focal_length) * 2;
    }

    // Return the vertical field-of-view of a lens with the given focal length
    //
    float vertical_fov (float focal_length)
    {
      return atan2 (film_height / 2, focal_length) * 2;
    }

    // Size of film
    //
    float film_width, film_height;
  };

  // Whether the (camera-relative) Z-axis increases when we move forward
  // (into the image), or decreases.  Our native mode is "increases forward",
  // but imported scenes may use a different convention.
  //
  enum z_mode {
    Z_INCREASES_FORWARD,
    Z_DECREASES_FORWARD
  };

  // Various pre-defined camera formats
  //
  static const Format FMT_35mm, FMT_6x6, FMT_6x7;
  static const Format FMT_APS_C, FMT_APS_H, FMT_APS_P; // who cares, but ...
  static const Format FMT_4x3, FMT_5x4, FMT_16x9; // ersatz formats for video

  static const float DEFAULT_SCENE_UNIT = 25.4; // 1 scene unit in camera units

  Camera (const Format &fmt = FMT_35mm, float _scene_unit = DEFAULT_SCENE_UNIT,
	  float focal_length = 0 /* 0==auto */);


  // Move the camera to absolution position POS
  //
  void move (const Pos &_pos) { pos = _pos; }
  void move (const Vec &offs) { pos += offs; }

  // Point at TARG, using the old "up" direction if not specified.
  //
  void point (const Pos &targ, const Vec &_user_up)
  {
    point (targ - pos, _user_up);
  }
  void point (const Pos &targ)
  {
    point (targ, user_up);
  }

  // Point at the target with offset VEC from the camera location, using
  // the old "up" direction if not specified.
  //
  void point (const Vec &vec, const Vec &_user_up)
  {
    target_dist = vec.length ();
    _point (vec.unit(), _user_up);
  }
  void point (const Vec &vec)
  {
    point (vec, user_up);
  }

  // Point the camera in direction DIR.  DIR is assumed to be a unit
  // vector; the target distance is left unchanged.
  //
  void _point (const Vec &dir, const Vec &_user_up)
  {
    user_up = _user_up;

    forward = dir.unit ();

    right = cross (_user_up, forward).unit ();
    up = cross (forward, right).unit ();

    if (z_mode == Z_DECREASES_FORWARD)
      right = -right;
  }

  // Change the current camera direction according to the rotational
  // transform ROT_XFORM (ROT_XFORM is assume to be a pure rotational
  // transform -- no scaling, no translation).
  //
  void rotate (const Xform &rot_xform);

  // Apply XFORM with the target at the origin, then move target back to
  // original location.
  //
  void orbit (const Xform &xform);

  // This moves the camera such that if the rest of the scene is
  // transformed with the same matrix XFORM, the apparent view will not
  // change.
  //
  void transform (const Xform &xform);


  // Set the mapping of "scene units" to camera units (nominally mm).
  // This is only used for depth-of-field simulation.  The default value is
  // 25.4mm, or 1 inch
  //
  void set_scene_unit (float camera_units)
  {
    scene_unit = camera_units;
  }

  // Set the distance to the focus plane, in scene units.  This defaults to
  // the "target distance" (the distance between the camera position and
  // the position passed to the `point' method).
  //
  void set_focus (float distance)
  {
    focus = distance;
  }

  // Similarly, but to a point in the scene, or along a scene vector.
  //
  void set_focus (const Pos &point_on_focus_plane)
  {
    set_focus (point_on_focus_plane - pos);
  }
  void set_focus (const Vec &vec)
  {
    // The distance to the focus plane is parallel to the camera forward
    // vector.
    //
    Vec components = vec.to_basis (right, up, forward);
    set_focus (components.z);
  }

  // Return / set the focal length in camera units (nominally mm).
  //
  float focal_length () const
  {
    return format.film_width / 2 / tan_half_fov_x;
  }
  void set_focal_length (float focal_len)
  {
    tan_half_fov_x = format.film_width / 2 / focal_len;
    tan_half_fov_y = format.film_height / 2 / focal_len;
  }

  // Set the camera aperture for depth-of-field simulation, in f-stops
  //
  void set_f_stop (float f_stop) { aperture = focal_length () / f_stop; }

  void zoom (float magnification)
  {
    tan_half_fov_x /= magnification;
    tan_half_fov_y /= magnification;
  }
  void set_horiz_fov (float fov)
  {
    tan_half_fov_x = tan (fov / 2);
    tan_half_fov_y = format.film_height / (format.film_width / tan_half_fov_x);
  }
  void set_vert_fov (float fov)
  {
    tan_half_fov_y = tan (fov / 2);
    tan_half_fov_x = format.film_width / (format.film_height / tan_half_fov_y);
  }

  float aspect_ratio () const
  {
    return format.film_width / format.film_height;
  }
  void set_aspect_ratio (float aspect_ratio)
  {
    float focal_len = focal_length ();

    float old_aspect_ratio = format.film_width / format.film_height;
    float aspect_ratio_change = aspect_ratio /  old_aspect_ratio;

    // Expand whichever film dimension necessary to fit the new aspect ratio.
    //
    if (aspect_ratio_change >= 1)
      format.film_width *= aspect_ratio_change;
    else
      format.film_height /= aspect_ratio_change;

    set_focal_length (focal_len); // update tan_half_fov_* variables
  }

  void set_format (const Format &fmt)
  {
    float focal_len = focal_length ();

    format = fmt;

    set_focal_length (focal_len); // update tan_half_fov_* variables
  }

  // Return an eye-ray from this camera for position U,V on the film plane,
  // with no depth-of-field.  U and V have a range of 0-1.
  //
  Ray get_ray (float u, float v) const
  {
    Pos targ = pos;

    targ += forward;
    targ += 2 * (u - 0.5) * right * tan_half_fov_x;
    targ += 2 * (v - 0.5) * up * tan_half_fov_y;

    return Ray (pos, targ);
  }

  // Return an eye-ray from this camera for position (U, V) on the film
  // plane, with the random perturbation (FOCUS_U, FOCUS_V) for
  // depth-of-field simulation no depth-of-field.  All paramters have a
  // range of 0-1.
  //
  Ray get_ray (float u, float v, float focus_u, float focus_v) const
  {
    // The source of the camera ray, which is the camera position
    // (actually the optical center of the lens), possibly perturbed for
    // depth-of-field simulation
    //
    Pos src = pos;

    // The point on the virtual film plane (one unit in front of the camera
    // position, projected from the actual film plane which lies behind the
    // camera position) which is the end of the camera ray.
    //
    Pos targ = pos;

    // Adjust TARG to its final poitn on the virtual film plane.
    //
    targ += forward;
    targ += 2 * (u - 0.5) * right * tan_half_fov_x;
    targ += 2 * (v - 0.5) * up * tan_half_fov_y;

    if (aperture != 0)
      {
	// The camera aperture, converted to scene units.
	//
	float adj_aperture = aperture / scene_unit;

	// How much we will randomly perturb the camera position to simulate
	// depth-of-field.
	//
	float src_perturb_u = adj_aperture * (focus_u - 0.5);
	float src_perturb_v = adj_aperture * (focus_v - 0.5);

	// The distance to the focus plane, in scene units.
	//
	float focus_distance = (focus == 0) ? target_dist : focus;

	// Similarly, much we will randomly perturb the corresponding point on
	// the "virtual film plane" 1 unit in front of camera position to
	// simulate depth-of-field.  This is simply the above perturbation
	// scaled by (1 - 1 / FOCUS_DISTANCE).
	//
	float targ_perturb_scale = 1 - 1 / focus_distance;
	float targ_perturb_u = src_perturb_u * targ_perturb_scale;
	float targ_perturb_v = src_perturb_v * targ_perturb_scale;

	// Perturb the camera position.
	//
	src += right * src_perturb_u + up * src_perturb_v;

	// Perturb the point on the virtual film plane.
	//
	targ += right * targ_perturb_u + up * targ_perturb_v;
      }

    return Ray (src, targ);
  }

  // Set whether the Z axis increases into the image or decreases
  //
  void set_z_mode (enum z_mode _z_mode)
  {
    if (_z_mode != z_mode)
      {
	right = -right;
	z_mode = _z_mode;
      }
  }

  Format format;

  Pos pos;
  Vec user_up;

  Vec forward, up, right;

  // How far it is to the "target".
  //
  dist_t target_dist;

  // Lens aperture.  This only affects depth-of-field, not exposure like in
  // a real camera.  Zero means perfect focus.
  //
  float aperture;

  // The distance to the focus plane, from POS.  If zero, TARGET_DIST is used.
  //
  float focus;

  // The length of one "scene unit", in "camera units" (the same units we
  // use for focal-length, aperture etc, nominally mm).
  //
  float scene_unit;

  // How the Z axis behaves with respect to the camera
  //
  enum z_mode z_mode;

  float tan_half_fov_x, tan_half_fov_y;
};

}

#endif /* __CAMERA_H__ */

// arch-tag: 45c04676-91b9-4fdb-ae65-2fd3ea0f0228
