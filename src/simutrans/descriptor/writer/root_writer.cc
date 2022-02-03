/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <string>
#include <stdlib.h>
#include "../../dataobj/tabfile.h"
#include "../../utils/searchfolder.h"
#include "../obj_desc.h"
#include "obj_node.h"
#include "obj_writer.h"
#include "root_writer.h"

using std::string;

string root_writer_t::inpath;

void root_writer_t::write_header(FILE* fp)
{
	fprintf(fp,
		"Simutrans object file\n"
		"Compiled with SimObjects " COMPILER_VERSION "\n\x1A"
	);

	uint32 l = endian(uint32(COMPILER_VERSION_CODE));
	fwrite(&l, 1, sizeof(uint32), fp); // Compiler version to check

	obj_node_t::set_start_offset(ftell(fp));
}


// makes pak file(s)
void root_writer_t::write(const char* filename, int argc, char* argv[])
{
	searchfolder_t find;
	FILE* outfp = NULL;
	obj_node_t* node = NULL;
	bool separate = false;
	string file = find.complete(filename, "pak");

	if (file[file.size()-1] == '/') {
		printf("writing individual files to %s\n", filename);
		separate = true;
	}
	else {
		outfp = fopen(file.c_str(), "wb");

		if (!outfp) {
			dbg->fatal( "Write pak", "Cannot create destination file %s", filename );
		}

		if (debuglevel >= log_t::LEVEL_WARN) {
			printf("Writing file %s\n", filename);
		}
		write_header(outfp);

		node = new obj_node_t(this, 0, NULL);
	}

	for(  int i=0;  i==0  ||  i<argc;  i++  ) {
		const char* arg = (i < argc) ? argv[i] : "./";

		find.search(arg, "dat");
		for(const char* const& i : find) {
			tabfile_t infile;

			if (infile.open(i)) {
				tabfileobj_t obj;

				if (debuglevel >= log_t::LEVEL_WARN) {
					printf("   Reading file %s\n", i);
				}

				inpath = arg;
				string::size_type n = inpath.rfind('/');

				if(n!=string::npos) {
					inpath = inpath.substr(0, n + 1);
				}
				else {
					inpath = "";
				}

				while(infile.read(obj)) {
					if(separate) {
						string name(filename);

						name = name + obj.get("obj") + "." + obj.get("name") + ".pak";

						outfp = fopen(name.c_str(), "wb");
						if (!outfp) {
							dbg->fatal( "Write pak", "Cannot create destination file %s", filename );
						}

						if (debuglevel >= log_t::LEVEL_WARN) {
							printf("   Writing file %s\n", name.c_str());
						}

						write_header(outfp);
						node = new obj_node_t(this, 0, NULL);
					}
					obj_writer_t::write(outfp, *node, obj);
					obj.unused( "#;-/" );

					if(separate) {
						node->write(outfp);
						delete node;
						fclose(outfp);
					}
				}
			}
			else {
				dbg->warning( "Write pak", "Cannot read %s", i);
			}
		}
	}
	if (!separate) {
		node->write(outfp);
		delete node;
		fclose(outfp);
	}
}


void root_writer_t::write_obj_node_info_t(FILE* outfp, const obj_node_info_t &root)
{
	uint32 type      = endian(root.type);
	uint16 children  = endian(root.children);
	uint16 root_size = endian(uint16(root.size));
	fwrite(&type,     4, 1, outfp);
	fwrite(&children, 2, 1, outfp);
	if(  root.size>=LARGE_RECORD_SIZE  ) {
		root_size = LARGE_RECORD_SIZE;
		fwrite(&root_size,     2, 1, outfp);
		uint32 size = endian(root.size);
		fwrite(&size,     4, 1, outfp);
	}
	else {
		fwrite(&root_size,     2, 1, outfp);
	}
}


