// lua-funs.h -- Functions for use with Lua
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

#ifndef __LUA_FUNS_H__
#define __LUA_FUNS_H__

#include "config.h"

extern "C"
{
#include "lua.h"
}


namespace snogray {


// Return a Lua string containing the entire contents of a file, or
// return false if that can't be done for some reason (it's expected
// that in that case, the caller will then attempt to do the same
// thing using standard lua functions, and determine the error
// itself).
//
// This is basically equivalent to io.open(filename,"r"):read"*a" but
// much more efficient and less likely to thrash the system to death.
//
extern int lua_read_file (lua_State *L);


};

#endif // __LUA_FUNS_H__