// uv.h -- UV datatype, for holding 2d texture coordinates
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#ifndef __UV_H__
#define __UV_H__

#include "tex2.h"


namespace Snogray {


struct UV
{
  UV (tparam_t _u, tparam_t _v) : u (_u), v (_v) { }
  tparam_t u, v;
};


}

#endif // __UV_H__


// arch-tag: b1f1e25c-0a80-4db0-a0e4-a4fcf52bfa6e