static bool skip_header(FILE* const f)
{
	for (;;) {
		int const c = fgetc(f);

		if (c == EOF) {
			dbg->error( "skip_header", "reached end of file while skipping header");
			return false;
		}

		if (c == '\x1A') {
			return true;
		}
	}
}


bool root_writer_t::do_dump(const char* open_file_name)
{
	if (FILE* const infp = fopen(open_file_name, "rb")) {
		if (skip_header(infp)) {
			// Compiled Version
			uint32 version;
			fread(&version, sizeof(version), 1, infp);
			printf("File %s (version %d):\n", open_file_name, endian(version));

			dump_nodes(infp, 1);
		}
		fclose(infp);
	}
	return true;
}


// dumps the node structure onto the screen
void root_writer_t::dump(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++) {
		bool any = false;

		// this is necessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i], '*') == NULL) {
			any = do_dump(argv[i]);
		}
		else {
			searchfolder_t find;
			find.search(argv[i], "pak");
			for(const char* const& i : find) {
				any |= do_dump(i);
			}
		}

		if (!any) {
			dbg->error( "root_writer_t::dump", "file or dir %s not found", argv[i] );
		}
	}
}


bool root_writer_t::do_list(const char* open_file_name)
{
	if (FILE* const infp = fopen(open_file_name, "rb")) {
		if (skip_header(infp)) {
			// Compiled Version
			uint32 version;

			fread(&version, sizeof(version), 1, infp);
			printf("Contents of file %s (pak version %d):\n", open_file_name, endian(version));
			printf("type              name                            nodes  size\n"
			       "----------------  ------------------------------  -----  ----------\n");

			obj_node_info_t node;
			obj_node_t::read_node( infp, node );
			fseek(infp, node.size, SEEK_CUR);

			for (int i = 0; i < node.children; i++) {
				list_nodes(infp);
			}
		}
		fclose(infp);
		return true;
	}

	return false;
}


// list the content of a file
void root_writer_t::list(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++) {
		bool any = false;

		// this is necessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i],'*') == NULL) {
			any = do_list(argv[i]);
		}
		else {
			searchfolder_t find;
			find.search(argv[i], "pak");
			for(const char* const& i : find) {
				any |= do_list(i);
			}
		}

		if (!any) {
			dbg->error( "root_writer_t::list", "file or dir %s not found", argv[i] );
		}
	}
}


bool root_writer_t::do_copy(FILE* outfp, obj_node_info_t& root, const char* open_file_name)
{
	bool any = false;
	if (FILE* const infp = fopen(open_file_name, "rb")) {
		if (skip_header(infp)) {
			// Compiled Version check (since the ancient ending was also pak)
			uint32 version;
			fread(&version, sizeof(version), 1, infp);
			if (endian(version) <= COMPILER_VERSION_CODE) {
				printf("   copying file %s\n", open_file_name);

				obj_node_info_t info;
				obj_node_t::read_node( infp, info );

				root.children += info.children;
				copy_nodes(outfp, infp, info);
				any = true;
			}
			else {
				dbg->warning( "root_writer_t::do_copy", "skipping file %s - version mismatch (need same version to merge!)", open_file_name);
			}
		}
		fclose(infp);
	}
	return any;
}


