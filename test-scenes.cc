// test-scenes.cc -- Test scenes for snogray ray tracer
//
//  Copyright (C) 2005, 2006  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <iostream>
#include <sstream>

#include "test-scenes.h"

#include "scene.h"
#include "camera.h"
#include "point-light.h"
#include "far-light.h"
#include "rect-light.h"
#include "sphere-light.h"
#include "sphere.h"
#include "tripar.h"
#include "lambert.h"
#include "phong.h"
#include "cook-torrance.h"
#include "glow.h"
#include "mirror.h"
#include "glass.h"
#include "mesh.h"
#include "tessel-param.h"
#include "string-funs.h"

using namespace Snogray;
using namespace std;


// Helper functions

const void
add_rect (Scene &scene, const Material *mat,
	  const Pos &corner_0, const Pos &mid_corner_0, const Pos &corner_1)
{
  scene.add (new Tripar (mat, mid_corner_0, corner_0 - mid_corner_0,
			 corner_1 - mid_corner_0, true));
}

const void
add_rect (Scene &scene, const Material *mat,
	  const Pos &corner, const Vec &side1, const Vec &side2)
{
  scene.add (new Tripar (mat, corner, side1, side2, true));
}

const void
add_cube (Scene &scene, const Material *mat,
	  const Pos &corner, const Vec &up, const Vec &right, const Vec &fwd)
{
  add_rect (scene, mat, corner, right, up);
  add_rect (scene, mat, corner, up, fwd);
  add_rect (scene, mat, corner, fwd, right);

  add_rect (scene, mat, corner + up, fwd, right);
  add_rect (scene, mat, corner + right, up, fwd);
  add_rect (scene, mat, corner + fwd, right, up);
}

const void
add_rect_bulb (Scene &scene,
	       const Pos &corner, const Vec &side1, const Vec &side2,
	       const Color &col = Color::white)
{
  const Material *bulb_mat = scene.add (new Glow (col));
  scene.add (new RectLight (corner, side1, side2, col));
  add_rect (scene, bulb_mat, corner, side1, side2);
}

static void
add_bulb (Scene &scene, const Pos &pos, float radius,
	  const Color &col = Color::white)
{
  dist_t area = 4 * M_PI * radius * radius;

  Color intens = col / area;

  scene.add (new SphereLight (pos, radius, intens));
  scene.add (new Sphere (scene.add (new Glow (intens)), pos, radius));
}



static void
add_chessboard (Scene &scene, const Xform &xform = Xform::identity,
		unsigned variant = 0)
{
  const Material *gloss_black
    = scene.add (new Mirror (1.5, 0.4, 0.02, cook_torrance (0.9, 0.2)));
  const Material *black
    = scene.add (new Material (0.02, cook_torrance (0.9, 1)));
  const Material *gloss_ivory
    = scene.add (new Mirror (1.5, 0.4, Color (1, 0.8, 0.5),
			     cook_torrance (0.2, 0.2)));
  const Material *ivory
    = scene.add (new Material (Color (1, 0.8, 0.5),
			       cook_torrance (0.2, 2)));

  const Material *brown
    = scene.add (new Material (Color (0.3, 0.2, 0.05),
			       cook_torrance (0.5, 3, 2)));

  Xform mesh_xform = Xform::x_rotation (-M_PI_2).scale (-1, 1, 1) * xform;

  const Material *b1_mat = variant == 0 ? gloss_black : black;
  const Material *b2_mat = variant == 0 ? gloss_ivory : ivory;

  scene.add (new Mesh (b1_mat, "board1.msh", mesh_xform));
  scene.add (new Mesh (b2_mat, "board2.msh", mesh_xform));
  scene.add (new Mesh (brown, "board3.msh", mesh_xform));
}



enum tobj_type { TOBJ_SPHERE, TOBJ_TORUS, TOBJ_SINC };

static Surface *
tobj (tobj_type type, const Material *mat, const Pos &pos, dist_t radius,
     dist_t max_err)
{
  switch (type)
    {
    case TOBJ_SPHERE:
      return new Mesh (mat, SphereTesselFun (pos + Vec (0, radius, 0),
					     radius, radius * 0.002),
			  Tessel::ConstMaxErr (max_err), true);
    case TOBJ_TORUS:
      return new Mesh (mat, TorusTesselFun (pos + Vec (0, radius / 3, 0),
					    radius, radius / 3,
					    radius * 0.002),
		       Tessel::ConstMaxErr (max_err), true);
    case TOBJ_SINC:
      return new Mesh (mat, SincTesselFun (pos + Vec (0, radius * 0.25, 0),
					   radius * 1.5),
		       Tessel::ConstMaxErr (max_err), true);
    default:
      throw runtime_error ("unknown tobj type");
    }
}



static void
def_scene_miles (unsigned num, const string &, Scene &scene, Camera &camera)
{
//  Material *mat1 = scene.add (new Lambert (Color (1, 0.5, 0.2)));
//  Material *mat2 = scene.add (new Lambert (Color (0.5, 0.5, 0)));
//  Material *mat3 = scene.add (new Lambert (Color (1, 0.5, 1)));
//  Material *mat4 = scene.add (new Lambert (Color (1, 0.5, 1)));
//   Material *mat3 = scene.add (Material::phong (Color (0.1, 0.1, 0.1),  400));
//   const Material *mat1
//     = scene.add (new Mirror (0.5, Color (1, 0.5, 0.2) * 0.5, 5));
  const Material *crystal = scene.add (new Glass (1.8));
  const Material *silver = scene.add (new Mirror (Ior (0.25, 3), 0.9, 0.05));
  const Material *mat1 = crystal, *mat2 = silver;
  const Material *mat3
    = scene.add (new Material (Color (0.8, 0, 0), phong (0.2, 400)));
  const Material *mat4
    = scene.add (new Material (Color (0.2, 0.5, 0.1)));

  // First test scene

  switch ((num / 10) % 10)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      add_bulb (scene, Pos (0, 15, 0), 0.06, 90);
      add_bulb (scene, Pos (0, 0, -5), 0.06, 90);
      add_bulb (scene, Pos (-5, 10, 0), 0.06, 120 * Color (0, 0, 1));
      add_bulb (scene, Pos (-40, 15, -40), 0.06, 3000);
      add_bulb (scene, Pos (-40, 15,  40), 0.06, 3000);
      add_bulb (scene, Pos ( 40, 15, -40), 0.06, 3000);
      add_bulb (scene, Pos ( 40, 15,  40), 0.06, 3000);
      break;

    case 2:
      add_rect_bulb (scene, Pos(-80, 0, -80), Vec(0, 80, 0), Vec(0, 0, 160),
		     10);
      break;

    case 3:
      add_rect_bulb (scene, Pos(-40, 0, 0), Vec(40, 0, 40), Vec(0, 5, 0), 15);
      add_rect_bulb (scene, Pos(0, 0, -40), Vec(40, 0, 40), Vec(0, 5, 0), 15);
      break;

    case 4:
      add_rect_bulb (scene, Pos(-20, -3, 0), Vec(20, 0, 20), Vec(0, 1, 0), 150);
      add_rect_bulb (scene, Pos(0, -3, -20), Vec(20, 0, 20), Vec(0, 1, 0), 150);
      break;
    }

  scene.add (new Sphere (mat1, Pos (0, 2, 7), 5));
  scene.add (new Sphere (mat2, Pos (-8, 0, 3), 3));
  scene.add (new Sphere (mat3, Pos (-6, 5, 2), 1));

  add_rect (scene, mat4, Pos (-100, -3, -100), Vec(200, 0, 0), Vec(0, 0, 200));

  switch (num % 10)
    {
    case 0:
    default:
      camera.move (Pos (-6.5, -0.4, -19));
      camera.point (Pos (0, -2, 5));
      break;

    case 2:			// overhead
      camera.move (Pos (0, 50, 30));
      camera.point (Pos (0, 5, 0));
      break;

    case 1:
      camera.move (Pos (-3, 2, -18));
      break;
    case 3:
      camera.move (Pos (-3, 1.5, -25));
      break;
    }

  
  const unsigned gsize = 10;
  const unsigned gsep = 4;
  const Pos gpos (-20, -1, -20);
  for (unsigned i = 0; i < gsize; i++)
    for (unsigned j = 0; j < gsize; j++)
      {
	Color color (0, float (j) / float (gsize), float (i) / float (gsize));
	Pos pos = gpos + Vec (i * gsep, 0, j * gsep);
	const Material *mat
	  = scene.add (new Material (color * 0.5, phong (0.5, 500)));
	scene.add (new Sphere (mat, pos, 0.5));
	scene.add (new Tripar (mat, pos + Vec (1.5, -0.2, 0),
			       Vec (-2, 0, -1.1), Vec (-2, 0, 1.1)));
      }
}

