// bbox.cc -- Axis-aligned bounding boxes
//
//  Copyright (C) 2007  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include "xform.h"

#include "bbox.h"


using namespace snogray;


// Transform this bounding-box by XFORM, ensuring that the result is
// still axis-aligned.
//
BBox
BBox::operator* (const XformBase<dist_t> &xform) const
{
  BBox xformed_bbox (min * xform);

  xformed_bbox += Pos (min.x, min.y, max.z) * xform;
  xformed_bbox += Pos (min.x, max.y, min.z) * xform;
  xformed_bbox += Pos (min.x, max.y, max.z) * xform;
  xformed_bbox += Pos (max.x, min.y, min.z) * xform;
  xformed_bbox += Pos (max.x, min.y, max.z) * xform;
  xformed_bbox += Pos (max.x, max.y, min.z) * xform;

  xformed_bbox += max * xform;

  return xformed_bbox;
}


std::ostream&
snogray::operator<< (std::ostream &os, const BBox &bbox)
{
  os << "bbox<"
     << std::setprecision (5) << lim (bbox.min.x) << ", "
     << std::setprecision (5) << lim (bbox.min.y) << ", "
     << std::setprecision (5) << lim (bbox.min.z) << " - "
     << std::setprecision (5) << lim (bbox.max.x) << ", "
     << std::setprecision (5) << lim (bbox.max.y) << ", "
     << std::setprecision (5) << lim (bbox.max.z)
     << ">";
  return os;
}


// arch-tag: fa0045f7-1646-422c-86ec-6375b51ae950