// merges pak files into an archive
//
void root_writer_t::copy(const char* name, int argc, char* argv[])
{
	searchfolder_t find;

	FILE* outfp = NULL;
	if (strchr(name, '*') == NULL) {
		// is not a wildcard name
		outfp = fopen(name, "wb");
	}
	if (outfp == NULL) {
		name = find.complete(name, "pak").c_str();
		outfp = fopen(name, "wb");
	}
	if (!outfp) {
		dbg->fatal( "Merge", "Cannot open destination file %s", name);
	}
	fclose(outfp);
	if (remove(name) != 0) {
		dbg->warning("Merge", "Could not delete %s");
	}
	// create temporary file
	std::string tmpfile_name = name;
	tmpfile_name += ".tmp";
	outfp = fopen(tmpfile_name.c_str(), "wb");

	printf("writing to temporary file %s\n", tmpfile_name.c_str());
	write_header(outfp);

	long start = ftell(outfp); // remember position for adding children
	obj_node_info_t root;
	root.children = 0; // we will change this later
	root.size = 0;
	root.type = obj_root;
	this->write_obj_node_info_t(outfp, root);

	for(  int i=0;  i<argc;  i++  ) {
		bool any = false;

		// this is necessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i], '*') == NULL) {
			any = do_copy(outfp, root, argv[i]);
		} else {
			find.search(argv[i], "pak");
			for(const char* const& i : find) {
				if (strcmp(i, name) != 0) {
					any |= do_copy(outfp, root, i);
				}
				else {
					dbg->warning( "Merge", "Skipping reading from output file");
				}
			}
		}

		if (!any) {
			dbg->warning( "Merge", "file or dir %s not found\n", argv[i]);
		}
	}
	fseek(outfp, start, SEEK_SET);

	this->write_obj_node_info_t(outfp, root);

	fclose(outfp);

	printf("renaming temporary file %s to %s\n", tmpfile_name.c_str(), name);

	rename(tmpfile_name.c_str(), name);
}


/* makes single files from a merged file */
void root_writer_t::uncopy(const char* name)
{
	FILE* infp = NULL;
	if (strchr(name,'*') == NULL) {
		// is not a wildcard name
		infp = fopen(name, "rb");
	}
	if (infp == NULL) {
		searchfolder_t find;
		name = find.complete(name, "pak").c_str();
		infp = fopen(name, "rb");
	}

	if (!infp) {
		dbg->fatal( "Unmerge", "Cannot open archive file %s\n", name);
	}

	if (skip_header(infp)) {
		// check version of pak format
		uint32 version;
		fread(&version, sizeof(version), 1, infp);
		if (endian(version) <= COMPILER_VERSION_CODE) {
			// read root node
			obj_node_info_t root;
			obj_node_t::read_node( infp, root );
			if (root.children == 1) {
				dbg->error( "Unmerge", "%s is not an archive (aborting)", name);
				fclose(infp);
				exit(3);
			}

			printf("  found %d files to extract\n\n", root.children);

			// now iterate over the archive
			for (  int number=0;  number<root.children;  number++  ) {
				// read the info node ...
				long start_pos=ftell(infp);

				// now make a name
				string writer = node_writer_name(infp);
				string node_name;
				if(  writer=="factory"  ) {
					// we need to take name from following building node ...
					obj_node_info_t node;
					size_t pos = ftell(infp);
					obj_node_t::read_node( infp, node );
					fseek(infp, node.size, SEEK_CUR );
					node_name = name_from_next_node(infp);
					fseek( infp, pos, SEEK_SET );
				}
				else if(  writer=="bridge"  ) {
					size_t pos=ftell(infp);
					node_name = name_from_next_node(infp);
					if (node_name.empty()) {
						fseek( infp, pos, SEEK_SET );
						// we need to take name from third children, the cursor node ...
						obj_node_info_t node;
						// quick and dirty: since there are a variable number, we just skip all image nodes
						do {
							obj_node_t::read_node( infp, node );
							fseek(infp, node.size, SEEK_CUR );
						} while(  (node.type==obj_imagelist  ||  node.type==obj_image)  &&  !feof(infp)  );
						// then we try the next node (should be cursor node then!
						if(  node.type==obj_cursor  ) {
							node_name = name_from_next_node(infp);
						}
						fseek( infp, pos, SEEK_SET );
					}
				}
				else {
					node_name = name_from_next_node(infp);
				}
				// use a clever default name, if no name there
				// note: this doesn't work because node_name.size() is not 0 even if invalid file characters are contained.
				if (node_name.empty()) {
					char random_name[16];
					// we use the file position as unique name
					sprintf( random_name, "p%li", ftell(infp) );
					dbg->warning( "Unmerge", "%s has no name! (using %s) as default", writer.c_str(), (const char *)random_name );
					node_name = random_name;
				}
				string outfile = writer + "." + node_name + ".pak";
				FILE* outfp = fopen(outfile.c_str(), "wb");
				if (!outfp) {
					dbg->error( "Unmerge", "Could not open %s for writing (aborting)", outfile.c_str());
					fclose(infp);
					exit(3);
				}
				printf("  writing '%s' ... \n", outfile.c_str());

				// now copy the nodes
				fseek(infp, start_pos, SEEK_SET);
				write_header(outfp);

				// write the root for the new pak file
				obj_node_info_t root;
				root.children = 1;
				root.size = 0;
				root.type = obj_root;
				write_obj_node_info_t( outfp, root );
				copy_nodes(outfp, infp, root); // this advances also the input to the next position
				fclose(outfp);
			}
		}
		else {
			dbg->warning( "Unmerge", "Skipping file %s - version mismatch", name);
		}
	}
	fclose(infp);
}