static void
add_scene_descs_miles (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("miles0", "Lots of spheres and triangles, low angle"));
  descs.push_back (TestSceneDesc ("miles1", "Lots of spheres and triangles, square angle"));
  descs.push_back (TestSceneDesc ("miles2", "Lots of spheres and triangles, high angle"));
  descs.push_back (TestSceneDesc ("miles3", "Lots of spheres and triangles, slightly wider angle"));
}

static void 
def_scene_pretty_bunny (unsigned num, const string &arg,
			Scene &scene, Camera &camera)
{
  // This is a mutation of test:cs465-4

  camera.move (Pos (-1, 1.35, 2.3)); // y=0.5
  camera.point (Pos (-0.75, 0.58, 0), Vec (0, 1, 0));
  camera.set_vert_fov (M_PI_4);
  camera.set_z_mode (Camera::Z_DECREASES_FORWARD);

  const Material *red
    = scene.add (new Material (Color (0.5, 0, 0),
			       cook_torrance (0.5, 0.1)));
  const Material *yellow
    = scene.add (new Material (Color (0.5, 0.5, 0),
			       cook_torrance (0.5, 0.1)));
  const Material *green
    = scene.add (new Material (Color (0, 0.5, 0),
			       cook_torrance (0.5, 0.1)));
  const Material *glass = scene.add (new Glass (1.5));
  const Material *gold
    = scene.add (new Mirror (Ior (0.25, 3), Color (0.852, 0.756, 0.12), 0,
			     cook_torrance (Color (1, 1, 0.3), 0.01,
					    Ior (0.25, 3))));

  bool goldbunny = (arg == "gold");

  Mesh *bunny = new Mesh (goldbunny ? gold : glass);
  if (num / 10 == 1)
    bunny->load ("+bunny69451.msh", Xform().scale(10).translate(0,-0.35,0));
  else
    bunny->load ("bunny500.msh", Xform().translate(0, 0.65, 0));
  bunny->compute_vertex_normals ();
  scene.add (bunny);

  scene.add (new Sphere (goldbunny ? glass : gold, Pos (-3, 0, -3), 1.5));

  scene.add (new Sphere (red,    Pos (3.5, 0.65, -5.0), 0.65));
  scene.add (new Sphere (green,  Pos (2.5, 0.40, -7.0), 0.40));
  scene.add (new Sphere (yellow, Pos (0.3, 0.40, -2.5), 0.40));

  add_chessboard (scene, 2);
  
  switch (num % 10)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      add_bulb (scene, Pos (0, 10, 0), 0.06, 300);
      add_bulb (scene, Pos (15, 2, 0), 0.06, 300);
      add_bulb (scene, Pos (0, 1, 15), 0.06, 300);
      break;

    case 2:
      add_rect_bulb (scene, Pos(-5, 10, -5), Vec(10, 0, 0), Vec(0, 0, 10), 6);
      break;
    case 3:
      add_rect_bulb (scene, Pos(-10, 0, 2), Vec(0, 10, 0), Vec(6, 0, 6), 10);
      break;
    }
}

static void
add_scene_descs_pretty_bunny (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("pretty-bunny", "Glass Stanford bunny with some spheres"));
  descs.push_back (TestSceneDesc ("goldbunny", "Gold Stanford bunny with some spheres"));
}



static void
def_scene_teapot (unsigned num, const string &arg, Scene &scene, Camera &camera)
{
  // The teapot and board meshes were originally produced as a .nff file by
  // the SPD package, which uses Z as the vertical axis.  This transform
  // mutates them into our preferred coordinate system.
  //
  Xform mesh_xform = Xform::x_rotation (-M_PI_2).scale (-1, 1, 1);

  // Pot

  const Material *silver
//     = scene.add (new Mirror (Ior (0.25, 3), 0.5, 0.1,
// 			     cook_torrance (0.8, 0.3, Ior (0.25, 3))));
    = scene.add (new Mirror (Ior (0.25, 3), 0.9, 0,
			     cook_torrance (1, 0.1, Ior (0.25, 3))));
  const Material *matte_silver
    = scene.add (new Material (0.1, cook_torrance (0.8, 0.1, Ior (0.25, 3))));
  const Material *gloss_blue
    = scene.add (new Mirror (4, 0.05, Color (0.3, 0.3, 0.6),
			     cook_torrance (0.4, 0.3, 4)));
  const Material *glass = scene.add (new Glass (1.5));
  const Material *gloss_neutral_grey
    = scene.add (new Material (0.5, cook_torrance (0.5, 0.03, 2)));
  const Material *semigloss_white
    = scene.add (new Material (0.9, cook_torrance (0.1, 0.05, 2)));
  const Material *semigloss_off_white
    = scene.add (new Material (0.8, cook_torrance (0.2, 0.05, 2)));

  const Material *teapot_mat = 0;
  switch ((num / 1000) % 10)
    {
    case 0: default:
      teapot_mat = silver; 	break;
    case 1:
      teapot_mat = matte_silver;break;
    case 2:
      teapot_mat = gloss_blue; 	break;
    case 3:
      teapot_mat = glass; 	break; // a bit dodgy, as surface is not closed
    case 4:
      teapot_mat = gloss_neutral_grey; break;
    case 5:
      teapot_mat = semigloss_off_white; break;
    case 6:
      teapot_mat = semigloss_white; break;
    }

  string mesh_file = arg.empty() ? "teapot.msh" : arg;
  Xform teapot_xform = mesh_xform * Xform::translation (Vec (0, -0.1, 0));
  scene.add (new Mesh (teapot_mat, mesh_file, teapot_xform, true));

  // Chessboard
  //
  add_chessboard (scene);

  // Table/ground

  const Material *grey
    = scene.add (new Material (Color (0.3, 0.2, 0.2),
			       cook_torrance (0.5, 0.2, Ior (1, 1))));
  const Material *green
    = scene.add (new Material (Color (0.1, 0.5, 0.1)));

  switch ((num / 100) % 10)
    {
    case 0:
      // Narrow grey "plinth"
      //
      {
	dist_t tw = 16;
	add_cube (scene, grey, Pos (-tw / 2, -1, -tw / 2),
		  Vec (tw, 0, 0), Vec (0, -tw, 0), Vec (0, 0, tw));
      }
      break;

    case 1:
      // green ground plane, wide grey "table"
      //
      add_rect (scene, grey, Pos (-14, -1, -14), Vec (38, 0, 0), Vec (0, 0, 38));
      add_rect (scene, green, Pos (-100, -3, -100), Vec (200, 0, 0), Vec (0, 0, 200));
      break;
    }

  switch ((num / 10) % 10)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      // night-time teapot
      //
      add_bulb (scene, Pos (3.1, 12.1, -9.8), 1, 100);
      //scene.add (new PointLight (Pos (3.1, 12.1, -9.8), 600));
      //scene.add (new PointLight (Pos (-11.3, 8.8, -5.1), 5));
      add_bulb (scene, Pos (-4.7, 3, -2), 0.2, 60 * Color (1, 1, 0.3));
      add_bulb (scene, Pos (1, 4, 2), 0.2, 60 * Color (1, 1, 0.3));
      break;
      
    case 2:
      // night-time teapot, area lights
      //
      add_rect_bulb (scene, Pos(3.1, 12.1, -9.8), Vec(-5, 0, 0), Vec(0, 5, 0),
		     50);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 3), Vec (0, 3, 0),
		     15 * Color (1, 1, 0.3));
      break;

    case 3:
      // day-time teapot, area lights
      //
      scene.add (new FarLight (Vec (1, 1, -0.5), 0.05, 6));
      scene.set_background (Color (0.078, 0.361, 0.753));
      break;
      
    case 4:
      // night-time teapot, area lights, strong overhead
      //
      add_rect_bulb (scene, Pos (3, 6, -3), Vec (-6, 0, 0), Vec (0, 0, 6), 6);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 1), Vec (0, 1, 0),
		     40 * Color (1, 1, 0.3));
      break;
 
    case 5:
      // night-time teapot, area lights, wide overhead
      //
      add_rect_bulb (scene, Pos(10, 6, -10), Vec(-20, 0, 0), Vec(0, 0, 20), 4);
      break;
      
    case 6:
      // night-time teapot, area lights, strong front light
      //
      add_rect_bulb (scene, Pos (-3, 0, -8), Vec (6, 0, 0), Vec (0, 3, 0), 6);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 3), Vec (0, 3, 0),
		     10 * Color (1, 1, 0.3));
      break;

    case 7:
      // surrounding area lights
      //
      {
	// Lights

	float b = 12;			// brightness
	dist_t ld = 12, lh = 6, lw = 8; // distance (from origin), height, width
	Vec lhv (0, lh, 0);		// height vector

	add_rect_bulb (scene, Pos(-ld,     0, -lw / 2), Vec(0,  0, lw), lhv, b);
	add_rect_bulb (scene, Pos( ld,     0, -lw / 2), Vec(0,  0, lw), lhv, b);
	add_rect_bulb (scene, Pos(-lw / 2, 0,      ld), Vec(lw, 0,  0), lhv, b);

	// Light bezels

	dist_t bd = ld + 0.1, bh = 1 + lh + 1, bw = lw + 2;;
	Vec bhv (0, bh, 0);

	add_rect (scene, grey, Pos(-bd,     -1, -bw / 2), Vec(0,  0, bw), bhv);
	add_rect (scene, grey, Pos( bd,     -1, -bw / 2), Vec(0,  0, bw), bhv);
	add_rect (scene, grey, Pos(-bw / 2, -1,  bd),     Vec(bw, 0,  0), bhv);
      }
      break;
    }

  if (num % 10 > 0)
    {
      const Material *orange
	= scene.add (new Material (Color (0.6,0.5,0.05), cook_torrance (0.4, 0.1)));
      const Material *glass = scene.add (new Glass (1.5));
      const Material *gold
	= scene.add (new Mirror (Ior (0.25, 3), Color (0.852, 0.756, 0.12), 0,
				 cook_torrance (Color (1, 1, 0.3), 0.01,
						Ior (0.25, 3))));

      dist_t max_err = 0.0002;

      switch (num % 10)
	{
	case 1:
	  scene.add (tobj (TOBJ_SPHERE, gold, Pos (-3, 0, -2), 0.6, max_err));
	  break;
	case 2:
	  scene.add (new Sphere (glass, Pos (-3, 0.5, -2), 0.5));
	  break;
	case 3:
	  scene.add (tobj (TOBJ_SINC, gold, Pos (-3, 0, -2), 0.6, max_err));
	  break;
	case 4:
	  scene.add (tobj (TOBJ_TORUS, gold, Pos (-3, 0, -2), 1, max_err));
	  break;
	case 5:
	  scene.add (tobj (TOBJ_SPHERE, orange, Pos (-3, 0, -2), 1, max_err));
	  break;
	}

      const Material *red
	= scene.add (new Material (Color (0.5, 0, 0),
				   cook_torrance (0.5, 0.1)));
      const Material *yellow
	= scene.add (new Material (Color (0.5, 0.5, 0),
				   cook_torrance (0.5, 0.1)));
      const Material *green
	= scene.add (new Material (Color (0, 0.5, 0),
				   cook_torrance (0.5, 0.1)));
      const Material *blue
	= scene.add (new Material (Color (0.1, 0.1, 0.5),
				   cook_torrance (0.5, 0.1)));

      scene.add (tobj (TOBJ_SINC,   blue,   Pos (-1.5, 0, -3.3), 0.4, max_err));
      scene.add (tobj (TOBJ_SPHERE, green,  Pos (3, 0, -1.2), 0.4, max_err));
      scene.add (tobj (TOBJ_SINC,   yellow, Pos (2.2, 0, -3.1), 0.7, max_err));
      scene.add (tobj (TOBJ_SPHERE, red,    Pos (-2.3, 0, 1.7), 0.7, max_err));
    }

  camera.set_vert_fov (M_PI_4 * 0.9);
  camera.move (Pos (-4.86, 5.4, -7.2));
  camera.point (Pos (0, 0, 0.2), Vec (0, 1, 0));
}

