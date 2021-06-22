/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_ROOT_WRITER_H
#define DESCRIPTOR_WRITER_ROOT_WRITER_H


#include <string>
#include <cstdio>

#include "obj_writer.h"
#include "../objversion.h"


struct obj_node_info_t;


class root_writer_t : public obj_writer_t
{
private:
	static root_writer_t the_instance;

	static std::string inpath;

	root_writer_t() { register_writer(false); }

	void copy_nodes(FILE* outfp, FILE* infp, obj_node_info_t& info);
	void write_header(FILE* fp);
	void write_obj_node_info_t(FILE* outfp, const obj_node_info_t &root);

public:
	void capabilites();

	static root_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_root; }
	const char *get_type_name() const OVERRIDE { return "root"; }

	void write(const char* name, int argc, char* argv[]);
	void dump(int argc, char* argv[]);
	void list(int argc, char* argv[]);
	void copy(const char* name, int argc, char* argv[]);

	/**
	 * @brief Expands makeobj pre-processor stuff
	 *
	 * Dat files can contain 'minified' pre-processor-like features, this
	 * function will run the dat on the tabfile reader and return write
	 * its output to a $filename-expanded.dat file.
	 *
	 * @param name Filename of the output file or folder
	 * @param argc Number of input files
	 * @param argv List of input files' paths
	 */
	void expand_dat(const char* name, int argc, char* argv[]);

	/* makes single files from a merged file */
	void uncopy(const char* name);

	static const std::string & get_inpath() { return inpath; }

private:
	bool do_copy(FILE* outfp, obj_node_info_t& root, const char* open_file_name);
	bool do_dump(const char* open_file_name);
	bool do_list(const char* open_file_name);
};


#endif
