# lua-cvt-num-vector.swg -- Lua table -> std::vector conversion
#
#  Copyright (C) 2011-2013  Miles Bader <miles@gnu.org>
#
# This source code is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or (at
# your option) any later version.  See the file COPYING for more details.
#
# Written by Miles Bader <miles@gnu.org>
#

// For handling variable-length array arguments, passed as std::vectors.
//
%{
#include <vector>
#include "lua-util/lua-vector.h"

// If the object at position INDEX on the Lua stack is a table
// containing numbers, store its contents into VEC and return true.
// Lua numbers will be cast into the type T.  If the object at INDEX
// is not a table, or the table contains non-numbers, false is
// returned instead.
//
template<class T>
bool
SWIG_read_numeric_vector (lua_State *L, int index, std::vector<T> &vec)
{
  if (! lua_istable (L, index))
    return false;			// error

#if LUA_VERSION_NUM >= 502
  size_t len = lua_rawlen (L, index);
#else
  size_t len = lua_objlen (L, index);
#endif
  vec.reserve (len);

  for (size_t i = 0; i < len; i++)
    {
      lua_rawgeti (L, index, i + 1);
      if (! lua_isnumber (L, -1))
	{
	  lua_pop (L, 1);
	  return false;
	}
      vec.push_back (T (lua_tonumber (L, -1)));
      lua_pop (L, 1);
    }

  return true;
}

// Return a pointer to a std::vector<T> object containing the contents
// of the table/vector at position POS on the Lua stack.  Tables will
// be converted by using SWIG_read_numeric_vector to store them into
// TEMP_STORAGE; C++ vectors wrapped using lua-vector.h will be
// returned directly (leaving TEMP_STORAGE unchanged).  If the object
// at POS is not a table or vector wrapper, or the table contains
// non-numbers, zero is returned instead.
//
template<class T>
std::vector<T> *
SWIG_get_numeric_vector (lua_State *L, int pos, std::vector<T> &temp_storage)
{
  int type = lua_type (L, pos);
  if (type == LUA_TUSERDATA)
    return snogray::LuaVec<T>::vec (L, pos);
  else if (type == LUA_TTABLE && SWIG_read_numeric_vector (L, pos, temp_storage))
    return &temp_storage;
  return 0;
}

%}

%define SWIG_TYPEMAP_NUM_VECTOR(T)
%typemap(in) const std::vector<T> &INPUT (std::vector<T> temp)
  %{	$1 = SWIG_get_numeric_vector<T> (L,$input, temp);
        if (! $1) SWIG_fail; %}
%enddef