static void
add_scene_descs_teapot (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("teapot[0-4]0", "Classic teapot"));
  descs.push_back (TestSceneDesc ("teapot[0-4]1", "Classic teapot with orange"));
  descs.push_back (TestSceneDesc ("teapot[0-4]2", "Classic teapot with glass ball"));
  descs.push_back (TestSceneDesc ("teapot[0-4]3", "Classic teapot with gold ball"));
  descs.push_back (TestSceneDesc ("teapot0[0-9]", "Teapot Night lighting (point lights)"));
  descs.push_back (TestSceneDesc ("teapot1[0-9]", "Teapot Daytime lighting (point lights)"));
  descs.push_back (TestSceneDesc ("teapot2[0-9]", "Teapot Night lighting (area lights)"));
  descs.push_back (TestSceneDesc ("teapot3[0-9]", "Teapot Daytime lighting (area lights)"));
  descs.push_back (TestSceneDesc ("teapot4[0-9]", "Teapot Night lighting (overhead light)"));

}



static void
def_scene_balls (unsigned num, const string &, Scene &scene, Camera &camera)
{
  // Chessboard
  //
  add_chessboard (scene);

  // Table/ground

  const Material *grey
    = scene.add (new Material (Color (0.3, 0.2, 0.2),
			       cook_torrance (0.5, 0.2, Ior (1, 1))));

  dist_t tw = 16;
  add_cube (scene, grey, Pos (-tw / 2, -1, -tw / 2),
	    Vec (tw, 0, 0), Vec (0, -tw, 0), Vec (0, 0, tw));

  unsigned saturation = (num % 10);
  unsigned lighting = ((num / 10) % 10);

  switch (lighting)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      add_bulb (scene, Pos (3.1, 12.1, -9.8), 1, 200);
      //scene.add (new PointLight (Pos (3.1, 12.1, -9.8), 600));
      //scene.add (new PointLight (Pos (-11.3, 8.8, -5.1), 5));
      add_bulb (scene, Pos (-4.7, 3, -2), 0.2, 120 * Color (1, 1, 0.3));
      add_bulb (scene, Pos (1, 4, 2), 0.2, 120 * Color (1, 1, 0.3));
      break;
      
    case 2:
      // night-time balls, area lights
      //
      add_rect_bulb (scene, Pos(3.1, 12.1, -9.8), Vec(-5, 0, 0), Vec(0, 5, 0),
		     100);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 3), Vec (0, 3, 0),
		     15 * Color (1, 1, 0.3));

    case 3:
      // day-time balls, area lights
      //
      scene.add (new FarLight (Vec (1, 1, -0.5), 0.05, 6));
      scene.set_background (Color (0.078, 0.361, 0.753));
      break;
      
    case 4:
      // night-time balls, area lights, strong overhead and small side light
      //
      add_rect_bulb (scene, Pos (3, 6, -3), Vec (-6, 0, 0), Vec (0, 0, 6), 12);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 1), Vec (0, 1, 0),
		     80 * Color (1, 1, 0.3));
      break;
 
    case 5:
      // night-time balls, area lights, wide overhead
      //
      add_rect_bulb (scene, Pos(10, 6, -10), Vec(-20, 0, 0), Vec(0, 0, 20), 4);
      break;

    case 6:
      // night-time balls, area lights, strong front light
      //
      add_rect_bulb (scene, Pos (-3, 0, -8), Vec (6, 0, 0), Vec (0, 3, 0), 20);
      add_rect_bulb (scene, Pos (-6, 0, -2), Vec (0, 0, 3), Vec (0, 3, 0),
		     20 * Color (1, 1, 0.3));
      break;

    case 7:
      // surrounding area lights
      //
      {
	// Lights

	float b = 25;			// brightness
	dist_t ld = 12, lh = 6, lw = 8; // distance (from origin), height, width
	Vec lhv (0, lh, 0);		// height vector

	add_rect_bulb (scene, Pos(-ld,     0, -lw / 2), Vec(0,  0, lw), lhv, b);
	add_rect_bulb (scene, Pos( ld,     0, -lw / 2), Vec(0,  0, lw), lhv, b);
	add_rect_bulb (scene, Pos(-lw / 2, 0,      ld), Vec(lw, 0,  0), lhv, b);

	// Light bezels

	dist_t bd = ld + 0.1, bh = 1 + lh + 1, bw = lw + 2;;
	Vec bhv (0, bh, 0);

	add_rect (scene, grey, Pos(-bd,     -1, -bw / 2), Vec(0,  0, bw), bhv);
	add_rect (scene, grey, Pos( bd,     -1, -bw / 2), Vec(0,  0, bw), bhv);
	add_rect (scene, grey, Pos(-bw / 2, -1,  bd),     Vec(bw, 0,  0), bhv);
      }
      break;
    }

  Color colors[] = { Color (0.7, 0.3, 0), Color (0, 0.1, 0.9), Color (0, 1, 0),
		     Color (0.2, 0.4, 0.1), Color (0.4, 0.3, 0.2) };
  const unsigned num_colors = (sizeof colors) / (sizeof colors[0]);

  const unsigned rank = 4;
  const dist_t width = (dist_t (rank) - 1) / 2;
  const unsigned num_balls = rank * rank;
  for (unsigned i = 0; i < num_balls; i++)
    {
      dist_t rad = 0.65;
      dist_t sep = rad * 2 * 1.5;

      coord_t x = ((i / rank) - width) * sep;
      coord_t y = rad;
      coord_t z = ((i % rank) - width) * sep;

      Color base_col = colors[i % num_colors] * 0.6;

      float col_rand = 0.4;
      Color col
	= base_col
	- random (col_rand * base_col)
	+ random (col_rand * (1 - base_col));

      Color pastel = base_col;
      switch (saturation)
	{
	default:
	case 0: /* nothing */ break;
	case 1: pastel = (pastel + 1) / 2; break;
	case 2: pastel = (pastel + 0.2) / 1.2; break;
	}

      float spec = powf (1.5f, float (i) - float (num_balls * 0.75));

      const Material *mat;
      if (i % 3 == 1)
	mat = new Glass (Medium (1.5, (base_col + 0.2) / 1.2));
      else
	mat = new Material (col * 0.7, cook_torrance (0.3, spec));

      scene.add (mat);
      scene.add (new Sphere (mat, Pos (x, y, z), rad));
    }

  camera.set_vert_fov (M_PI_4 * 0.9);
  camera.move (Pos (-4.86, 5.4, -7.2));
  camera.point (Pos (0, 0, 0.2), Vec (0, 1, 0));
}

