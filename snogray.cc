// snogray.cc -- Main driver for snogray ray tracer
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
#include <iomanip>
#include <string>
#include <cstring>
#include <stdexcept>

#include "cmdlineparser.h"
#include "rusage.h"
#include "string-funs.h"
#include "progress.h"

#include "scene.h"
#include "camera.h"
#include "light.h"
#include "render.h"
#include "image-output.h"
#include "image-cmdline.h"
#include "render-cmdline.h"
#include "scene-def.h"
#include "trace-stats.h"

using namespace Snogray;
using namespace std;



static void
print_params (const Params &params, const string &name_pfx,
	      unsigned col1, unsigned col2)
{
  string pfx (col1, ' ');
  pfx += name_pfx;
  pfx += " ";
  unsigned pfx_len =  pfx.length ();

  for (Params::const_iterator p = params.begin(); p != params.end(); p++)
    cout << pfx << p->name << ": "
	 << setw (col2 - pfx_len - p->name.length() - 2)
	 << p->string_val() << endl;
}

// Print useful scene info
//
static void
print_scene_info (const Scene &scene, const SceneDef &scene_def)
{
  Space::Stats tstats = scene.space.stats ();

  cout << "Scene:" << endl;
      
  cout << "   scene:   " << setw (20) << scene_def.specs_rep() << endl;
  cout << "   top-level surfaces:  "
       << setw (8) << commify (scene.surfaces.size ()) << endl;
  cout << "   lights:          "
       << setw (12) << commify (scene.lights.size ()) << endl;

  print_params (scene_def.params, "scene", 3, 32);

  cout << "   materials:       "
       << setw (12) << commify (scene.materials.size ()) << endl;

  cout << "   tree surfaces:     "
       << setw (10) << commify (tstats.num_surfaces)
       << " (" << (tstats.num_dup_surfaces * 100 / tstats.num_surfaces)
       << "% duplicates)" << endl;
  cout << "   tree nodes:      "
       << setw (12) << commify (tstats.num_nodes)
       << " (" << (tstats.num_leaf_nodes * 100 / tstats.num_nodes)
       << "% leaves)" << endl;
  cout << "   tree avg depth:      "
       << setw (8) << int (tstats.avg_depth) << endl;
  cout << "   tree max depth:     "
       << setw (9) << tstats.max_depth << endl;
}



// Print image info
//
static void
print_image_info (const Params &image_params, const Params &render_params,
		  unsigned width, unsigned height,
		  unsigned limit_x, unsigned limit_y,
		  unsigned limit_width, unsigned limit_height)
{
  cout << "Image:" << endl;

  cout << "   size:    "
       << setw (20) << (stringify (width) + " x " + stringify (height))
       << endl;

  if (limit_x != 0 || limit_y != 0
      || limit_width != width || limit_height != height)
    cout << "   limit:  "
	 << setw (20) << (stringify (limit_x) + "," + stringify (limit_y)
			  + " - " + stringify (limit_x + limit_width)
			  + "," + stringify (limit_y + limit_height))
	 << " (" << limit_width << " x "  << limit_height << ")"
	 << endl;

  print_params (render_params, "render", 3, 32);
  print_params (image_params, "output", 3, 32);
}


// LimitSpec datatype

// A "limit spec" represents a user's specification of a single
// rendering limit (on x, y, width, or height), which may be absolute,
// relative, a proportion of the image size, or both.
//
struct LimitSpec
{
  LimitSpec (const char *_name, unsigned _abs_val)
    : name (_name), is_frac (false), abs_val (_abs_val), is_rel (false) { }
  LimitSpec (const char *_name, int _abs_val)
    : name (_name), is_frac (false), abs_val (_abs_val), is_rel (false) { }
  LimitSpec (const char *_name, float _frac_val)
    : name (_name), is_frac (true), frac_val (_frac_val), is_rel (false) { }
  LimitSpec (const char *_name, double _frac_val)
    : name (_name), is_frac (true), frac_val (_frac_val), is_rel (false) { }

  bool parse (const char *&str);
  unsigned apply (CmdLineParser &clp, unsigned range, unsigned base = 0) const;

  const char *name;

  bool is_frac;
  unsigned abs_val;
  float frac_val;

  bool is_rel;
};

bool
LimitSpec::parse (const char *&str)
{
  const char *skip = str + strspn (str, "0123456789");
  char *end = 0;

  is_frac = (*skip == '.' || *skip == '%');

  if (is_frac)
    // floating-point fractional spec
    {
      frac_val = strtof (str, &end);
      if (!end || end == str)
	return false;

      if (*end == '%')
	{
	  end++;
	  frac_val /= 100;
	}

      if (frac_val < 0 || frac_val > 1.0)
	return false;
    }
  else
    // integer absolute spec
    {
      abs_val = strtoul (str, &end, 10);
      if (!end || end == str)
	return false;
    }

  str = end;

  return true;
}

