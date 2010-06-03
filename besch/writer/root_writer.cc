#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../utils/searchfolder.h"
#include "../obj_besch.h"
#include "obj_node.h"
#include "obj_writer.h"
#include "root_writer.h"
#include <stdlib.h>


cstring_t root_writer_t::inpath;

void root_writer_t::write_header(FILE* fp)
{
	fprintf(fp,
		"Simutrans object file\n"
		"Compiled with SimObjects " COMPILER_VERSION "\n\x1A"
	);

	uint32 l = endian(uint32(COMPILER_VERSION_CODE));
	fwrite(&l, 1, sizeof(uint32), fp); // Compiler Version zum Checken

	obj_node_t::set_start_offset(ftell(fp));
}


// makes pak file(s)
void root_writer_t::write(const char* filename, int argc, char* argv[])
{
	searchfolder_t find;
	FILE* outfp = NULL;
	obj_node_t* node = NULL;
	bool separate = false;
	cstring_t file = find.complete(filename, "pak");

	if (file.right(1) == "/") {
		printf("writing invidual files to %s\n", filename);
		separate = true;
	} else {
		outfp = fopen(file, "wb");

		if (!outfp) {
			printf("ERROR: cannot create destination file %s\n", filename);
			exit(3);
		}
		printf("writing file %s\n", filename);
		write_header(outfp);

		node = new obj_node_t(this, 0, NULL);
	}

	for(  int i=0;  i==0  ||  i<argc;  i++  ) {
		const char* arg = (i < argc) ? argv[i] : "./";

		find.search(arg, "dat");
		for(  searchfolder_t::const_iterator i=find.begin(), end=find.end();  i!=end;  ++i  ) {
			tabfile_t infile;

			if(infile.open(*i)) {
				tabfileobj_t obj;

				printf("   reading file %s\n", *i);

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

						outfp = fopen(name, "wb");
						if (!outfp) {
							printf("ERROR: cannot create destination file %s\n", filename);
							exit(3);
						}
						printf("   writing file %s\n", (const char*)name);
						write_header(outfp);

						node = new obj_node_t(this, 0, NULL);
					}
					obj_writer_t::write(outfp, *node, obj);

					if(separate) {
						node->write(outfp);
						delete node;
						fclose(outfp);
					}
				}
			} else {
				printf("WARNING: cannot read %s\n", *i);
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


bool root_writer_t::do_dump(const char* open_file_name)
{
	FILE* infp = fopen(open_file_name, "rb");
	if (infp) {
		int c;
		do {
			c = fgetc(infp);
		} while(  !feof(infp)  &&  c!='\x1a'  );

		// Compiled Verison
		uint32 version;
		fread(&version, sizeof(version), 1, infp);
		printf("File %s (version %d):\n", open_file_name, endian(version));

		dump_nodes(infp, 1);
		fclose(infp);
	}
	return true;
}


// dumps the node structure onto the screen
void root_writer_t::dump(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++) {
		bool any = false;

		// this is neccessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i], '*') == NULL) {
			any = do_dump(argv[i]);
		} else {
			searchfolder_t find;
			find.search(argv[i], "pak");
			for (searchfolder_t::const_iterator i = find.begin(), end = find.end(); i != end; ++i) {
				any |= do_dump(*i);
			}
		}

		if (!any) {
			printf("WARNING: file or dir %s not found\n", argv[i]);
		}
	}
}


bool root_writer_t::do_list(const char* open_file_name)
{
	FILE* infp = fopen(open_file_name, "rb");
	if (infp) {
		int c;
		do {
			c = fgetc(infp);
		} while (!feof(infp) && c != '\x1a');

		// Compiled Verison
		uint32 version;

		fread(&version, sizeof(version), 1, infp);
		printf("Contents of file %s (pak version %d):\n", open_file_name, endian(version));
		printf("type             name\n"
		"---------------- ------------------------------\n");

		obj_node_info_t node;
		obj_node_t::read_node( infp, node );
		fseek(infp, node.size, SEEK_CUR);

		for(  int i=0;  i<node.children;  i++  ) {
			list_nodes(infp);
		}
		fclose(infp);
	}
	return true;
}


// list the content of a file
void root_writer_t::list(int argc, char* argv[])
{
	for (int i = 0; i < argc; i++) {
		bool any = false;

		// this is neccessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i],'*') == NULL) {
			do_list(argv[i]);
		}
		else {
			searchfolder_t find;
			find.search(argv[i], "pak");
			for (searchfolder_t::const_iterator i = find.begin(), end = find.end(); i != end; ++i) {
				do_list(*i);
			}
		}

		if (!any) {
			printf("WARNING: file or dir %s not found\n", argv[i]);
		}
	}
}


