/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../simutrans/utils/log.h"

log_t::level_t debuglevel = log_t::LEVEL_WARN;

#include "../simutrans/simdebug.h"
#include "../simutrans/simtypes.h"
#include "../simutrans/simversion.h"
#include "../simutrans/utils/simstring.h"
#include "../simutrans/descriptor/writer/obj_pak_exception.h"
#include "../simutrans/descriptor/writer/root_writer.h"
#include "../simutrans/descriptor/writer/image_writer.h"


// Needed to avoid linking problems
uint32 dr_time(void)
{
	return 0;
}


int main(int argc, char* argv[])
{
	argv++; argc--;

	init_logging("stderr", true, true, "", "makeobj");
	debuglevel = log_t::LEVEL_WARN; // only warnings and errors

	while(  argc  &&  (  !STRICMP(argv[0], "quiet")  ||  !STRICMP(argv[0], "verbose")  ||  !STRICMP(argv[0], "debug")  )  ) {

		if (argc && !STRICMP(argv[0], "debug")) {
			argv++; argc--;
			debuglevel = log_t::LEVEL_DEBUG; // everything
		}
		else if (argc && !STRICMP(argv[0], "verbose")) {
			argv++; argc--;
			debuglevel = log_t::LEVEL_MSG; // only messages errors
		}
		else if (argc && !STRICMP(argv[0], "quiet")) {
			argv++; argc--;
			debuglevel = log_t::LEVEL_ERROR; // only fatal errors
		}
	}

	if(  debuglevel>=log_t::LEVEL_WARN  ) {
		puts( "Makeobj version " MAKEOBJ_VERSION " for Simutrans " VERSION_NUMBER " and higher" );
		puts( "(c) 2002-2012 V. Meyer, Hj. Malthaner, M. Pristovsek & Simutrans development team\n" );
	}

	if (argc && !STRICMP(argv[0], "capabilities")) {
		argv++; argc--;
		root_writer_t::instance()->capabilites();
		return 0;
	}

	if (argc && !STRICMP(argv[0], "pak")) {
		argv++; argc--;

		try {
			const char* dest;
			if (argc) {
				dest = argv[0];
				argv++; argc--;
			}
			else {
				dest = "./";
			}
			root_writer_t::instance()->write(dest, argc, argv);
		}
		catch (const obj_pak_exception_t& e) {
			dbg->error( e.get_class(), e.get_info() );
			return 1;
		}
		return 0;
	}

	if (argc && STRNICMP(argv[0], "pak", 3) == 0) {
		const int img_size = atoi(argv[0] + 3);

		if (img_size >= 16 && img_size < 32766) {
			dbg->message( "Image size", "Now set to %dx%d", img_size, img_size );

			obj_writer_t::set_img_size(img_size);

			argv++; argc--;

			try {
				const char* dest;
				if (argc) {
					dest = argv[0];
					argv++; argc--;
				}
				else {
					dest = "./";
				}
				root_writer_t::instance()->write(dest, argc, argv);
			}
			catch (const obj_pak_exception_t& e) {
				dbg->error( e.get_class(), e.get_info() );
				return 1;
			}

			return 0;
		}
	}

	if (argc && !STRICMP(argv[0], "expand")) {
		argv++; argc--;

		try {
			const char* dest;

			if (argc) {
				dest = argv[0];
				argv++; argc--;
			}
			else {
				dest = "./";
			}

			root_writer_t::instance()->expand_dat(dest, argc, argv);
		}
		catch (const obj_pak_exception_t& e) {
			dbg->error( e.get_class(), e.get_info() );
			return 1;
		}

		return 0;
	}

	if (argc > 1) {
		if (!STRICMP(argv[0], "dump")) {
			argv++; argc--;
			root_writer_t::instance()->dump(argc, argv);
			return 0;
		}
		if (!STRICMP(argv[0], "list")) {
			argv++; argc--;
			root_writer_t::instance()->list(argc, argv);
			return 0;
		}
		if (!STRICMP(argv[0], "extract")) {
			argv++; argc--;
			root_writer_t::instance()->uncopy(argv[0]);
			return 0;
		}
		if (!STRICMP(argv[0], "merge")) {
			argv++; argc--;
			try {
				const char* dest = argv[0];
				argv++; argc--;
				root_writer_t::instance()->copy(dest, argc, argv);
			}
			catch (const obj_pak_exception_t& e) {
				dbg->error( e.get_class(), e.get_info() );
				return 1;
			}
			return 0;
		}
	}

	puts(
		"\n   Usage: MakeObj [QUIET|VERBOSE|DEBUG] <Command> <params>\n"
		"\n"
		"      MakeObj CAPABILITIES\n"
		"         Gives the list of objects, this program can read\n"
		"      MakeObj PAK <pak file> <dat file(s)>\n"
		"         Creates a ready to use pak file for Simutrans from the dat files\n"
		"      MakeObj pak128 <pak file> <dat file(s)>\n"
		"         Creates a special pak file for with 128x128 images\n"
		"         Works with PAK16 up to PAK32767 but only up to 255 are tested\n"
		"      MakeObj LIST <pak file(s)>\n"
		"         Lists the contents of the given pak files\n"
		"      MakeObj DUMP <pak file> <pak file(s)>\n"
		"         List the internal nodes of a file\n"
		"      MakeObj MERGE <pak file library> <pak file(s)>\n"
		"         Merges multiple pak files into one new pak file library\n"
		"      MakeObj EXPAND <output> <dat file(s)>\n"
		"         Expands minified notation to complete notation on dat file(s)\n"
		"      MakeObj EXTRACT <pak file archive>\n"
		"         Creates single files from a pak file library\n"
		"\n"
		"      with a trailing slash a directory is searched rather than a file\n"
		"      default for PAK is PAK ./ ./\n"
		"\n"
		"      with QUIET as first arg status and copyright messages are omitted\n"
		"\n"
		"      with VERBOSE as first arg also unused lines\n"
		"      and unassigned entries are printed\n"
		"\n"
		"      DEBUG dumps extended information about the pak process.\n"
		"          Source: interpreted line from .dat file\n"
		"          Image:  .png file name\n"
		"          X:      X start position in .png to pack\n"
		"          Y:      Y start position in .png to pack\n"
		"          Off X:  X offset in image\n"
		"          Off Y:  Y offset in image\n"
		"          Width:  image width\n"
		"          Width:  image height\n"
		"          Zoom:   If image is zoomable or not\n"
	);

	return 3;
}