static void
add_scene_descs_balls (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("balls", "Balls of varying specularity"));
}



static void
def_scene_orange (unsigned num, const string &, Scene &scene, Camera &camera)
{
  const Material *silver
    = scene.add (new Mirror (Ior (0.25, 3), 0.5, 0.1,
			     cook_torrance (0.8, 0.3, Ior (0.25, 3))));
  const Material *orange
    = scene.add (new Material (Color (0.6, 0.5, 0.05), phong (0.4, 250)));
  const Material *glass = scene.add (new Glass (1.5));

  add_chessboard (scene);

  unsigned lighting  = (num / 10) % 10;
  num = (num % 10);

  switch (lighting)
    {
    case 0:
      // night-time orange
      add_bulb (scene, Pos (-3.1, 12.1, 9.8), 1, 80);
      add_bulb (scene, Pos (4.7, 3, 2), 0.2, 40 * Color (1, 1, 0.3));
      add_bulb (scene, Pos (-1, 4, -2), 0.2, 40 * Color (1, 1, 0.3));
      break;

    case 1:
      // day-time orange
      scene.add (new FarLight (Vec (-1, 1, 0.5), 0.05, 1));
      scene.add (new FarLight (Vec (0, 0, 1), 1, 1));
      scene.set_background (Color (0.078, 0.361, 0.753));
      break;

    case 2:
      // night-time orange 2
      add_rect_bulb (scene, Pos (6, 0, 2), Vec (0, 0, -3), Vec (0, 3, 0),
		     2 * Color (1, 1, 0.3));
      add_rect_bulb (scene, Pos (4,6, 0), Vec(1,0,0), Vec(0,8,0),
		     Color(10,10,6));
      break;
    }

  dist_t max_err = 0.0002;
  bool smooth = true;

  const Material *mat;
  switch (num)
    {
    default:
    case 0: mat = orange; break;
    case 1: mat = silver; break;
    case 2: mat = glass; max_err = 0.001; break;
    }

  scene.add (new Mesh (mat, SphereTesselFun (Pos (0, 3, 0), 3, 0.002),
		       Tessel::ConstMaxErr (max_err), smooth));

  camera.set_vert_fov (M_PI_4 * 0.9);
  camera.move (Pos (4.86, 5.4, 7.2));
  camera.point (Pos (0, 0, -0.2), Vec (0, 1, 0));
}

static void
add_scene_descs_orange (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("orange", "Giant orange on a chessboard"));
  descs.push_back (TestSceneDesc ("orange1", "Big rough silver ball on a chessboard"));
  descs.push_back (TestSceneDesc ("orange2", "Big rough glass ball on a chessboard"));
}



static void
def_scene_cornell_box (unsigned num, const string &,
		       Scene &scene, Camera &camera)
{
  float light_intens = 15;
  bool fill_light = true;
  float scale = 1;

  unsigned scn = num % 10;
  unsigned floor = (num / 10) % 10;
  unsigned walls = (num / 100) % 10;

  coord_t rear   =  2   * scale, front = -3   * scale;
  coord_t left   = -1.2 * scale, right =  1.2 * scale;
  coord_t bottom =  0   * scale, top   =  2   * scale;

  dist_t width   = right - left;
  dist_t depth	 = rear - front;
  dist_t height  = top - bottom;
  coord_t mid_x  = left + width / 2;
  coord_t mid_z  = 0;

  dist_t light_width = width / 3;
  dist_t light_inset = 0.01 * scale;
  coord_t light_x    = left + width / 2;
  coord_t light_z    = 0;

  // Various spheres use this radius
  //
  dist_t rad = 0.4 * scale;

  // Appearance of left and right walls; set in ifs below
  //
  const Material *left_wall_mat, *right_wall_mat;

  // Corners of room (Left/Right + Bottom/Top + Rear/Front)
  //
  const Pos LBR (left, bottom, rear),   RBR (right, bottom, rear);
  const Pos RTR (right, top, rear),     LTR (left, top, rear);
  const Pos RBF (right, bottom, front), RTF (right, top, front);
  const Pos LBF (left, bottom, front),  LTF (left, top, front);

  const Material *wall_mat = scene.add (new Material (1));

  if (scn >= 1)
    {
      fill_light = false;
      light_intens *= 1.5;
      light_z += scale * 0.2;

      const Material *glass, *metal;

      switch (scn)
	{
	default:
	case 1:
	  // Default: metal with slightly non-perfect reflection, and
	  // yellow-green glass.
	  //
	  glass = new Glass (Medium (1.5, Color (0.3, 0.3, 0.9)));
	  metal = new Mirror (Ior (0.25, 3), 0.9, 0,
			      cook_torrance (1, 0.1, Ior (0.25, 3)));
	  break;

	case 2:
	  // Perfect materials
	  //
	  glass = new Glass (1.5);
	  metal = new Mirror (Ior (0.25, 3), 0.9);
	  break;
	}

      scene.add (metal);
      scene.add (glass);

      scene.add (new Sphere (metal, LBR + Vec (rad*1.55, rad, -rad*3), rad));
      scene.add (new Sphere (glass, Pos (right - rad*1.5, rad, -rad), rad));

      left_wall_mat = scene.add (new Material (Color (0.6, 0.1, 0.1)));
      right_wall_mat = scene.add (new Material (Color (0.1, 0.1, 0.6)));
    }
  else // default
    {
      const Material *gloss_blue
	= scene.add (new Mirror (4, 0.05, Color (0.3, 0.3, 0.6),
			     cook_torrance (0.4, 0.3, 4)));
      const Material *white
	= scene.add (new Material (0.8, cook_torrance (0.2, 0.5, 2)));

      // blue sphere
      scene.add (new Sphere (gloss_blue, RBR + Vec (-rad*1.7, rad, -rad*4), rad));
      left_wall_mat = scene.add (new Material (Color (1, 0.35, 0.35)));
      right_wall_mat = scene.add (new Material (Color (0.35, 1, 0.35)));

      dist_t cube_sz = height * 0.4;
      float cube_angle = 50 * (M_PIf / 180);
      Vec cube_up (0, cube_sz, 0);
      Vec cube_right (cube_sz * cos(cube_angle), 0, cube_sz * sin(cube_angle));
      Vec cube_fwd (cube_sz * -sin(cube_angle), 0, cube_sz * cos(cube_angle));
      add_cube (scene, white,
		Pos (mid_x - width / 4.5, bottom, mid_z - width / 3),
		cube_up, cube_right, cube_fwd);
    }

  // light

  const coord_t light_left  = light_x - light_width / 2;
  const coord_t light_right = light_x + light_width / 2;
  const coord_t light_front = light_z - light_width / 2;
  const coord_t light_back  = light_z + light_width / 2;

  // If true, omit the light, the back wall, and the ceiling.
  //
  bool open = walls != 0;
  bool really_open = walls > 1;

  if (! open)
    add_rect_bulb (scene, Pos (light_left, top + light_inset, light_front),
		   Vec (light_width, 0, 0), Vec (0, 0, light_width),
		   light_intens);

  // Back wall
  //
  if (! open)
    add_rect (scene, wall_mat, LBR, LTR, RTR);

  // Right wall
  //
  if (! really_open)
    add_rect (scene, right_wall_mat, RBR, RTR, RTF);

  // Left wall
  //
  if (! really_open)
    add_rect (scene, left_wall_mat, LBR, LTR, LTF);

  // Ceiling
  //
  if (! open)
    {
      add_rect (scene, wall_mat, LTF, LTR, Pos (light_left, top, rear));
      add_rect (scene, wall_mat, RTR, RTF, Pos (light_right, top, front));
      add_rect (scene, wall_mat,
		Pos (light_left, top, front),
		Pos (light_left, top, light_front),
		Pos (light_right, top, light_front));
      add_rect (scene, wall_mat,
		Pos (light_left, top, light_back),
		Pos (light_left, top, rear),
		Pos (light_right, top, rear));
    }

  // Floor
  //
  if (floor == 0)
    add_rect (scene, wall_mat, LBF, LBR, RBR);
  else
    add_chessboard (scene, max (width, depth) / 8, (num / 10 - 1) % 10);

  if (scene.env_map)
    fill_light = false;

  // for debugging
  //
  if (fill_light)
    scene.add (new PointLight (Pos (left + 0.1, bottom + 0.1, front + 0.1),
			       light_intens / 5));

  camera.move (Pos  (mid_x, 0.525 * height + bottom, -6.6 * scale));
  camera.point (Pos (mid_x, 0.475 * height + bottom, 0), Vec (0, 1, 0));
  camera.set_horiz_fov (M_PI_4 * 0.7);
}

