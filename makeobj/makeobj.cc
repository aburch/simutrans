#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../simdebug.h"
#include "../simtypes.h"
#include "../simversion.h"
#include "../utils/simstring.h"
#include "../besch/writer/obj_pak_exception.h"
#include "../besch/writer/root_writer.h"
#include "../besch/writer/image_writer.h"

// Needed to avoid linking problems
unsigned long dr_time(void)
{
	return 0;
}

int main(int argc, char* argv[])
{
	argv++, argc--;

	init_logging("stderr", true, true, "Makeobj version " MAKEOBJ_VERSION " for Simutrans " VERSION_NUMBER " and higher\n");

	if (argc && !STRICMP(argv[0], "quiet")) {
		argv++, argc--;
	} else {
		puts( "\nMakeobj version " MAKEOBJ_VERSION " for Simutrans " VERSION_NUMBER " and higher\n" );
		puts( "(c) 2002-2012 V. Meyer, Hj. Malthaner, M. Pristovsek & Simutrans development team\n" );
	}

	if (argc && !STRICMP(argv[0], "capabilities")) {
		argv++, argc--;
		root_writer_t::instance()->capabilites();
		return 0;
	}

	if (argc && !STRICMP(argv[0], "pak")) {
		argv++, argc--;

		try {
			const char* dest;
			if (argc) {
				dest = argv[0];
				argv++, argc--;
			} else {
				dest = "./";
			}
			root_writer_t::instance()->write(dest, argc, argv);
		}
		catch (const obj_pak_exception_t& e) {
			fprintf(stderr, "ERROR IN CLASS %s: %s\n", e.get_class(), e.get_info());
			return 1;
		}
		return 0;
	}

	if (argc && STRNICMP(argv[0], "pak", 3) == 0) {
		int img_size = atoi(argv[0] + 3);

		if (img_size >= 16 && img_size < 32766) {
			printf("Image size is set to %dx%d\n", img_size, img_size);

			image_writer_t::set_img_size(img_size);

			argv++, argc--;

			try {
				const char* dest;
				if (argc) {
					dest = argv[0];
					argv++, argc--;
				} else {
					dest = "./";
				}
				root_writer_t::instance()->write(dest, argc, argv);
			}
			catch (const obj_pak_exception_t& e) {
				fprintf(stderr, "ERROR IN CLASS %s: %s\n", e.get_class(), e.get_info());
				return 1;
			}

			// image_writer_t::dump_special_histogramm();
			return 0;
		}
	}

	if (argc > 1) {
		if (!STRICMP(argv[0], "dump")) {
			argv++, argc--;
			root_writer_t::instance()->dump(argc, argv);
			return 0;
		}
		if (!STRICMP(argv[0], "list")) {
			argv++, argc--;
			root_writer_t::instance()->list(argc, argv);
			return 0;
		}
		if (!STRICMP(argv[0], "extract")) {
			argv++, argc--;
			root_writer_t::instance()->uncopy(argv[0]);
			return 0;
		}
		if (!STRICMP(argv[0], "merge")) {
			argv++, argc--;
			try {
				const char* dest = argv[0];
				argv++, argc--;
				root_writer_t::instance()->copy(dest, argc, argv);
			}
			catch (const obj_pak_exception_t& e) {
				fprintf(stderr, "ERROR IN CLASS %s: %s\n", e.get_class(), e.get_info());
				return 1;
			}
			return 0;
		}
	}

	puts(
		"\n   Usage:\n"
		"      MakeObj CAPABILITIES\n"
		"         Gives the list of objects, this program can read\n"
		"      MakeObj PAK <pak file> <dat file(s)>\n"
		"         Creates a ready to use pak file for Simutrans from the dat files\n"
		"      MakeObj pak128 <pak file> <dat file(s)>\n"
		"         Creates a special pak file for with 128x128 images\n"
		"         Works with PAK16 up to PAK255 but only 32 to 192 are tested\n"
		"      MakeObj LIST <pak file(s)>\n"
		"         Lists the contents ot the given pak files\n"
		"      MakeObj DUMP <pak file> <pak file(s)>\n"
		"         List the internal nodes of a file\n"
		"      MakeObj MERGE <pak file library> <pak file(s)>\n"
		"         Merges multiple pak files into one new pak file library\n"
		"      MakeObj EXTRACT <pak file archieve>\n"
		"         Creates single files from a pak file library\n"
		"\n"
		"      with QUIET as first arg copyright message will be omitted\n"
		"      with a trailing slash a direcory is searched rather than a file\n"
		"      default for PAK is PAK ./ ./\n"
#if 0
		"      MakeObj DUMP <pak file(s)>\n"
		"         Dumps the node structure of the given pak files\n"
#endif
	);

	return 3;
}
