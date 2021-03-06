-- snogray.lua -- Top-level driver for snogray
--
--  Copyright (C) 2012, 2013  Miles Bader <miles@gnu.org>
--
-- This source code is free software; you can redistribute it and/or
-- modify it under the terms of the GNU General Public License as
-- published by the Free Software Foundation; either version 3, or (at
-- your option) any later version.  See the file COPYING for more details.
--
-- Written by Miles Bader <miles@gnu.org>
--

local cmdline = ...

local default_width, default_height = 720, 480


----------------------------------------------------------------
-- Imports

local string = require 'snogray.string'
local commify = string.commify
local commify_with_units = string.commify_with_units
local round_and_commify = string.round_and_commify
local lpad = string.left_pad

local file = require 'snogray.file'
local table = require 'snogray.table'

local clp = require 'snogray.cmdlineparser'

local load = require 'snogray.load'
local render = require 'snogray.render'
local image = require 'snogray.image'
local sys = require 'snogray.sys'
local camera = require 'snogray.camera'
local environ = require 'snogray.environ'
local surface = require 'snogray.surface'

local img_out_cmdline = require 'snogray.image-sampled-output-cmdline'
local render_cmdline = require 'snogray.render-cmdline'
local scene_cmdline = require 'snogray.scene-cmdline'
local camera_cmdline = require 'snogray.camera-cmdline'
local limit_cmdline = require 'snogray.limit-cmdline'


----------------------------------------------------------------
-- Parse command-line options
--

-- option values
--
local quiet = false
local progress = true
local recover = false
local num_threads = sys.num_cores ()
local render_params = {}
local scene_params = {}
local camera_params = {}
local output_params = {}

-- Pre/post-loaded things.  Each element is a table with a field
-- 'action' describing how to process it, and any other fields
-- containing the data to process.
--
-- Currently supported pre/post-load actions are:
--
--   'load'   Load the scene file whose name is in the .filename field
--
--   'eval'   Evaluate the Lua code string in the .code field, in the
--            Lua scene-loading environment
--
local preloads, postloads = {}, {}

-- All parameters together.
--
local params = { render = render_params, scene = scene_params,
		 camera = camera_params, output = output_params }