static void
add_scene_descs_cornell_box (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("cbox0", "Cornell box, Henrik Jensen version 1 (simulated soft shadows)"));
  descs.push_back (TestSceneDesc ("cbox1", "Cornell box, Henrik Jensen version 0 (glass & mirror spheres)"));
}


// CS465 test scenes

// from cs465 Test1.xml
static void
def_scene_cs465_test1 (Scene &scene, Camera &camera)
{
  // First test scene, only uses spheres, Lambertian shading,
  // and one light directly above the center of the 3 spheres.

  const Material *mat1
    = scene.add (new Material (Color (1, 0.5, 0.2), phong (1, 300)));
  const Material *mat2
    = scene.add (new Material (Color (0.8, 0.8, 0.8), phong (.2, 300)));
  const Material *mat3
    = scene.add (new Material (Color (0.8, 0, 0), phong (.2, 400)));

  camera.move (Pos (0, 3, -4));
  camera.point (Pos (0, 0, 0), Vec (0, 1, 0));
  scene.add (new Sphere (mat1, Pos (0, 0, -0.866), 1));
  scene.add (new Sphere (mat2, Pos (1, 0, 0.866), 1));
  scene.add (new Sphere (mat3, Pos (-1, 0, 0.866), 1));
  scene.add (new PointLight (Pos (0, 5, 0), 25));
}

static void
def_scene_cs465_test2 (Scene &scene, Camera &camera)
{
  // Sphere on plane.  Sphere has greenish phong material.

  camera.move (Pos (0, 4, 4));
  camera.point (Pos (-0.5, 0, 0.5), Vec (0, 1, 0));

  const Material *sphereMat
    = scene.add (new Material (Color (0.249804, 0.218627, 0.0505882),
			       phong (.3, 100)));
  const Material *grey
    = scene.add (new Material (Color (0.3, 0.3, 0.3)));

  scene.add (new Sphere (sphereMat, Pos (0, 0, 0), 1));

  // ground
  add_rect (scene, grey, Pos (-10, -1, -10), Vec (20, 0, 0), Vec (0, 0, 20));

  // Small Area type light
  scene.add (new PointLight (Pos (5, 5, 0), 8));
  scene.add (new PointLight (Pos (5.1, 5, 0), 8));
  scene.add (new PointLight (Pos (5.2, 5, 0), 8));
  scene.add (new PointLight (Pos (5.3, 5, 0), 8));
  scene.add (new PointLight (Pos (5, 5.1, 0), 8));
  scene.add (new PointLight (Pos (5.1, 5.1, 0), 8));
  scene.add (new PointLight (Pos (5.2, 5.1, 0), 8));
  scene.add (new PointLight (Pos (5.3, 5.1, 0), 8));
  scene.add (new PointLight (Pos (5, 5.2, 0), 8));
  scene.add (new PointLight (Pos (5.1, 5.2, 0), 8));
  scene.add (new PointLight (Pos (5.2, 5.2, 0), 8));
  scene.add (new PointLight (Pos (5.3, 5.2, 0), 8));
  scene.add (new PointLight (Pos (5, 5.3, 0), 8));
  scene.add (new PointLight (Pos (5.1, 5.3, 0), 8));
  scene.add (new PointLight (Pos (5.2, 5.3, 0), 8));
  scene.add (new PointLight (Pos (5.3, 5.3, 0), 8));

  // fill light
  scene.add (new PointLight (Pos (-5, 1, -22), 100));
}

static void
def_scene_cs465_test3 (Scene &scene, Camera &camera)
{
  // Three spheres and a box on a plane.  Mix of Lambertian
  // and Phong materials.

  camera.move (Pos (6, 6, 6));
  camera.point (Pos (0, 0, 0), Vec (0, 1, 0));

  const Material *shinyBlack
    = scene.add (new Material (Color (0.02, 0.02, 0.02), phong (1, 300)));
  const Material *shinyWhite
    = scene.add (new Material (Color (0.6, 0.6, 0.6), phong (1, 300)));
  const Material *shinyGray
    = scene.add (new Material (Color (0.2, 0.2, 0.2), phong (2, 300)));
  const Material *boxMat
    = scene.add (new Material (Color (0.3, 0.19, 0.09)));
  const Material *gray
    = scene.add (new Material (Color (0.6, 0.6, 0.6)));
	
  // box
  add_cube (scene, boxMat, Pos (-1, -1, -1),
	    Vec (2, 0, 0), Vec (0, 2, 0), Vec (0, 0, 2));

  // ground
  add_rect (scene, gray, Pos (-10, -1, -10), Vec (20, 0, 0), Vec (0, 0, 20));

  // spheres	
  scene.add (new Sphere (shinyBlack, Pos (0, 2, 0), 1));
  scene.add (new Sphere (shinyGray, Pos (0, 0, 2.5), 1));
  scene.add (new Sphere (shinyWhite, Pos (2.5, 0, 0), 1));

  scene.add (new PointLight (Pos (0, 10, 5), Color (50, 30, 30)));
  scene.add (new PointLight (Pos (5, 10, 0), Color (30, 30, 50)));
  scene.add (new PointLight (Pos (5, 10, 5), Color (30, 50, 30)));
  scene.add (new PointLight (Pos (6, 6, 6), Color (25, 25, 25)));
}

void 
def_scene_cs465_test4 (Scene &scene, Camera &camera, unsigned variant)
{
  // Low resolution Stanford Bunny Mesh.

  camera.set_z_mode (Camera::Z_DECREASES_FORWARD);

  switch (variant % 10)
    {
    case 0:
      // original scene#4 camera pos
      camera.move (Pos (0, 0, 3));
      break;

    case 1:
      camera.move (Pos (0, 3, .7));
      break;

    case 2:
      camera.move (Pos (-0.1, 1.8, 1.2));
      break;

    case 4:
      camera.move (Pos (10, 3, 10));
      break;
    }

  camera.point (Pos (-0.25, -0.07, 0), Vec (0, 1, 0));
  camera.set_vert_fov (M_PI_4);

  const Material *red;
  if (variant == 0)
    red = scene.add (new Material (Color (1, 0, 0))); // original, flat red
  else
    red = scene.add (new Mirror (1.5, 0.1, Color (.5, 0, 0),
				 cook_torrance (0.5, 0.1, 1.5))); // glossy red

  // Add bunny.  For variant 0, we use the original unsmoothed appearance;
  // for everythign else we do smoothing.
  //
  scene.add (new Mesh (red, "bunny500.msh", Xform::identity, (variant > 0)));
  
  add_chessboard (scene, Xform::translation (Vec (0, -0.65, 0)));
  
  switch ((variant / 10) % 10)
    {
    case 0:
    case 1:
      add_bulb (scene, Pos (0, 10, 0), .5, 250);
      add_bulb (scene, Pos (15, 2, 0), .5, 250);
      add_bulb (scene, Pos (0, 1, 15), .5, 250);
      break;

    case 2:
      add_rect_bulb (scene, Pos (-5, 10, -5), Vec(10, 0,0), Vec(0,0, 10), 5);
      break;

    case 3:
      add_rect_bulb (scene, Pos (-10, 0, -5), Vec(0, 10,0), Vec(0, 0, 10), 10);
      break;
    }
}

