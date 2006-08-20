/*
 *
 *  root_writer.cpp
 *
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 *
 *  Modulbeschreibung:
 *      ...
 *
 */
#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../utils/searchfolder.h"

#include "../obj_besch.h"
#include "obj_node.h"
#include "obj_writer.h"

#include "root_writer.h"

/*
 *  static data
 */
cstring_t root_writer_t::inpath;


/*
 *  member function:
 *      root_writer_t::write_header()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      FILE *fp
 */
void root_writer_t::write_header(FILE *fp)
{
    uint32 l;

    fprintf(fp,
	"Simutrans object file\n"
	"Compiled with SimObjects " COMPILER_VERSION "\n\x1A");

    l = COMPILER_VERSION_CODE;
    fwrite(&l, 1, sizeof(uint32), fp);	// Compiler Version zum Checken

    obj_node_t::set_start_offset(ftell(fp));
}

/*
 *  member function:
 *      root_writer_t::write()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      const char *filename
 *      int argc
 *      char *argv[]
 */
void root_writer_t::write(const char *filename, int argc, char *argv[])
{
    searchfolder_t find;
    FILE *outfp = NULL;
    obj_node_t *node = NULL;
    bool separate = false;
    int i, j;
    cstring_t file = find.complete(filename, "pak");

    if(file.right(1) == "/") {
	printf("writing invidual files to %s\n", filename);
	separate = true;
    }
    else {
	outfp = fopen(file, "wb");

	if(!outfp) {
	    printf("ERROR: cannot create destination file %s\n", filename);
	    exit(3);
	}
	printf("writing file %s\n", filename);
	write_header(outfp);

	node = new obj_node_t(this, 0, NULL, false);
    }
    for(i = 0; !i || i < argc; i++) {
	const char *arg = i < argc ? argv[i] : "./";

	for(j = find.search(arg, "dat"); j-- > 0; ) {
	    tabfile_t infile;

	    if(infile.open(find.at(j))) {
		tabfileobj_t obj;

		printf("   reading file %s\n", find.at(j).chars());

		inpath = arg;
		int n = inpath.find_back('/');

		if(n) {
		    inpath = inpath.substr(0, n + 1);
		} else {
		    inpath = "";
		}

		while(infile.read(obj)) {
		    if(separate) {
			cstring_t name(filename);

			name = name + obj.get("obj") + "." + obj.get("name") + ".pak";

			outfp = fopen(name.chars(), "wb");
			if(!outfp) {
			    printf("ERROR: cannot create destination file %s\n", filename);
			    exit(3);
			}
			printf("   writing file %s\n", name.chars());
			write_header(outfp);

			node = new obj_node_t(this, 0, NULL, false);
		    }
		    obj_writer_t::write(outfp, *node, obj);

		    if(separate) {
			node->write(outfp);
			delete node;
			fclose(outfp);
		    }
		}
	    }
	    else
		printf("WARNING: cannot read %s\n", find.at(j).chars());
	}
    }
    if(!separate) {
	node->write(outfp);
	delete node;
	fclose(outfp);
    }
}


/*
 *  member function:
 *      root_writer_t::dump()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      int argc
 *      char *argv[]
 */
void root_writer_t::dump(int argc, char *argv[])
{
    searchfolder_t find;
    int i, j;

    for(i = 0; i < argc; i++) {
	bool any = false;
	for(j = find.search(argv[i], "pak"); j-- > 0; ) {
	    FILE *infp= fopen(find.at(j), "rb");

	    if(infp) {
		int c;
		do {
		    c = fgetc(infp);
		} while(!feof(infp) && c != '\x1a');

		// Compiled Verison
		uint32 version;

		fread(&version, sizeof(version), 1, infp);
		printf("File %s (version %d):\n", find.at(j).chars(), version);

		dump_nodes(infp, 1);
		fclose(infp);
	    }
	    any = true;
	}
	if(!any) {
	    printf("WARNING: file or dir %s not found\n", argv[i]);
	}
    }
}


/*
 *  member function:
 *      root_writer_t::list()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      int argc
 *      char *argv[]
 */