unsigned
LimitSpec::apply (CmdLineParser &clp, unsigned range, unsigned base) const
{
  unsigned val = is_frac ? (unsigned)(frac_val * range) : abs_val;

  if (is_rel)
    val += base;

  if (val > range)
    {
      cerr << clp.err_pfx()
	   << val << ": " << name
	   << " limit out of range (0 - " << range << ")" << endl;
      exit (5);
    }

  return val;
}


// Parsers for --size and --limit command-line option arguments

static void
parse_size_opt_arg (CmdLineParser &clp, unsigned &width, unsigned &height)
{
  const char *size = clp.opt_arg ();
  char *end = 0;

  width = strtoul (size, &end, 10);

  if (end && end != size)
    {
      size = end + strspn (end, " ,x");

      height = strtoul (size, &end, 10);

      if (end && end != size && *end == '\0')
	return;
    }

  clp.opt_err ("requires a size specification (WIDTHxHEIGHT)");
}

static void
parse_limit_opt_arg (CmdLineParser &clp,
		     LimitSpec &limit_x_spec, LimitSpec &limit_y_spec,
		     LimitSpec &limit_max_x_spec, LimitSpec &limit_max_y_spec)
{
  const char *spec = clp.opt_arg ();

  bool ok = limit_x_spec.parse (spec);
  if (ok)
    {
      spec += strspn (spec, ", ");

      ok = limit_y_spec.parse (spec);
      if (ok)
	{
	  spec += strspn (spec, " ");

	  limit_max_x_spec.is_rel = limit_max_y_spec.is_rel = (*spec == '+');

	  spec += strspn (spec, "+-");
	  spec += strspn (spec, " ");

	  ok = limit_max_x_spec.parse (spec);
	  if (ok)
	    {
	      spec += strspn (spec, " ,x");
	      ok = limit_max_y_spec.parse (spec);
	    }
	}
    }

  if (!ok || *spec != '\0')
    clp.opt_err ("requires a limit specification (X,Y[+-]W,H)");
}


// Main driver

static void
usage (CmdLineParser &clp, ostream &os)
{
  os << "Usage: " << clp.prog_name()
     << " [OPTION...] [SCENE_FILE... [OUTPUT_IMAGE_FILE]]" << endl;
}

static void
help (CmdLineParser &clp, ostream &os)
{
  usage (clp, os);

  // These macros just makes the source code for help output easier to line up
  //
#define s  << endl <<
#define n  << endl

  os <<
  "Ray-trace an image"
n
s "  -s, --size=WIDTHxHEIGHT    Set image size to WIDTH x HEIGHT pixels/lines"
n
s RENDER_OPTIONS_HELP
n
s SCENE_DEF_OPTIONS_HELP
n
s IMAGE_OUTPUT_OPTIONS_HELP
n
s "  -l, --limit=LIMIT_SPEC     Limit output to area defined by LIMIT_SPEC"
n
s "  -q, --quiet                Do not output informational or progress messages"
s "  -P, --no-progress          Do not output progress indicator"
s "  -p, --progress             Output progress indicator despite --quiet"
n
s CMDLINEPARSER_GENERAL_OPTIONS_HELP
n
s "If no input/output filenames are given, standard input/output are used"
s "respectively.  When no explicit scene/image formats are specified, the"
s "filename extensions are used to guess the format (so an explicit format"
s "must be specified when standard input/output are used)."
n
s SCENE_DEF_EXTRA_HELP
n;

#undef s
#undef n
}


