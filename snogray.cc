// snogray.cc -- Main driver for snogray ray tracer
//
//  Copyright (C) 2005  Miles Bader <miles@gnu.org>
//
// This file is subject to the terms and conditions of the GNU General
// Public License.  See the file COPYING in the main directory of this
// archive for more details.
//
// Written by Miles Bader <miles@gnu.org>
//

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

#include "cmdlineparser.h"

#include "scene.h"
#include "camera.h"
#include "light.h"
#include "image.h"
#include "image-cmdline.h"

using namespace Snogray;
using namespace std;

// This defines a test scene
extern void test_scene (Scene &scene, Camera &camera, unsigned scene_num);



// LimitSpec datatype

// A "limit spec" represents a user's specification of a single
// rendering limit (on x, y, width, or height), which may be absolute,
// relative, a proportion of the image size, or both.
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
	      spec += strspn (spec, ", ");
	      ok = limit_max_y_spec.parse (spec);
	    }
	}
    }

  if (!ok || *spec != '\0')
    clp.opt_err ("requires a limit specification (X,Y[+-]W,H)");
}


// Random string helper functions

// Return a string version of NUM
static string 
stringify (unsigned num)
{
  string str = (num > 9) ? stringify (num / 10) : "";
  char ch = (num % 10) + '0';
  str += ch;
  return str;
}

// Return a string version of NUM, with commas added every 3rd place
static string 
commify (unsigned long long num, unsigned sep_count = 1)
{
  string str = (num > 9) ? commify (num / 10, sep_count % 3 + 1) : "";
  char ch = (num % 10) + '0';
  if (sep_count == 3 && num > 9)
    str += ',';
  str += ch;
  return str;
}


// Main driver

static void
usage (CmdLineParser &clp, ostream &os)
{
  os << "Usage: " << clp.prog_name()
     << "[OPTION...] [OUTPUT_IMAGE_FILE]" << endl;
}

static void
help (CmdLineParser &clp, ostream &os)
{
  usage (clp, os);
  os << "Ray-trace an image" << endl
     << endl
<< "  -w, --width=PIXELS         Set image width to PIXELS wide" << endl
<< "  -h, --height=LINES         Set image height to LINES high" << endl
<< "  -s, --size=WIDTHxHEIGHT    Set image size to WIDTH x HEIGHT pixels/lines"
     << endl
<< "  -l, --limit=LIMIT_SPEC     Limit output to area defined by LIMIT_SPEC"
     << endl
     << endl
<< "  -q, --quiet                Do not output informational or progress messages"
     << endl
<< "  -P, --no-progress          Do not output progress indicator" << endl
<< "  -p, --progress             Output progress indicator despite --quiet"
     << endl
     << endl
<< "  -t, --test-scene=NUM       Render test scene NUM" << endl
     << endl
     << IMAGE_OUTPUT_OPTIONS_HELP << endl
     << endl
     << CMDLINEPARSER_GENERAL_OPTIONS_HELP << endl
     << endl
     << "If no filename is given, standard output is used.  The output"  << endl
     << "image format is guessed using the output filename when possible"<< endl
     << "(using the file's extension)." << endl;
}

