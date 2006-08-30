#include "../../utils/cstring_t.h"
#include "../../dataobj/tabfile.h"
#include "../../utils/searchfolder.h"

#include "../obj_besch.h"
#include "obj_node.h"
#include "obj_writer.h"

#include "root_writer.h"


cstring_t root_writer_t::inpath;


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

			printf("   reading file %s\n", find.at(j));

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
			printf("WARNING: cannot read %s\n", find.at(j));
		}
	}
	if(!separate) {
		node->write(outfp);
		delete node;
		fclose(outfp);
	}
}



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
		printf("File %s (version %d):\n", find.at(j), version);

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
		printf("Contents of file %s (pak version %d):\n", find.at(j), version);
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
		    printf("   copying file %s\n", find.at(j));

		    obj_node_info_t info;

		    fread(&info, sizeof(info), 1, infp);
		    root.children += info.children;
		    copy_nodes(outfp, infp, info);
		    any = true;
		}
		else {
		    printf("   WARNING: skipping file %s - version mismatch\n", find.at(j));
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



/* makes single files from a merged file */
void root_writer_t::uncopy(const char *fname)
{
	searchfolder_t find;
	int i, j;
	cstring_t name=fname;

	FILE *infp = fopen(name.chars(), "rb");
	if(infp==NULL) {
		name = find.complete(fname, "pak");
		infp = fopen(name.chars(), "rb");
	}

	if(!infp) {
		printf("ERROR: cannot open archieve file %s\n", name);
		exit(3);
	}

	// read header
	int c;
	do {
		c = fgetc(infp);
	} while(!feof(infp) && c != '\x1a');

	// check version of pak format
	uint32 version;
	fread(&version, sizeof(version), 1, infp);
	if(version == COMPILER_VERSION_CODE) {

		// read root node
		obj_node_info_t root;
		fread(&root, sizeof(root), 1, infp);
		if(root.children==1) {
			printf("  ERROR: %s is not an archieve (aborting)\n", name );
			fclose(infp);
			return;
		}

		printf("  found %d files to extract\n\n", root.children );

		// now itereate over the archieve
		for( int number=0;  number<root.children;  number++  ) {
			// read the info node ...
			long start_pos=ftell(infp);

			// now make a name
			cstring_t writer = node_writer_name(infp);
			cstring_t outfile = writer + "." + name_from_next_node(infp) + ".pak";
			FILE *outfp = fopen( outfile.chars(), "wb" );
			if(!outfp) {
				printf("  ERROR: could not open %s for writing (aborting)\n", outfile.chars() );
				fclose(infp);
				return;
			}
			printf("  writing '%s' ... \n", outfile.chars() );

			// now copy the nodes
			fseek( infp, start_pos, SEEK_SET );
			write_header(outfp);

			// write the root for the new pak file
			obj_node_info_t root;
			root.children = 1;
			root.size = 0;
			root.type = obj_root;
			fwrite(&root, sizeof(root), 1, outfp);
			copy_nodes(outfp, infp, root );	// this advances also the input to the next position
			fclose(outfp);
		}
	}
	else {
		printf("   WARNING: skipping file %s - version mismatch\n", find.at(j));
	}
	fclose( infp );
}



void root_writer_t::copy_nodes(FILE *outfp, FILE *infp, obj_node_info_t &start)
{
    for(int i = 0; i < start.children; i++) {
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


void root_writer_t::capabilites()
{
    printf("This program can pack the following object types (pak version %d) :\n", COMPILER_VERSION_CODE);
    show_capabilites();
}