static void
def_scene_cs465 (unsigned num, const string &, Scene &scene, Camera &camera)
{
  switch (num)
    {
    case 1: def_scene_cs465_test1 (scene, camera); break;
    case 2: def_scene_cs465_test2 (scene, camera); break;
    case 3: def_scene_cs465_test3 (scene, camera); break;
    case 4: def_scene_cs465_test4 (scene, camera, 0); break;

    default:
      // others are variations on scene 4
      if (num >= 10)
	def_scene_cs465_test4 (scene, camera, num);
      else
	throw runtime_error ("unknown cs465 test scene");
    }
}

static void
add_scene_descs_cs465 (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("cs465-[1-4]", "Cornell CS465 test-scene 1-4"));
  descs.push_back (TestSceneDesc ("cs465-[1-3][0-4]", "Variations on CS465 test-scene 4"));
}



#if 0
static void
def_scene_cs465_kdtree (const string &name, unsigned num, Scene &scene, Camera &camera)
{
  const Material *red
    = scene.add (new Mirror (5, 0.1, Color (.5, 0, 0), 500)); // glossy red

//       const Material *red
// 	= scene.add (new Material (Color (1, 0, 0), 1, 500));
  const Material *yellow
    = scene.add (new Material (Color (1.5, 1.5, 0.1), 500));
  const Material *green
    = scene.add (new Material (Color (0, 1, 0), 500));
  const Material *blue
    = scene.add (new Material (Color (0.3, 0.3, 1.2), 500));

  const string msh_file = "/tmp/eli-images1/kdtree4.xml.mesh";
  scene.add (new Mesh (red, msh_file, "Material0"));
  scene.add (new Mesh (yellow, msh_file, "Material1"));
  scene.add (new Mesh (green, msh_file, "Material2"));
  scene.add (new Mesh (blue, msh_file, "Material3"));
  scene.add (new Mesh (red, msh_file, "Material4"));
  scene.add (new Mesh (yellow, msh_file, "Material5"));
  scene.add (new Mesh (green, msh_file, "Material6"));
  scene.add (new Mesh (blue, msh_file, "Material7"));
  
  //scene.add (new PointLight (Pos (6, 8, 10), 100));
  add_rect_bulb (scene, Pos (-15, -5, -5), Vec (0, 10, 0), Vec (0, 0, 10), 1.5);
  add_rect_bulb (scene, Pos (15, -5, -5), Vec (0, 10, 0), Vec (0, 0, 10), 1.5);
  add_rect_bulb (scene, Pos (-5, -5, -20), Vec (10, 0, 0), Vec (0, 10, 0), 1.5);

  camera.set_z_mode (Camera::Z_DECREASES_FORWARD);
  //camera.move (Pos (9.7, 9.7, 5.8));
  camera.move (Pos (0, 7, 25));
  camera.point (Pos (0, 0, 0), Vec (0, 1, 0));
  camera.set_vert_fov (M_PI_4);
}
#endif

static void
def_scene_pretty_dancer (unsigned num, const string &,
			 Scene &scene, Camera &camera)
{
  // Simple colored materials
  //
  struct SimpleNamedMat{
    const char *name;
    float m;
    Color diff;
    Color spec;
    Ior ior;
  };
  static const SimpleNamedMat materials[] = {
    { "Material0",  0,   Color (1.0, 0.8, 0.8), 0, 0 },
    { "Material1",  0,   Color (1.0, 0.7, 0.7), 0, 0 },
    { "Material2",  0.2, Color (0.8, 0.2, 0.2), Color (0.2,  0.5,  0.3), 0 },
    { "Material3",  0.2, Color (0.5, 0.5, 0.0), Color (0.5,  0.2,  0.0), 0 },
    { "Material4",  0.3, Color (1.0, 0.6, 0.6), 0, 0 },
    { "Material5",  0.3, Color (0.8, 0.2, 0.2), 0, 0 },
    { "Material6",  0.2, Color (0.0, 0.0, 0.5), Color (0.2,  0.4,  0.5), 0 },
    { "Material7",  0.3, Color (0.5, 0.1, 0.1), Color (0.3,  0.4,  0.0), 0 },
    //{ "Material8",  Color (1.0, 1.0, 0.2), Color (0.0, 10.0,  0.0), 40 },
    { "Material9",  0.3, Color (0.0, 0.5, 0.5), Color (0.0,  0.5,  0.0), 0 },
    { "Material10", 0.5, Color (0.4, 0.3, 0.1), Color (0.0,  0.5,  0.4), 0 },
    { "Material11", 0.5, Color (0.5, 0.0, 0.5), Color (0.0,  0.0,  0.5), 0 },
    { "Material12", 0.5, Color (0.1, 0.35, 0.4), Color (0.0,  0.5, 0.5), 0 },
    { "Material13", 0.3, Color (0.4, 0.3, 0.3), Color (0.0,  0.5,  0.0), 0 },
    { "Material14", 0.5, Color (0.0, 0.5, 0.1), Color (0.0,  0.3,  0.5), 0 },
    { 0, 0, 0, 0, 0 }
  };
  
  // More complex materials
  //
  struct NamedMat { const char *name; const Material *mat; };
  const NamedMat material_refs[] = {
    { "Material8",		// gold
      new Mirror (Ior (0.25, 3), Color (0.852, 0.756, 0.12), 0,
		  cook_torrance (Color (1, 1, 0.3), 0.1, Ior (0.25, 3))) },
    { 0, 0 }
  };

  const string msh_file_base = "+pretty-dancer";
  const string msh_file_ext = ".msh";

  for (const SimpleNamedMat *sm = materials; sm->name; sm++)
    {
      const string msh_file = msh_file_base + "-" + sm->name + msh_file_ext;

      const Brdf *brdf
	= (sm->m != 0
	   ? static_cast<const Brdf *> (
				 cook_torrance (sm->spec, sm->m,
						sm->ior == 0 ? 5 : sm->ior))
	   : static_cast<const Brdf *> (lambert));

      const Material *mat = scene.add (new Material (sm->diff, brdf));

      scene.add (new Mesh (mat, msh_file, Xform::identity, sm->name));
    }
  for (const NamedMat *nm = material_refs; nm->name; nm++)
    {
      const string msh_file = msh_file_base + "-" + nm->name + msh_file_ext;
      scene.add (nm->mat);
      scene.add (new Mesh (nm->mat, msh_file, Xform::identity, nm->name));
    }

  bool birthday_card = (num / 1000) > 0;
  unsigned stage     = (num / 100) % 10;
  unsigned lighting  = (num / 10)  % 10;
  num %= 10;

  const Material *gloss_black
    = scene.add (new Mirror (1.5, 0.2, 0.02, cook_torrance (0.9, 0.2)));
  const Material *gloss_ivory
    = scene.add (new Mirror (1.5, 0.2, Color (0.8, 0.7, 0.5),
			     cook_torrance (0.2, 0.2)));
  const Material *stage_mat = (stage == 1) ? gloss_ivory : gloss_black;

  add_rect (scene, stage_mat, Pos (-5, -2.2, 5), Vec (10, 0, 0), Vec (0, 0, -10));
  add_rect (scene, stage_mat, Pos (-5, -2.2, 5), Vec (10, 0, 0), Vec (0, -2, 0));

  if (birthday_card)
    {
      const Material *text_mat
	= scene.add (new Mirror (Ior (0.25, 3), Color (0.5, 0.6, 0.5)));
// 	= scene.add (new Mirror (10, 0.2, Color (1.2, 1.2, 0.8), phong (.1, 500)));
// 	= scene.add (new Mirror (10, 0.3, Color (0.5, 0.6, 0.5), phong (.3, 100)));
// 	= scene.add (new Mirror (10, 0.3, Color (0.7, 0.8, 0.7), phong (.2, 10 )));

      scene.add (new Mesh (text_mat, "+eli-birthday.msh"));
    }

  switch (lighting)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      if (! birthday_card)
	{
	  scene.add (new PointLight (Pos (6, 8, 10), 100));
	  break;
	}

      // otherwise, fall through

    case 1:
      // indoor lighting -- big lights on sides and in back

      add_rect_bulb (scene, Pos (-15, -5, -5), Vec (0, 10, 0), Vec (0, 0, 10), 1.5);
      add_rect_bulb (scene, Pos (15, -5, -5), Vec (0, 10, 0), Vec (0, 0, 10), 1.5);
      add_rect_bulb (scene, Pos (-5, -5, -20), Vec (10, 0, 0), Vec (0, 10, 0), 1.5);
      break;

    case 2:
      // like case 2, but with no explicitly visible light objects

      scene.add (new RectLight (Pos (-15, -5, -5),
				Vec (0, 10, 0), Vec (0, 0, 10), 150));
      scene.add (new RectLight (Pos (15, -5, -5),
				Vec (0, 10, 0), Vec (0, 0, 10), 150));
      scene.add (new RectLight (Pos (-5, -5, -20),
				Vec (10, 0, 0), Vec (0, 10, 0), 150));
      break;
    }

  if (birthday_card)
    {
      const Color red (0.7, 0.15, 0.15), green (0.15, 0.7, 0.15);
      const Material *shiny_red
	= scene.add (new Mirror (5, red, red / 12));
      const Material *shiny_green
	= scene.add (new Mirror (5, green, green / 8));
//       const Material *shiny_red
// 	= scene.add (new Mirror (5, Color (0.2, 0.05, 0.05),
// 				 Color (0.1, 0, 0), phong (0.9, 1000)));
//       const Material *shiny_green
// 	= scene.add (new Mirror (5, Color (0.05, 0.2, 0.05),
// 				 Color (0, 0.1, 0), phong (0.9, 1000)));
      const Material *glass = scene.add (new Glass (1.5));

      scene.add (new Sphere (shiny_red, Pos (3.2, -2.2 + 0.3, 4.2), 0.3));
      scene.add (new Sphere (shiny_green, Pos (3.6, -2.2 + 0.1, 4.5), 0.1));
      scene.add (new Sphere (glass, Pos (4, -2.2 + 0.2, 3), 0.2));
      scene.add (new Sphere (shiny_green, Pos (-2, -2.2 + 0.3, 3), 0.3));
      scene.add (new Sphere (glass, Pos (-3.5, -2.2 + 0.2, 2), 0.2));
    }

  camera.set_z_mode (Camera::Z_DECREASES_FORWARD);

  switch (num)
    {
    case 0:
      if (birthday_card)
	camera.move (Pos (4.51, 2.365, 7.64));
      else
	camera.move (Pos ( 1.5,  1.7, 10));
      break;

    case 1:  camera.move (Pos ( 1.5,  1.7, 10));	break;
    case 2:  camera.move (Pos ( 3.13, 1.7,  5.2));	break;
    case 3:  camera.move (Pos ( 0,    1.7,  6));	break;
    case 4:  camera.move (Pos (-3.13, 1.7,  5.2));	break;
    case 5:  camera.move (Pos (-6,    1.7,  0));	break;
    case 6:  camera.move (Pos (-3.13, 1.7, -5.2));	break;
    case 7:  camera.move (Pos ( 0,    1.7, -6));	break;
    case 8:  camera.move (Pos ( 3.13, 1.7, -5.2));	break;
    case 9:  camera.move (Pos ( 6,    1.7,  0));	break;
    }

  camera.point (Pos (0.37, 0.37, 0.32), Vec (0, 1, 0));

  if (birthday_card && num == 0)
    {
      camera.move (Vec (0, -2, 0));
      camera.set_horiz_fov (55 * M_PI_2 / 90);
    }
  else
    camera.set_vert_fov (M_PI_4);
}