int main (int argc, char *const *argv)
{
  // Command-line option specs
  static struct option long_options[] = {
    { "width",		required_argument, 0, 'w' },
    { "height",		required_argument, 0, 'h' },
    { "size",		required_argument, 0, 's' },
    { "limit",		required_argument, 0, 'l' },
    { "quiet",		no_argument,	   0, 'q' },
    { "progress",	no_argument,	   0, 'p' },
    { "no-progress",	no_argument,	   0, 'P' },
    { "test-scene", 	required_argument, 0, 't' },
    IMAGE_OUTPUT_LONG_OPTIONS,
    CMDLINEPARSER_GENERAL_LONG_OPTIONS,
    { 0, 0, 0, 0 }
  };
  char short_options[] =
    "w:h:s:l:qpPt:"
    IMAGE_OUTPUT_SHORT_OPTIONS
    IMAGE_INPUT_SHORT_OPTIONS
    CMDLINEPARSER_GENERAL_SHORT_OPTIONS;
  //
  CmdLineParser clp (argc, argv, short_options, long_options);

  // Parameters set from the command line
  unsigned final_width = 640, final_height = 480;
  LimitSpec limit_x_spec ("min-x", 0), limit_y_spec ("min-y", 0);
  LimitSpec limit_max_x_spec ("max-x", 1.0), limit_max_y_spec ("max-y", 1.0);
  bool quiet = false, progress = true; // quiet mode, progress indicator
  bool progress_set = false;
  unsigned test_scene_num = 0;
  ImageCmdlineSinkParams image_sink_params (clp);

  // Parse command-line options
  int opt;
  while ((opt = clp.get_opt ()) > 0)
    switch (opt)
      {
	// Verbosity options
      case 'q':
	quiet = true;
	if (! progress_set)
	  progress = false;
	break;
      case 'p':
	progress = true;
	progress_set = true;
	break;
      case 'P':
	progress = false;
	progress_set = true;
	break;

	// Size options
      case 's':
	parse_size_opt_arg (clp, final_width, final_height);
	break;
      case 'l':
	parse_limit_opt_arg (clp, limit_x_spec, limit_y_spec,
			     limit_max_x_spec, limit_max_y_spec);
	break;
      case 'w':
	final_width = clp.unsigned_opt_arg ();
	break;
      case 'h':
	final_height = clp.unsigned_opt_arg ();
	break;

      case 't':
	test_scene_num = clp.unsigned_opt_arg ();
	break;

	IMAGE_OUTPUT_OPTION_CASES (clp, image_sink_params);

	CMDLINEPARSER_GENERAL_OPTION_CASES (clp);
      }

  // Final output file parameter
  if (clp.num_remaining_args() > 1)
    {
      usage (clp, cerr);
      cerr << "Try `" << clp.prog_name() << " --help' for more information"
	   << endl;
      exit (10);
    }
  image_sink_params.file_name = clp.get_arg();

  // We reference this a lot below, so make a local copy
  unsigned aa_factor = image_sink_params.aa_factor;
  if (aa_factor == 0)
    aa_factor = 1;

  // The size of the actual full final image
  const unsigned width = final_width * aa_factor;
  const unsigned height = final_height * aa_factor;

  // Set our drawing limits based on the scene size
  unsigned limit_x = limit_x_spec.apply (clp, final_width);
  unsigned limit_y = limit_y_spec.apply (clp, final_height);
  unsigned limit_width
    = limit_max_x_spec.apply (clp, final_width, limit_x) - limit_x;
  unsigned limit_height
    = limit_max_y_spec.apply (clp, final_height, limit_y) - limit_y;

  // The size of what we actually output is the same as the limit
  image_sink_params.width = limit_width;
  image_sink_params.height = limit_height;

  // Create output image
  ImageOutput image (image_sink_params);

  // Print image info
  if (! quiet)
    {
      cout << "Image:" << endl;
      cout << "   size:         "
	   << setw (11) << (stringify (final_width)
			    + " x " + stringify (final_height))
	   << endl;

      if (limit_x != 0 || limit_y != 0
	  || limit_width != final_width || limit_height != final_height)
	cout << "   limit:"
	     << setw (19) << (stringify (limit_x) + "," + stringify (limit_y)
			      + " - " + stringify (limit_x + limit_width)
			      + "," + stringify (limit_y + limit_height))
	     << " (" << limit_width << " x "  << limit_height << ")"
	     << endl;

      if (image_sink_params.target_gamma != 0)
        cout << "   target_gamma:        "
	     << setw (4) << image_sink_params.target_gamma << endl;

      // Anti-aliasing info
      if ((aa_factor + image_sink_params.aa_overlap) > 1)
	{
	  if (aa_factor > 1)
	    cout << "   aa_factor:           "
		 << setw (4) << aa_factor << endl;

	  if (image_sink_params.aa_overlap > 0)
	    cout << "   aa_kernel_size:      "
		 << setw (4)
		 << (aa_factor + image_sink_params.aa_overlap*2)
		 << " (overlap = " << image_sink_params.aa_overlap << ")"
		 << endl;
	  else
	    cout << "   aa_kernel_size:      "
		 << setw (4) << aa_factor << endl;

	  cout << "   aa_filter:       " << setw (8);
	  if (image_sink_params.aa_filter == ImageOutput::aa_box_filter)
	    cout << "box";
	  else if (image_sink_params.aa_filter == ImageOutput::aa_triang_filter)
	    cout << "triang";
	  else if (image_sink_params.aa_filter == ImageOutput::aa_gauss_filter
		   || !image_sink_params.aa_filter)
	    cout << "gauss";
	  else
	    cout << "???";
	  cout << endl;
	}
    }

  // 
  Scene scene;
  Camera camera;

  // Set camera aspect ratio to give pixels a 1:1 aspect ratio
  camera.set_aspect_ratio ((float)width / (float)height);

  // Define our scene!
  test_scene (scene, camera, test_scene_num);

  // Print scene info
  if (! quiet)
    {
      cout << "Scene:" << endl;
      cout << "   objects:       "
	   << setw (10) << commify (scene.objs.size ()) << endl;
      cout << "   lights:        "
	   << setw (10) << commify (scene.lights.size ()) << endl;
      cout << "   materials:     "
	   << setw (10) << commify (scene.materials.size ()) << endl;
      cout << "   voxtree nodes: "
	   << setw (10) << commify (scene.obj_voxtree.num_nodes ()) << endl;
      cout << "   voxtree max depth:"
	   << setw (7) << commify (scene.obj_voxtree.max_depth ()) << endl;
    }

  // Limits in terms of higher-resolution pre-AA image
  unsigned hr_limit_x = limit_x * aa_factor;
  unsigned hr_limit_y = limit_y * aa_factor;
  unsigned hr_limit_max_x = hr_limit_x + limit_width * aa_factor;
  unsigned hr_limit_max_y = hr_limit_y + limit_height * aa_factor;

  if (! quiet)
    cout << endl;

  // Main ray-tracing loop
  for (unsigned y = hr_limit_y; y < hr_limit_max_y; y++)
    {
      ImageRow &output_row = image.next_row ();

      // Progress indicator
      if (progress)
	{
	  if (aa_factor > 1)
	    cout << "\rrendering: line "
		 << setw (5) << y / aa_factor
		 << "_" << (y - (y / aa_factor) * aa_factor);
	  else
	    cout << "\rrendering: line "
		 << setw (5) << y;
	  cout << " (" << (y - hr_limit_y) * 100 / (hr_limit_max_y - hr_limit_y)
	       << "%)";
	  cout.flush ();
	}

      for (unsigned x = hr_limit_x; x < hr_limit_max_x; x++)
	{
	  float u = (float)x / (float)width;
	  float v = (float)(height - y) / (float)height;
	  Ray camera_ray = camera.get_ray (u, v);

	  camera_ray.set_len (10000);

	  output_row[x - hr_limit_x] = scene.render (camera_ray);
	}
    }

  if (progress)
    {
      cout << "\rrendering: done              " << endl;
      if (! quiet)
	cout << endl;
    }

  // Print render stats
  if (! quiet)
    {
      Scene::Stats &sstats = scene.stats;
      Voxtree::Stats &vstats1 = sstats.voxtree_closest_intersect;
      Voxtree::Stats &vstats2 = sstats.voxtree_shadowed;

      cout << "Rendering stats:" << endl;
      cout << "  closest_intersect:" << endl;
      cout << "     scene calls:       "
	   << setw (14) << commify (sstats.scene_closest_intersect_calls)
	   << endl;
      cout << "     voxtree node calls:"
	   << setw (14) << commify (vstats1.node_intersect_calls) << endl;
      cout << "     obj calls:         "
	   << setw (14) << commify (sstats.obj_closest_intersect_calls) << endl;

      cout << "  shadowed:" << endl;
      cout << "     scene tests:       "
	   << setw (14) << commify (sstats.scene_shadowed_tests) << endl;
      cout << "     shadow hint hits:  "
	   << setw (14) << commify (sstats.shadow_hint_hits) << endl;
      cout << "     shadow hint misses:"
	   << setw (14) << commify (sstats.shadow_hint_misses) << endl;
      cout << "     voxtree node tests:"
	   << setw (14) << commify (vstats2.node_intersect_calls) << endl;
      cout << "     obj tests:         "
	   << setw (14) << commify (sstats.obj_intersects_tests) << endl;
    }
}

// arch-tag: adea1df7-f224-4a25-9856-7959d7674435
