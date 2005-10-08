#ifndef __RAY_H__
#define __RAY_H__

#include <fstream>

#include "pos.h"
#include "vec.h"

// A ray is a vector with a position and a length; we include various other
// fields for handy test.
class Ray {
public:
  Ray (Pos _origin, Vec _extent)
    : origin (_origin), dir (_extent.unit ()), len (_extent.length ()),
      _end (_origin + _extent)
  {
  }
  Ray (Pos _origin, Pos _targ)
    : origin (_origin), dir ((_targ - _origin).unit ()),
      len ((_targ - _origin).length ()), _end (_targ)
  {
  }
  Ray (const Ray &ray)
    : origin (ray.origin), dir (ray.dir), len (ray.len), _end (ray._end)
  {
  }

  /* Returns an end point of the ray izzf it is extended to length LEN.  */
  Pos extension (float _len) const { return origin + dir * _len; }

  Pos end () const { return _end; }

  Pos origin;

  Vec dir;			// should always be a unit vector
  Space::dist_t len;

private:
  // This is a pre-computed copy of (ORIGIN + DIR * LEN)
  Pos _end;
};

extern std::ostream& operator<< (std::ostream &os, const Ray &ray);

#endif /* __RAY_H__ */

// arch-tag: e8ba773e-11bd-4fb2-83b6-ace5f2908aad