static void
add_scene_descs_pretty_dancer (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("pretty-dancer", "Eli's pretty-dancer scene"));
  descs.push_back (TestSceneDesc ("pretty-dancer-1", "Pretty-dancer with outdoor lighting"));
  descs.push_back (TestSceneDesc ("pretty-dancer-[2-9]", "Pretty-dancer closeups with outdoor lighting"));
  descs.push_back (TestSceneDesc ("pretty-dancer-1[1-9]", "Pretty-dancer closeups with indoor lighting"));
  descs.push_back (TestSceneDesc ("pretty-dancer-1[01][1-9]", "Pretty-dancer with white stage"));
}



// Blerg
//
float Snogray::tessel_accur = 0.001;
bool Snogray::tessel_smooth = true;

static void
def_scene_tessel (unsigned num, const string &arg, Scene &scene, Camera &camera)
{
  unsigned lighting = num / 10;
  num %= 10;

  coord_t height = -1.2;
  coord_t cheight = 0;

  camera.move (Pos (3, cheight + 0.25, -5));
  camera.point (Pos (0, -0.5, 0), Vec (0, 1, 0));

  num %= 10;

  const Material *silver
    = scene.add (new Mirror (Ior (0.25, 3), 0.5, 0.1,
			     cook_torrance (0.8, 0.3, Ior (0.25, 3))));
  const Material *gloss_blue
    = scene.add (new Mirror (4, 0.05, Color (0.3, 0.3, 0.6),
			     cook_torrance (0.4, 0.3, 4)));
  const Material *glass = scene.add (new Glass (1.5));

  const Material *mat;
  float light_intens = 100;
  switch (num % 4)
    {
    case 0:
    default:
      mat = gloss_blue;
      break;
    case 1:
      mat = silver;
      light_intens = 50;
      break;
    case 2:
      mat = glass;
      break;
    }

  num /= 4;

  Tessel::ConstMaxErr max_err (tessel_accur);

  // Sphere and torus accept a "perburb" factor
  dist_t perturb = 0;
  switch (num)
    {
    case 1: perturb = 0.001; break;
    case 2: perturb = 0.002; break;
    case 3: perturb = 0.01;  break;
    }

  if (arg == "sphere")
    scene.add (new Mesh (mat, SphereTesselFun (Pos (0, height + 1, 0), 1, perturb),
			 max_err, tessel_smooth));
  else if (arg == "sinc")
    scene.add (new Mesh (mat, SincTesselFun (Pos (0, height + 0.22, 0), 1.5),
			 max_err, tessel_smooth));
  else if (arg == "torus")
    scene.add (new Mesh (mat,
			 TorusTesselFun (Pos (0, height + 0.35, 0), 1, 0.3, perturb),
			 max_err, tessel_smooth));
  else
    throw (runtime_error ("Unknown tessellation test scene"));

  const Material *grey
    = scene.add (new Material (Color (0.3, 0.2, 0.2),
			       cook_torrance (0.7, 0.1, 1)));

  add_cube (scene, grey, Pos (1, height, 1),
	    Vec (0, 0, -2), Vec (-2, 0, 0), Vec (0, -1, 0));

  switch (lighting)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      add_bulb (scene, Pos (0, height + 5, 5), 1, light_intens);
      add_bulb (scene, Pos (-5, height + 5, -5), 1, 15);
      add_bulb (scene, Pos (10, height + -5, -15), 1, 100);
      break;

    case 2:
      add_rect_bulb (scene, Pos (-1, 0, -5), Vec (0, 3, 0), Vec (-3, 0, 3), 15);
      add_rect_bulb (scene, Pos (10, 0, -5), Vec (0, 7, 0), Vec (0, 0, 7), 5);
      break;
    }
}

static void
add_scene_descs_tessel (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("tessel-sinc-[0-3][01]", "Sinc function"));
  descs.push_back (TestSceneDesc ("tessel-sphere-[0-3][0-7]", "Tessellated sphere"));
  descs.push_back (TestSceneDesc ("tessel-torus-[0-3][0-7]", "Tessellated torus"));
}


// Test scene for looking at meshes

static void
normalize (Mesh *mesh, const SXform &xform = SXform::identity)
{
  const BBox bbox = mesh->bbox ();
  const SPos center = (bbox.max + bbox.min) / 2;
  const SVec size = bbox.max - bbox.min;

  SXform norm;
  norm.translate (-center.x, size.y / 2 - center.y, -center.z);
  norm.scale (2 / bbox.max_size ());
  norm *= xform;

  mesh->transform (norm);
}

