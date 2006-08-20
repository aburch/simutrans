#include <stdio.h>
#include <string.h>
#include "../utils/cstring_t.h"
#include "../dataobj/tabfile.h"

#include "../simtypes.h"
#include "../simversion.h"
#include "../besch/writer/obj_pak_exception.h"

#include "../besch/writer/root_writer.h"
#include "../besch/writer/image_writer.h"


int main(int argc, char* argv[])
{
    argv++, argc--;

    if(argc && !STRICMP(argv[0], "quiet")) {
	argv++, argc--;
    }
    else {
	puts(
	    "\nMakeobj version " MAKEOBJ_VERSION " for simutrans " VERSION_NUMBER " and higher\n"
	    "(c) 2002-2006 V. Meyer , Hj. Malthaner, M. Pristovsek (markus@pristovsek.de)\n");
    }
    if(argc && !STRICMP(argv[0], "capabilities")) {
	argv++, argc--;
	root_writer_t::instance()->capabilites();
	return 0;
    }
    if(argc && !STRICMP(argv[0], "pak")) {
	argv++, argc--;

	try {
	    const char *dest;
	    if(argc) {
		dest = argv[0];
		argv++, argc--;
	    }
	    else {
		dest = "./";
	    }
	    root_writer_t::instance()->write(dest, argc, argv);
	} catch(obj_pak_exception_t *e) {
	    printf("ERROR IN CLASS %s: %s\n", e->get_class().chars(), e->get_info().chars());
	    delete e;
	}
	return 0;
    }
    if(argc && STRNICMP(argv[0],"pak",3)==0) {
	int img_size = atoi(argv[0] + 3);

        if(img_size >= 16 && img_size < 256) {
	    printf("Image size is set to %dx%d\n", img_size, img_size);

	    image_writer_t::set_img_size(img_size);

	    argv++, argc--;

    	    try {
	        const char *dest;
	        if(argc) {
		    dest = argv[0];
		    argv++, argc--;
	        }
	        else {
		    dest = "./";
	        }
	        root_writer_t::instance()->write(dest, argc, argv);
	    } catch(obj_pak_exception_t *e) {
	        printf("ERROR IN CLASS %s: %s\n", e->get_class().chars(), e->get_info().chars());
	        delete e;
	    }


	    // image_writer_t::dump_special_histogramm();
	    return 0;
	}
    }
    if(argc > 1) {
	if(!STRICMP(argv[0], "dump")) {
	    argv++, argc--;
	    root_writer_t::instance()->dump(argc, argv);
	    return 0;
	}
	if(!STRICMP(argv[0], "list")) {
	    argv++, argc--;
	    root_writer_t::instance()->list(argc, argv);
	    return 0;
	}
	if(!STRICMP(argv[0], "merge")) {
	    argv++, argc--;
	    try {
		const char *dest = argv[0];
		argv++, argc--;
		root_writer_t::instance()->copy(dest, argc, argv);
	    } catch(obj_pak_exception_t *e) {
		printf("ERROR IN CLASS %s: %s\n", e->get_class().chars(), e->get_info().chars());
		delete e;
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
	"         Should work with PAK16 up to PAK255 but only 64 and 128 are tested\n"
	"      MakeObj LIST <pak file(s)>\n"
	"         Lists the contents ot the given pak files\n"
	"      MakeObj MERGE <pak file> <pak file(s)>\n"
	"         Merges multiple pak files into one new pak file\n"
	"\n"
	"      with QUIET as first arg copyright message will be omitted\n"
	"      with a trailing slash a direcory is searched rather than a file\n"
	"      default for PAK is PAK ./ ./\n"
	);
/*	"      MakeObj DUMP <pak file(s)>\n"
	"         Dumps the node structure of the given pak files\n"*/

    return 3;
}