void root_writer_t::copy_nodes(FILE* outfp, FILE* infp, obj_node_info_t& start)
{
	for(  int i=0;  i<start.children;  i++  ) {
		// copy header
		obj_node_info_t info;
		obj_node_t::read_node( infp, info );
		write_obj_node_info_t( outfp, info );
		// copy data
		char* buf = new char[info.size];
		fread(buf, info.size, 1, infp);
		fwrite(buf, info.size, 1, outfp);
		delete []  buf;
		copy_nodes(outfp, infp, info);
	}
}


void root_writer_t::capabilites()
{
	printf("This program can pack the following object types (pak version %d) :\n", COMPILER_VERSION_CODE);
	show_capabilites();
}


void root_writer_t::expand_dat(const char* filename, int argc, char* argv[])
{
	searchfolder_t find;
	FILE* outfp = NULL;
	bool generate_name = false;
	string file = find.complete(filename, "dat");

	if (file[file.size()-1] == '/') {
		printf("writing individual files to %s\n", filename);
		generate_name = true;
	}
	else {
		outfp = fopen(file.c_str(), "wb");

		if (!outfp) {
			dbg->fatal( "Write dat", "Cannot create destination file %s", filename );
		}

		if (debuglevel >= log_t::LEVEL_WARN) {
			printf("Writing file %s\n", filename);
		}
	}

	for(  int i=0;  i==0  ||  i<argc;  i++  ) {
		const char* arg = (i < argc) ? argv[i] : "./";

		find.search(arg, "dat");

		for(const char* const& i : find) {
			tabfile_t infile;

			if (infile.open(i)) {
				tabfileobj_t obj;

				if (debuglevel >= log_t::LEVEL_WARN) {
					printf("   Reading file %s\n", i);
				}

				inpath = arg;
				string::size_type n = inpath.rfind('/');

				if(n!=string::npos) {
					inpath = inpath.substr(0, n + 1);
				}
				else {
					inpath = "";
				}

				if(generate_name) {
					string name(i);

					name.replace(name.size() - 3, 3, "expanded.dat");

					outfp = fopen(name.c_str(), "wb");
					if (!outfp) {
						dbg->fatal( "Write pak", "Cannot create destination file %s", name.c_str() );
					}

					if (debuglevel >= log_t::LEVEL_WARN) {
						printf("   Writing file %s\n", name.c_str());
					}

				}

				// fprintf(outfp, "# Expanded file by makeobj\n");
				while(infile.read(obj, outfp)) {
					fprintf(outfp, "---\n");
				}

				if(generate_name) {
					fclose(outfp);
				}
			}
			else {
				dbg->warning( "Write dat", "Cannot read %s", i);
			}
		}
	}
	if(!generate_name) {
		fclose(outfp);
	}
}