static void
def_scene_mesh (unsigned num, const string &arg, Scene &scene, Camera &camera)
{
  unsigned csys  = (num / 10000) % 10;
  unsigned rot  = (num / 1000) % 10;
  unsigned base  = (num / 100) % 10;
  unsigned lighting  = (num / 10) % 10;
  num %= 10;

  camera.move (Pos (1, 1, 0.5));
  camera.point (Pos (0, 0.3, 0), Vec (0, 1, 0));
  camera.set_horiz_fov (M_PI_4);

  const Material *jade
    = scene.add (new Material (Color (0.3, 0.6, 0.3),
			       cook_torrance (0.4, 0.1, 2)));
  const Material *blue
    = scene.add (new Material (Color (0.3, 0.3, 0.6),
			       cook_torrance (0.4, 0.3, 4)));
  const Material *glass = scene.add (new Glass (1.5));
  const Material *silver
    = scene.add (new Mirror (Ior (0.25, 3), 0.5, 0.1,
			     cook_torrance (0.8, 0.3, Ior (0.25, 3))));
//const Material *matte_silver
//  = scene.add (new Material (0.1, cook_torrance (0.8, 0.3, Ior (0.25, 3))));
  const Material *grey
    = scene.add (new Material (Color (0.3, 0.2, 0.2),
			       cook_torrance (0.5, 0.2, Ior (1, 1))));
  const Material *dull_grey
    = scene.add (new Material (Color (0.2, 0.2, 0.2),
			       cook_torrance (0.8, 0.5, 5)));
  const Material *gloss_neutral_grey
    = scene.add (new Material (0.5, cook_torrance (0.5, 0.03, 2)));
  const Material *semigloss_off_white
    = scene.add (new Material (0.8, cook_torrance (0.2, 0.05, 2)));
  const Material *gloss_blue
    = scene.add (new Mirror (4, 0.05, Color (0.3, 0.3, 0.6),
			     cook_torrance (0.4, 0.3, 4)));
  const Material *moss
    = scene.add (new Material (Color (0.1, 0.2, 0.05),
			       cook_torrance (0.4, 0.1, 2)));
  const Material *mirror
    = scene.add (new Mirror (Ior (0.25, 3), 0.95));
    

  const Material *obj_mat;
  switch (num)
    {
    case 0:
    default:
      obj_mat = jade; break;
    case 1:
      obj_mat = glass; break;
    case 2:
      obj_mat = silver; break;
    case 3:
      obj_mat = moss; break;
    case 4:
      obj_mat = blue; break;
    case 5:
      obj_mat = dull_grey; break;
    case 6:
      obj_mat = mirror; break;
    case 7:
      obj_mat = semigloss_off_white; break;
    case 8:
      obj_mat = gloss_neutral_grey; break;
    case 9:
      obj_mat = gloss_blue; break;
    }

  Xform xform;

  // Mesh correction transforms
  //
  if (csys == 1)
    {
      // flip z-axis
      //
      xform.scale (1, 1, -1);
    }
  else if (csys == 2)
    {
      // Transform vertical Z axis into our preferred vertical Y axis
      //
      xform.rotate_x (-M_PI_2);
      xform.scale (-1, 1, 1);
    }

  // Presentation transforms
  //
  if (rot >= 1 && rot <= 3)
    {
      // rotate ROT * 90 deg counter-clockwise around y axis
      //
      xform.rotate_y (rot * M_PI_2);
    }

  Mesh *mesh = new Mesh (obj_mat, arg, xform, true);
  normalize (mesh, 0.4);
  scene.add (mesh);

  float floor_level = -0.2;	// default

  switch (base)
    {
    case 0:			// platform + chessboard floor
    default:
      add_cube (scene, grey, Pos (-0.5, 0, -0.5),
		Vec (1, 0, 0), Vec (0, 0, 1), Vec (0, floor_level, 0));
      add_chessboard (scene, Vec (0, floor_level, 0));
      break;

    case 1:			// no platform, only chessboard floor
      floor_level = 0;
      add_chessboard (scene);
      break;

    case 2:			// platform only
      add_cube (scene, grey, Pos (-0.5, 0, -0.5),
		Vec (1, 0, 0), Vec (0, 0, 1), Vec (0, floor_level, 0));
      floor_level = 0; // no floor, really, so pretend it's at top of platform
      break;

    case 3:			// wide platform only
      add_cube (scene, grey, Pos (-2, 0, -2),
		Vec (4, 0, 0), Vec (0, 0, 4), Vec (0, -1, 0));
      floor_level = 0; // platform is floor
      break;

    case 4:			// glass platform + chessboard floor
      add_cube (scene, glass, Pos (-0.5, 0, -0.5),
		Vec (1, 0, 0), Vec (0, 0, 1), Vec (0, floor_level, 0));
      add_chessboard (scene, Vec (0, floor_level, 0));
      break;

    case 5:			// glass platform only
      add_cube (scene, glass, Pos (-0.5, 0, -0.5),
		Vec (1, 0, 0), Vec (0, 0, 1), Vec (0, floor_level, 0));
      break;

    case 9:
      break;
    }

  float fbr = 0.2;
  float fby = floor_level + fbr;

  switch (lighting)
    {
    case 0:
      // default
      //
      if (scene.env_map)
	break;

      // otherwise, fall through

    case 1:
      add_rect_bulb (scene, Pos (-3, 5, -3), Vec (6, 0, 0), Vec (0, 0, 6), 5);
      break;

    case 2:
      add_rect_bulb (scene, Pos (-7, 5, -7), Vec (14, 0, 0), Vec (0, 0, 14), 2);
      break;

    case 3:
      add_rect_bulb (scene, Pos (7, 0, -5), Vec (0, 0, 10), Vec (0, 4, 0), 10);
      break;

    case 4:
      add_rect_bulb (scene, Pos (-1, 5, -1), Vec (2, 0, 0), Vec (0, 0, 2), 30);
      break;

    case 6:
      add_bulb (scene, Pos ( 2.0, fby,   1.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos ( 1.0, fby,  -2.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos ( 2.0, fby,  -1.0), fbr, Color (1, 1, 0.5) * 8);
      // fallthrough
    case 5:
      add_bulb (scene, Pos (-2.0, fby,  -1.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos (-1.0, fby,  -2.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos (-2.0, fby,   1.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos (-1.0, fby,   2.0), fbr, Color (1, 1, 0.5) * 8);
      add_bulb (scene, Pos ( 1.0, fby,   2.0), fbr, Color (1, 1, 0.5) * 8);
      break;
    }
}

static void
add_scene_descs_mesh (vector<TestSceneDesc> &descs)
{
  descs.push_back (TestSceneDesc ("mesh[0-3][0-3][0-1]", "Mesher mesh test"));
}



static void
def_scene_light (unsigned num, const string &arg, Scene &scene, Camera &)
{
  if (arg == "reset")
    {
      scene.lights.clear ();
      return;
    }

  if (arg == "grace")
    num = 8;
  else if (arg == "rnl")
    num = 9;

  switch (num)
    {
    case 0:
      scene.add (new PointLight (Pos (  10, 10,  0), 500));
      break;

    case 1:
      add_rect_bulb (scene, Pos (-3, 5, -3), Vec (6, 0, 0), Vec (0, 0, 6), 5);
      break;

    case 2:
      add_rect_bulb (scene, Pos (-7, 5, -7), Vec (14, 0, 0), Vec (0, 0, 14), 2);
      break;

    case 3:
      add_rect_bulb (scene, Pos (7, 0, -5), Vec (0, 0, 10), Vec (0, 4, 0), 10);
      break;

    case 4:
      scene.add (new FarLight (Vec (1, 1, -0.5), 0.05, 6));
      break;
    }
}



void
Snogray::def_test_scene (const string &_name, Scene &scene, Camera &camera)
{
  string name (_name);		// make a local copy

  string arg;
  unsigned arg_start = name.find_first_of (":");
  if (arg_start < name.length ())
    {
      arg = name.substr (arg_start + 1);
      name = name.substr (0, arg_start);
    }

  // Devide the name into a "base name" and "scene number" if possible
  //
  unsigned num = 0;
  unsigned base_end = name.find_last_not_of ("0123456789");
  if (base_end < name.length ())
    {
      istringstream idiots (name.substr (base_end + 1));
      idiots >> num;

      base_end = name.find_last_not_of ("-_ ", base_end);

      name = name.substr (0, base_end + 1);
    }

  if (name == "miles")
    def_scene_miles (num, arg, scene, camera);
  else if (name == "mesh")
    def_scene_mesh (num, arg, scene, camera);
  else if (name == "teapot")
    def_scene_teapot (num, arg, scene, camera);
  else if (name == "balls")
    def_scene_balls (num, arg, scene, camera);
  else if (name == "orange")
    def_scene_orange (num, arg, scene, camera);
  else if (ends_in (name, "bunny"))
    def_scene_pretty_bunny (num, arg, scene, camera);
  else if (name == "cornell-box" || name == "cbox")
    def_scene_cornell_box (num, arg, scene, camera);
  else if (name == "cs465")
    def_scene_cs465 (num, arg, scene, camera);
  else if (ends_in (name, "dancer"))
    def_scene_pretty_dancer (num, arg, scene, camera);
  else if (name == "tessel")
    def_scene_tessel (num, arg, scene, camera);
  else if (name == "light")
    def_scene_light (num, arg, scene, camera);
  else
    throw (runtime_error ("Unknown test scene"));
}

vector<TestSceneDesc>
Snogray::list_test_scenes ()
{
  vector<TestSceneDesc> descs;

  add_scene_descs_miles (descs);
  add_scene_descs_teapot (descs);
  add_scene_descs_balls (descs);
  add_scene_descs_orange (descs);
  add_scene_descs_pretty_bunny (descs);
  add_scene_descs_cornell_box (descs);
  add_scene_descs_cs465 (descs);
  add_scene_descs_pretty_dancer (descs);
  add_scene_descs_tessel (descs);
  add_scene_descs_mesh (descs);

  return descs;
}

// arch-tag: 307938a9-c663-4949-a58b-fb51040a6529