-- Print information about how snogray was built, and exit.
--
local function print_build_info_and_exit ()
   local max_key_len = 0
   local keys = {}
   for k,v in pairs (environ.build_info) do
      keys[#keys+1] = k
      if #k > max_key_len then
	 max_key_len = #k
      end
   end

   table.sort (keys)

   for i = 1, #keys do
      local k = keys[i]
      local v = environ.build_info[k]
      print (string.right_pad (k, max_key_len + 2)..v)
   end

   os.exit (0)
end


-- Command-line parser.
--
local parser = clp.standard_parser {
   desc = "Ray-trace an image",
   usage = "SCENE_FILE [OUTPUT_IMAGE_FILE]",
   prog_name = cmdline[0],
   package = "snogray",
   version = environ.version,

   -- sub-parsers
   --
   "Rendering options:",
   render_cmdline.option_parser (render_params),

   "Scene options:",
   scene_cmdline.option_parser (scene_params),

   { "--preload=SCENE_FILE",
     function (arg) table.insert (preloads, {action='load', filename=arg}) end,
     doc = [[Load scene-file SCENE_FILE before main scene]] },
   { "--postload=SCENE_FILE",
     function (arg) table.insert (postloads, {action='load', filename=arg}) end,
     doc = [[Load scene-file SCENE_FILE after main scene]] },
   { "--pre-eval=LUA_CODE",
     function (arg) table.insert (preloads, {action='eval', code=arg}) end,
     doc = [[Evaluate Lua code LUA_CODE before main scene]] },
   { "--post-eval=LUA_CODE",
     function (arg) table.insert (postloads, {action='eval', code=arg})end,
     doc = [[Evaluate Lua code LUA_CODE after main scene]] },

   "Camera options:",
   camera_cmdline.option_parser (camera_params),

   "Output image options:",
   img_out_cmdline.option_parser (output_params),
   limit_cmdline.option_parser (params),

   -- Misc options; we put these last as they're rarely used.
   --
   "Misc options:",
   { "-C/--continue", function () recover = true end,
     doc = [[Continue a previously aborted render]] },
   { "-j/--threads=NUM",
     function (num) num_threads = clp.unsigned_argument (num) end,
     doc = [[Use NUM threads for rendering (default all cores)]] },
   --{ "limit",		required_argument, 0, 'L' },
   { "-q/--quiet", function () quiet = true end,
     doc = [[Do not output informational or progress messages]] },
   { "-p/--progress", function () progress = true end, hidden = true,
     doc = [[Output progress indicator despite --quiet]] },
   { "-P/--no-progress", function () progress = false end,
     doc = [[Do not output progress indicator]] },
   { "--build-info", print_build_info_and_exit,
     doc = [[Print information about how snogray was built]] }
}


local args = parser (cmdline)

if #args < 1 or #args > 2 then
   parser:usage_error ()
end

local scene_file = args[1]
local output_file = args[2] -- may be null


----------------------------------------------------------------
-- Helper functions
--

-- Copy the contents of FROM to TO, except where TO has an existing
-- entry.  Table values are recursively installed in the same manner.
--
local function install_new_entries (from, to)
   if type (from) == 'table' and type (to) == 'table' then
      for k, v in pairs (from) do
	 local existing = to[k]
	 if not existing then
	    to[k] = v
	 elseif type (v) == 'table' then
	    install_new_entries (v, existing)
	 end
      end
   end
end


----------------------------------------------------------------
-- Load the scene
--

local beg_time = os.time ()
local scene_beg_ru = sys.rusage () -- begin marker for scene loading

local scene = surface.group ()
local camera = camera.new ()  	-- note, shadows variable, but oh well

-- Stick the output filenamein OUTPUT_PARAMS, so the scene loader can
-- see it (and optionally, change it).
--
output_params.filename = output_file

-- We set up the camera's nominal state before loading the scene so
-- that the scene can see it.  Mostly the defaults are OK, but the
-- output-image dimensions can affect the aspect-ratio.
--
camera:set_aspect_ratio ((output_params.width or default_width)
		         / (output_params.height or default_height))


--------
-- Set up the scene environment, which is just a Lua environment into
-- which all scene files are loaded.
--------

local coord = require 'snogray.coord'

-- The Lua environment passed to the scene loader(s).  All globals
-- defined in loaded files will end up there, and we initialize it
-- with various useful things for scene definition.
--
local load_environ = {
   -- References to the scene object and the camera.  The actual scene
   -- is defined in them.
   --
   scene = scene,
   camera = camera,

   -- A copy of our parameters.  New entries will be copied back after
   -- the scene is loaded, but changes to existing entries will be
   -- ignored.
   --
   params = table.deep_copy (params),

   -- Lua modules commonly used in scene definitions.  Scene files may
   -- also use the Lua 'require' function to explicitly load Lua
   -- modules.
   --
   coord = coord,
   light = require 'snogray.light',
   material = require 'snogray.material',
   surface = require 'snogray.surface',
   texture = require 'snogray.texture',
   transform = require 'snogray.transform',

   -- Shortcuts for commonly used functions (these are also available
   -- as entries in appropriate modules, but these are so freqeuently
   -- used, it's nice to have shorter names).
   --
   pos = coord.pos, vec = coord.vec
}
-- inherit from the default global environment
setmetatable (load_environ, {__index = _G})


-- Snogray Lua scene files used to use a different interface where
-- everything was just plunked down in a single namespace.  The
-- following special function inserted into the load-environment,
-- tries to set things up to emulate that for compatibility.
--
-- A old-style scene file can be made compatible simply by putting a
-- call to "snogray_old_style_scene_file ()" at its beginning.
--
function load_environ.snogray_old_style_scene_file ()
   -- Only execute this function once.
   --
   if not load_environ._old_style_scene_file then
      local filename = require 'snogray.filename'

      -- Remember that we've already installed the compatibility hooks.
      --
      load_environ._old_style_scene_file = true

      -- Inherit from the "all-in-one" scene interface for backward
      -- compatibility.
      --
      setmetatable (load_environ, { __index = require 'snogray.all-in-one' })

      local function with_lua_ext (file)
	 if not filename.extension (file) then
	    file = file..".lua"
	 end
	 return file
      end

      -- Add specialized file-loading functions that know what
      -- environment to use.
      --
      function load_environ.include (file)
	 load.scene (with_lua_ext (file), load_environ)
      end

      load_environ._used_files = {}
      function load_environ.use (file)
	 file = with_lua_ext (file)
	 if not load_environ._used_files[file] then
	    load.scene (file, load_environ)
	    load_environ._used_files[file] = true
	 end
      end
   end
end


--------
-- Do the actual loading, calling the loader to load the main scene,
-- and any pre- or post-loaded scene files (which are loaded into the
-- same environment, and so are "additive").
--------

local function do_pre_post_loads (entries, which)
   for i, entry in ipairs (entries) do
      if entry.action == 'load' then
	 -- Load a scene file.
	 load.scene (entry.filename, load_environ)
      else
	 -- Evaluate some Lua code.  We've used the variable "load" for
	 -- a module, so get the original Lua load function from _G.
	 --
	 local thunk, err = _G.load (entry.code, nil, nil, load_environ)
	 if thunk then
	    thunk ()
	 else
	    error ("error in --"..which.."-eval: "..err, 0)
	 end
      end
   end
end

-- pre-loaded scene files/statements
do_pre_post_loads (preloads, "pre")

-- main scene file
load.scene (scene_file, load_environ)

-- post-loaded scene files/statements
do_pre_post_loads (postloads, "post")


--------
-- Do post-loading processing.  Most of the scene definition is
-- already present in the scene object, but we look for parameter
-- changes etc which we need to deal with explicitly.
--------

-- Update our local idea of the scene and camera objects in case the
-- loader changed them (usually it won't, but it can).
--
scene = load_environ.scene
camera = load_environ.camera

-- Copy back any _new_ parameters added the loader's copy of the scene
-- parametes to PARAMS.
--
-- Keeping existing params in PARAMS ensures that any parameters the
-- user specified on the command-line override parameters set by the
-- loader.
--
install_new_entries (load_environ.params, params)

-- Apply parameter tables.
--
scene_cmdline.apply (scene_params, scene)

-- Get the output file back, in case loading the scene changed it.
--
output_file = output_params.filename

-- We're done copying stuff out, so zero out the load environment to
-- let the garbage collector have a shot at any cruft in there.
--
load_environ = nil

-- The output file is optional on the command-line, but must be supplied
-- by the scene if not there.
--
if not output_file then
   error ("no output file specified", 0)
end

local scene_end_ru = sys.rusage () -- end marker for scene setup


----------------------------------------------------------------
-- Post-loading setup
--

-- Do final setup of image dimensions (it can't be done until we've
-- loaded the scene, as the scene can change things).
--
img_out_cmdline.finalize_dimensions (output_params,
				     default_width, default_height)

-- The nominal image size.
--
local width, height = output_params.width, output_params.height

-- Give camera correct aspect-ratio based on the output image dimensions.
--
camera:set_aspect_ratio (width / height)

-- Evaluate camera command-line commands.
--
camera_cmdline.apply (camera_params, camera, scene)


----------------------------------------------------------------
-- Setup the output file
--


-- If rendering with an alpha-channel, make sure the output handles it too.
--
if render_params.background_alpha and render_params.background_alpha ~= 1 then
   output_params.alpha_channel = true
end


-- If we're in recovery mode, move an existing output file out of the
-- way; otherwise an existing output file is an error (to prevent
-- accidental overwriting).
--
local recover_backup = nil
if file.exists (output_file) then
   if recover then
      recover_backup = file.rename_to_backup_file (output_file, 99)
      if not quiet then
	 print ("* recover: "..output_file..": Backup in "..recover_backup)
      end
   else
      error (output_file..": Output file already exists\n"
	     .."To continue a previously aborted render, use the `--continue' option", 0)
   end
end


-- The "limit", which is the portion the nominal image which we will
-- actually render.
--
local limit_x, limit_y, limit_width, limit_height
   = limit_cmdline.bounds (params, width, height)

-- Make the output image reflect the limit rather than the "nominal"
-- image size.
--
output_params.sample_base_x = limit_x
output_params.sample_base_y = limit_y
output_params.width = limit_width
output_params.height = limit_height

-- Create the output.
--
local image_out = img_out_cmdline.make_output (output_file, output_params)

if output_params.alpha_channel and not image_out:has_alpha_channel () then
   error (output_file..": alpha-channel not supported", 0)
end

-- If recovering, do the actual recovery.
--
if recover_backup then
   local num_rows_recovered
      = image.recover (recover_backup, output_file, output_params, image_out)

   if not quiet then
      print ("* recover: "..output_file..": Recovered "
	     ..tostring(num_rows_recovered).." rows")
   end

   if num_rows_recovered == limit_height then
      print (output_file..": Entire image was recovered, not rendering")
      return
   end

   -- Remove the recovered rows from what we will render.
   --
   limit_y = limit_y + num_rows_recovered
   limit_height = limit_height - num_rows_recovered
end


----------------------------------------------------------------
-- Pre-render info output
--

if not quiet then
   local surf_stats = scene:stats ()
   local scene_out = "* scene: "
   scene_out = scene_out..commify_with_units (surf_stats.num_render_surfaces,
					      " surface", true)
   if surf_stats.num_render_surfaces ~= surf_stats.num_real_surfaces then
      scene_out
	 = scene_out.." ("..commify (surf_stats.num_real_surfaces).." real)"
   end
   scene_out
      = (scene_out..", "
	 ..commify_with_units (surf_stats.num_lights, " light", true))
   print (scene_out)

   print ("* camera: at "..tostring (camera.pos)
	  ..", pointing at "
          ..tostring (camera.pos + camera.forward * camera.target_dist)
	  .." (up = "..tostring(camera.up)
	  ..", right = "..tostring(camera.right)..")")
   local cam_desc = "focal-length "..round_and_commify(camera:focal_length())
   if camera.aperture ~= 0 then
      cam_desc = cam_desc..", f-stop f:"..round_and_commify (camera:f_stop())
                 ..", focus distance "..round_and_commify (camera.focus)
   end
   print ("* camera: "..cam_desc)

   print ("* output: file "..output_file)
   print ("* output: size "..image_out.width.."x"..image_out.height
          ..", "..(image_out:has_alpha_channel() and "with" or "no")
          .." alpha-channel") 

   print ("* using "
	  ..commify_with_units(num_threads, " thread", true)
          .." and "
	  ..commify_with_units(render_params.samples or 1, " sample", true)
          .."/pixel")
end


----------------------------------------------------------------
-- Pre-render setup (this can be time-consuming)
--

local setup_beg_ru = sys.rusage ()
local grstate = render_cmdline.make_global_render_state (scene, render_params)
local setup_end_ru = sys.rusage ()


----------------------------------------------------------------
-- Rendering
--

-- The pattern of pixels we will render; we add a small margin around
-- the output image to keep the edges clean.
--
local x_margin = image_out:filter_x_radius ()
local y_margin = image_out:filter_y_radius ()
local render_pattern
   = render.pattern (limit_x - x_margin, limit_y - y_margin,
		     limit_width + x_margin * 2, limit_height + y_margin * 2)

local render_stats = render.stats ()
local render_mgr = render.manager (grstate, camera, width, height)

local tty_prog = sys.tty_progress ("rendering...")

local render_beg_ru = sys.rusage () -- begin marker for rendering

render_mgr:render (num_threads, render_pattern, image_out,
		   tty_prog, render_stats)

local render_end_ru = sys.rusage () -- end marker for rendering
local end_time = os.time ()


----------------------------------------------------------------
-- Post-render reporting
--

if not quiet then
   -- Return 100 * (NUM / DEN) as an int; if DEN == 0, return 0.
   --
   local function percent (num, den)
      if den == 0 then return 0 else return math.floor (100 * num / den) end
   end

   -- Return NUM / DEN as a float; if DEN == 0, return 0;
   --
   local function fraction (num, den)
      if den == 0 then return 0 else return num / den end
   end

   local function elapsed_time_string (sec)
      sec = math.floor (sec * 10 + 0.5) / 10

      local min = math.floor (sec / 60)
      sec = sec - min * 60
      local hours = math.floor (min / 60)
      min = min - hours * 60
      local days = math.floor (hours / 24)
      hours = hours - days * 24

      local rval = nil
      if days ~= 0 then
	 rval = string.sep_concat (rval, ", ", days).." days"
      end
      if hours ~= 0 then
	 rval = string.sep_concat (rval, ", ", hours).." hours"
      end
      if min ~= 0 then
	 rval = string.sep_concat (rval, ", ", min).." min"
      end
      if sec ~= 0 then
	 rval = string.sep_concat (rval, ", ", sec).." sec"
      end

      return rval
   end

   -- Print post-rendering scene statistics in RENDER_STATS.
   --
   local function print_render_stats (rstats)
      local function print_search_stats (sstats)
	 local node_tests = sstats.space_node_intersect_calls
	 local surf_tests = sstats.surface_intersects_tests
	 local neg_cache_hits = sstats.neg_cache_hits
	 local neg_cache_colls = sstats.neg_cache_collisions
	 local tot_tries = surf_tests + neg_cache_hits
	 local pos_tries = sstats.surface_intersects_hits

	 print("     tree node tests: "..lpad (commify (node_tests), 16))
	 print("     surface tests:   "..lpad (commify (tot_tries), 16)
	       .." (success = "..lpad(percent(pos_tries, tot_tries), 2).."%"
	       ..", cached = "..lpad(percent(neg_cache_hits, tot_tries), 2).."%"
	       .."; coll = "..lpad(percent(neg_cache_colls, tot_tries), 2).."%)")
      end

      local sic = rstats.scene_intersect_calls
      local sst = rstats.scene_shadow_tests

      print ""
      print "Rendering stats:"
      print "  intersect:"
      print("     rays:            "..lpad (commify (sic), 16))

      print_search_stats (rstats.intersect)

      if sst ~= 0 then
	 print "  shadow:"
	 print("     rays:            "..lpad (commify (sst), 16))

	 print_search_stats (rstats.shadow)
      end

      local ic = rstats.illum_calls
      if ic ~= 0 then
	 print "  illum:"
	 print("     illum calls:     "..lpad (commify (ic), 16))
	 if sst ~= 0 then
	    print("     average shadow rays:   "
		  ..lpad (round_and_commify (fraction (sst, ic), 3), 10))
	 end
      end
   end

   -- Print the amount of CPU time used between BEG_RU and END_RU,
   -- prefix with LABEL.
   --
   local function print_cpu_time (label, beg_ru, end_ru)
      -- Because scene-loading often involves significant disk I/O, we
      -- report system time as well (this usually isn't a factor for
      -- other time periods we measure).
      --
      local user_cpu_time = end_ru:user_cpu_time() - beg_ru:user_cpu_time()
      local user_cpu_time_str = elapsed_time_string (user_cpu_time)
      local sys_cpu_time = end_ru:sys_cpu_time() - beg_ru:sys_cpu_time()
      local sys_cpu_time_str = elapsed_time_string (sys_cpu_time)

      if user_cpu_time_str or sys_cpu_time_str then
	 if sys_cpu_time_str then
	    sys_cpu_time_str = "(system "..sys_cpu_time_str..")"
	 end
	 print(label..string.sep_concat (user_cpu_time_str, ' ', sys_cpu_time_str))
      end
   end

   print_render_stats (render_stats)

   --
   -- Print times; a field width of 14 is enough for over a year of
   -- time...
   --

   print "Time:"

   print_cpu_time ("  scene def cpu:       ", scene_beg_ru, scene_end_ru)
   print_cpu_time ("  setup cpu:           ", setup_beg_ru, setup_end_ru)
   print_cpu_time ("  rendering cpu:       ", render_beg_ru, render_end_ru)

   local real_time = os.difftime (end_time, beg_time)
   print("  total elapsed:       "..(elapsed_time_string (real_time) or "0"))

   local maxrss = sys.rusage():max_rss ()
   print("  max working set:     "..commify_with_units (maxrss/1024, " MB"))

   local sic = render_stats.scene_intersect_calls
   local sst = render_stats.scene_shadow_tests
   local num_eye_rays = limit_width * limit_height

   local rps, erps = 0, 0
   local render_time
      = render_end_ru:user_cpu_time() - render_beg_ru:user_cpu_time()
   if render_time ~= 0 then
      rps = (sic + sst) / render_time
      erps = (num_eye_rays) / render_time
   end
   local function fmt_rps_str (rps)
      if rps < 10 then
	 return round_and_commify (rps, 1)
      else
	 return commify (rps)
      end
   end
   local rps_str = fmt_rps_str (rps)
   print("  rays per second:     "..rps_str)
   print("  eye-rays per second: "..lpad (fmt_rps_str (erps), #rps_str))
end


--
-- ~fini~
--
