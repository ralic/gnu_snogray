// grid-iter.h -- Iterator for stratified grid sampling
//
//  Copyright (C) 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __GRID_ITER_H__
#define __GRID_ITER_H__


namespace Snogray {


// Iterator for stratified grid sampling
//
class GridIter
{
 public:

  GridIter (unsigned num)
    : num_steps (num ? unsigned (ceil (sqrt (num))) : 0),
      step (num ? 1.f / num_steps : 0),
      u_offs (0), v_offs (0), u_left (num_steps), v_left (num_steps)
  { }

  bool next (float &u, float &v)
  {
    if (! v_left)
      return false;

    u = u_offs + random (step);
    v = v_offs + random (step);

    if (--u_left)
      u_offs += step;
    else
      {
	u_offs = 0;
	u_left = num_steps;

	v_offs += step;
	--v_left;
      }

    return true;
  }

  unsigned num_samples () const { return num_steps * num_steps; }

private:

  unsigned num_steps;

  // Iterance between grid points in u and v directions.
  //
  float step;

  float u_offs, v_offs;

  unsigned u_left, v_left;
};


}


#endif /* __GRID_ITER_H__ */

// arch-tag: fcd8480e-e592-45f9-a8cf-91687c2e92b7