void root_writer_t::list(int argc, char *argv[])
{
    searchfolder_t find;
    int i, j;

    for(i = 0; i < argc; i++) {
	bool any = false;
	for(j = find.search(argv[i], "pak"); j-- > 0; ) {
	    FILE *infp= fopen(find.at(j), "rb");

	    if(infp) {
		int c;
		do {
		    c = fgetc(infp);
		} while(!feof(infp) && c != '\x1a');

		// Compiled Verison
		uint32 version;

		fread(&version, sizeof(version), 1, infp);
		printf("Contents of file %s (pak version %d):\n", find.at(j).chars(), version);
		printf("type             name\n"
		       "---------------- ------------------------------\n");

		obj_node_info_t node;
		fread(&node, sizeof(node), 1, infp);
		fseek(infp, node.size, SEEK_CUR);
		for(int i = 0; i < node.children; i++) {
		    list_nodes(infp);
		}
		fclose(infp);
	    }
	    any = true;
	}
	if(!any) {
	    printf("WARNING: file or dir %s not found\n", argv[i]);
	}
    }
}


/*
 *  member function:
 *      root_writer_t::copy()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      ...
 *
 *  Argumente:
 *      const char *name
 *      int argc
 *      char *argv[]
 */
void root_writer_t::copy(const char *name, int argc, char *argv[])
{
    searchfolder_t find;
    int i, j;

    cstring_t file = find.complete(name, "pak");

    FILE *outfp = fopen(file, "wb");

    if(!outfp) {
	printf("ERROR: cannot open destination file %s\n", file.chars());
	exit(3);
    }
    printf("writing file %s\n", file.chars());
    write_header(outfp);

    long start = ftell(outfp);

    obj_node_info_t root;

    root.children = 0;
    root.size = 0;
    root.type = obj_root;
    fwrite(&root, sizeof(root), 1, outfp);

    for(i = 0; i < argc; i++) {
	bool any = false;
	for(j = find.search(argv[i], "pak"); j-- > 0; ) {
	    FILE *infp = fopen(find.at(j), "rb");

	    if(infp) {
		int c;
		do {
		    c = fgetc(infp);
		} while(!feof(infp) && c != '\x1a');

		// Compiled Verison
		uint32 version;

		fread(&version, sizeof(version), 1, infp);
		if(version == COMPILER_VERSION_CODE) {
		    printf("   copying file %s\n", find.at(j).chars());

		    obj_node_info_t info;

		    fread(&info, sizeof(info), 1, infp);
		    root.children += info.children;
		    copy_nodes(outfp, infp, info);
		    any = true;
		}
		else {
		    printf("   WARNING: skipping file %s - version mismatch\n", find.at(j).chars());
		}
		fclose(infp);
	    }
	}
	if(!any) {
	    printf("WARNING: file or dir %s not found\n", argv[i]);
	}
    }
    fseek(outfp, start, SEEK_SET);
    fwrite(&root, sizeof(root), 1, outfp);
    fclose(outfp);
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      root_writer_t::copy_nodes()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
//
//  Arguments:
//      FILE *outfp
//      FILE *infp
//      obj_node_info_t &info
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void root_writer_t::copy_nodes(FILE *outfp, FILE *infp, obj_node_info_t &info)
{
    for(int i = 0; i < info.children; i++) {
	obj_node_info_t info;

	fread(&info, sizeof(info), 1, infp);
	fwrite(&info, sizeof(info), 1, outfp);
	char *buf = new char[info.size];
	fread(buf, info.size, 1, infp);
	fwrite(buf, info.size, 1, outfp);
	delete buf;
	copy_nodes(outfp, infp, info);
    }
}


//@ADOC
/////////////////////////////////////////////////////////////////////////////
//  member function:
//      root_writer_t::capabilites()
//
//---------------------------------------------------------------------------
//  Description:
//      ...
/////////////////////////////////////////////////////////////////////////////
//@EDOC
void root_writer_t::capabilites()
{
    printf("This program can pack the following object types (pak version %d) :\n", COMPILER_VERSION_CODE);
    show_capabilites();
}