int main (int argc, char *const *argv)
{
  // Command-line option specs
  //
  static struct option long_options[] = {
    { "size",		required_argument, 0, 's' },
    { "limit",		required_argument, 0, 'l' },
    { "quiet",		no_argument,	   0, 'q' },
    { "progress",	no_argument,	   0, 'p' },
    { "no-progress",	no_argument,	   0, 'P' },

    SCENE_DEF_LONG_OPTIONS,
    RENDER_LONG_OPTIONS,
    IMAGE_OUTPUT_LONG_OPTIONS,
    CMDLINEPARSER_GENERAL_LONG_OPTIONS,

    { 0, 0, 0, 0 }
  };
  //
  char short_options[] =
    "s:l:qpP"
    SCENE_DEF_SHORT_OPTIONS
    RENDER_SHORT_OPTIONS
    IMAGE_OUTPUT_SHORT_OPTIONS
    IMAGE_INPUT_SHORT_OPTIONS
    CMDLINEPARSER_GENERAL_SHORT_OPTIONS;
  //
  CmdLineParser clp (argc, argv, short_options, long_options);

  // Parameters set from the command line
  //
  unsigned width = 640, height = 480;
  LimitSpec limit_x_spec ("min-x", 0), limit_y_spec ("min-y", 0);
  LimitSpec limit_max_x_spec ("max-x", 1.0), limit_max_y_spec ("max-y", 1.0);
  bool quiet = false;
  Progress::Verbosity verbosity = Progress::CHATTY;
  bool progress_set = false;
  Params image_params, render_params;
  SceneDef scene_def;

  // This speeds up I/O on cin/cout by not syncing with C stdio.
  //
  ios::sync_with_stdio (false);

  // Parse command-line options
  //
  int opt;
  while ((opt = clp.get_opt ()) > 0)
    switch (opt)
      {
	// Size options
	//
      case 's':
	parse_size_opt_arg (clp, width, height);
	break;
      case 'l':
	parse_limit_opt_arg (clp, limit_x_spec, limit_y_spec,
			     limit_max_x_spec, limit_max_y_spec);
	break;

	// Verbosity options
	//
      case 'q':
	quiet = true;
	if (! progress_set)
	  verbosity = Progress::QUIET;
	break;
      case 'p':
	verbosity = Progress::CHATTY;
	progress_set = true;
	break;
      case 'P':
	verbosity = Progress::QUIET;
	progress_set = true;
	break;

	// Rendering options
	//
	RENDER_OPTION_CASES (clp, render_params);

	// Scene options
	//
	SCENE_DEF_OPTION_CASES (clp, scene_def);

	// Image options
	//
	IMAGE_OUTPUT_OPTION_CASES (clp, image_params);

	// Generic options
	//
	CMDLINEPARSER_GENERAL_OPTION_CASES (clp);
      }

  // Parse scene spec (filename or test-scene name)
  //
  CMDLINEPARSER_CATCH
    (clp, scene_def.parse (clp, clp.num_remaining_args() - 1));

  // Output filename
  //
  string file_name = clp.get_arg();


  // Start of "overall elapsed" time
  //
  Timeval beg_time (Timeval::TIME_OF_DAY);


  //
  // Define the scene
  //

  Rusage scene_beg_ru;		// start timing scene definition

  Scene scene;
  Camera camera;

  // Default camera aspect ratio to give pixels a 1:1 aspect ratio
  //
  camera.set_aspect_ratio ((float)width / (float)height);

  // Read in the scene/camera definitions
  //
  CMDLINEPARSER_CATCH (clp, scene_def.load (scene, camera));

  Rusage scene_end_ru;		// stop timing scene definition


  // To avoid annoying artifacts in cases where the camera is looking
  // exactly along an axis, always perturb the camera position just a
  // litttle bit.
  //
  camera.move (Vec (Eps, Eps, Eps));


  // Set our drawing limits based on the scene size
  //
  unsigned limit_x = limit_x_spec.apply (clp, width);
  unsigned limit_y = limit_y_spec.apply (clp, height);
  unsigned limit_width
    = limit_max_x_spec.apply (clp, width, limit_x) - limit_x;
  unsigned limit_height
    = limit_max_y_spec.apply (clp, height, limit_y) - limit_y;

  // Create output image.  The size of what we output is the same as the
  // limit (which defaults to, but is not the same as the nominal output
  // image size).
  //
  // This will produce diagnostics and exit if there's something wrong
  // with IMAGE_SINK_PARAMS or a problem creating the output image, so
  // we create the image before printing any normal output.
  //
  ImageOutput output (file_name, limit_width, limit_height, image_params);

  // Maybe print lots of useful information
  //
  if (! quiet)
    {
      print_scene_info (scene, scene_def);
      print_image_info (image_params, render_params, width, height,
			limit_x, limit_y, limit_width, limit_height);
      cout << endl; // blank line
    }


  TraceStats trace_stats;

  // Create the image.
  //
  Rusage render_beg_ru;
  render (scene, camera, width, height, output, limit_x, limit_y,
	  render_params, trace_stats, cout, verbosity);
  Rusage render_end_ru;


  Timeval end_time (Timeval::TIME_OF_DAY);


  // Print stats
  //
  if (! quiet)
    {
      trace_stats.print (cout, scene);

      long num_eye_rays = limit_width * limit_height;

      // a field width of 14 is enough for over a year of time...
      cout << "Time:" << endl;
      Timeval scene_def_time
	= ((scene_end_ru.utime() - scene_beg_ru.utime())
	   + (scene_end_ru.stime() - scene_beg_ru.stime()));
      cout << "  scene def cpu:" << setw (14) << scene_def_time << endl;
      Timeval render_time = render_end_ru.utime() - render_beg_ru.utime();
      cout << "  rendering cpu:" << setw (14) << render_time << endl;
      Timeval elapsed_time = end_time - beg_time;
      cout << "  total elapsed:" << setw (14) << elapsed_time << endl;

      long long sc  = trace_stats.scene_intersect_calls;
      long long sst = trace_stats.scene_shadow_tests;
      double rps = (double)(sc + sst) / render_time;
      double erps = (double)num_eye_rays / render_time;
      cout << "  rays per second:    " << setw (8) << commify ((long long)rps)
	   << endl;
      cout << "  eye-rays per second:" << setw (8) << commify ((long long)erps)
	   << endl;
    }
}


// arch-tag: adea1df7-f224-4a25-9856-7959d7674435