bool root_writer_t::do_copy(FILE* outfp, obj_node_info_t& root, const char* open_file_name)
{
	bool any = false;
	FILE* infp = fopen(open_file_name, "rb");
	if (infp) {
		int c;
		do {
			c = fgetc(infp);
		} while (!feof(infp) && c != '\x1a');

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
			fprintf(stderr, "   WARNING: skipping file %s - version mismatch\n", open_file_name);
		}
		fclose(infp);
	}
	return any;
}


// merges pak files into an archieve
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
		name = find.complete(name, "pak");
		outfp = fopen(name, "wb");
	}

	if (!outfp) {
		printf("ERROR: cannot open destination file %s\n", name);
		exit(3);
	}
	printf("writing file %s\n", name);
	write_header(outfp);

	long start = ftell(outfp);	// remember position for adding children
	obj_node_info_t root;
	root.children = 0;	// we will change this later
	root.size = 0;
	root.type = obj_root;
	this->write_obj_node_info_t(outfp, root);

	for(  int i=0;  i<argc;  i++  ) {
		bool any = false;

		// this is neccessary to avoid the hassle with "./*.pak" otherwise
		if (strchr(argv[i], '*') == NULL) {
			any = do_copy(outfp, root, argv[i]);
		} else {
			find.search(argv[i], "pak");
			for (searchfolder_t::const_iterator i = find.begin(), end = find.end(); i != end; ++i) {
				if (strcmp(*i, name) != 0) {
					any |= do_copy(outfp, root, *i);
				}
				else {
					printf("WARNING: skipping reading from output file\n");
				}
			}
		}

		if (!any) {
			printf("WARNING: file or dir %s not found\n", argv[i]);
		}
	}
	fseek(outfp, start, SEEK_SET);

	this->write_obj_node_info_t(outfp, root);

	fclose(outfp);
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
		name = find.complete(name, "pak");
		infp = fopen(name, "rb");
	}

	if (!infp) {
		printf("ERROR: cannot open archieve file %s\n", name);
		exit(3);
	}

	// read header
	int c;
	do {
		c = fgetc(infp);
	} while(  !feof(infp)  &&  c!='\x1a'  );

	// check version of pak format
	uint32 version;
	fread(&version, sizeof(version), 1, infp);
	if (endian(version) <= COMPILER_VERSION_CODE) {
		// read root node
		obj_node_info_t root;
		obj_node_t::read_node( infp, root );
		if (root.children == 1) {
			printf("  ERROR: %s is not an archieve (aborting)\n", name);
			fclose(infp);
			exit(3);
		}

		printf("  found %d files to extract\n\n", root.children);

		// now itereate over the archieve
		for (  int number=0;  number<root.children;  number++  ) {
			// read the info node ...
			long start_pos=ftell(infp);

			// now make a name
			cstring_t writer = node_writer_name(infp);
			cstring_t node_name;
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
				// we need to take name from thrid children, the cursor node ...
				obj_node_info_t node;
				size_t pos = ftell(infp);
				// quick and dirty: since there are a variable number, we just skip all image nodes
				do {
					obj_node_t::read_node( infp, node );
					fseek(infp, node.size, SEEK_CUR );
				} while(  (node.type==obj_imagelist  ||  node.type==obj_image)  &&  !feof(infp)  );
				// then we try the next node (shoudl be cursor node then!
				if(  node.type==obj_cursor  ) {
					node_name = name_from_next_node(infp);
				}
				fseek( infp, pos, SEEK_SET );
			}
			else {
				node_name = name_from_next_node(infp);
			}
			// use a clever default name, if no name there
			if(  node_name.len()==0  ) {
				char random_name[16];
				// we use the file position as unique name
				sprintf( random_name, "p%li", ftell(infp) );
				printf("  ERROR: %s has no name! (using %s) as default\n", (const char *)writer, (const char *)random_name );
				node_name = random_name;
			}
			cstring_t outfile = writer + "." + node_name + ".pak";
			FILE* outfp = fopen(outfile, "wb");
			if (!outfp) {
				printf("  ERROR: could not open %s for writing (aborting)\n", (const char*)outfile);
				fclose(infp);
				exit(3);
			}
			printf("  writing '%s' ... \n", (const char*)outfile);

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
	} else {
		printf("   WARNING: skipping file %s - version mismatch\n", name);
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
