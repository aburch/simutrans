/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef loadsave_h
#define loadsave_h

#include <stdio.h>
#include <bzlib.h>

#include "../utils/cstring_t.h"
#include "../simtypes.h"

/**
 * loadsave_t:
 *
 * This class replaces the FILE when loading and saving games.
 * <p>
 * Hj. Malthaner, 16-Feb-2002, added zlib compression support
 * </p>
 * Can now read and write 3 formats: text, binary and zipped
 * Input format is automatically detected.
 * Output format has a default, changeable with set_savemode, but can be
 * overwritten in wr_open.
 *
 * @author V. Meyer, Hj. Malthaner
 */


class loadsave_t {
public:
	enum mode_t { text=1, xml=2, binary=0, zipped=4, xml_zipped=6, bzip2=8, xml_bzip2=10 };

private:
	int mode;
	bool saving;
	int version;
	int ident;		// only for XML formatting
	char pak_extension[64];	// name of the pak folder during savetime

	cstring_t filename;	// the current name ...

	FILE *fp;
	BZFILE *bzfp;
	int bse;

	// Hajo: putc got a name clash on my system
	void lsputc(int c);

	// Hajo: getc got a name clash on my system
	int lsgetc();
	long write(const void * buf, size_t len);
	long read(void *buf, size_t len);

	void rdwr_xml_number(sint64 &s, const char *typ);

public:
	static mode_t save_mode;	// default to use for saving
	static uint32 int_version(const char *version_text, int *mode, char *pak);

	loadsave_t();
	~loadsave_t();

	bool rd_open(const char *filename);
	bool wr_open(const char *filename, mode_t mode, const char *pak_extension);
	const char *close();

	static void set_savemode(mode_t mode) { save_mode = mode; }
	/**
	 * Checks end-of-file
	 * @author Hj. Malthaner
	 */
	bool is_eof();

	bool is_loading() const { return !saving; }
	bool is_saving() const { return saving; }
	bool is_zipped() const { return mode&zipped; }
	bool is_bzip2() const { return mode&bzip2; }
	bool is_xml() const { return mode&xml; }
	uint32 get_version() const { return version; }
	const char *get_pak_extension() const { return pak_extension; }

	void rdwr_byte(sint8 &c, const char *delim);
	void rdwr_byte(uint8 &c, const char *delim);
	void rdwr_short(sint16 &i, const char *delim);
	void rdwr_short(uint16 &i, const char *delim);
	void rdwr_long(sint32 &i, const char *delim);
	void rdwr_long(uint32 &i, const char *delim);
	void rdwr_longlong(sint64 &i, const char *delim);
	void rdwr_bool(bool &i, const char *delim);
	void rdwr_double(double &dbl);

	void wr_obj_id(short id);
	short rd_obj_id();
	void wr_obj_id(const char *id_text);
	void rd_obj_id(char *id_buf, int size);

	// s is a malloc-ed string (will be freed and newly allocated on load time!)
	void rdwr_str(const char *&s);

	// s is a buf of size given
	void rdwr_str(char *s, int size);

	// only meaningful for XML
	void start_tag( const char *tag );
	void end_tag( const char *tag );

	// use this for enum types
	template <class X>
	void rdwr_enum(X &x, const char *delim)
	{
		sint32 int_x;

		if(is_saving()) {
			int_x = (sint32)x;
		}
		rdwr_long(int_x, delim);
		if(is_loading()) {
			x = (X)int_x;
		}
	}
};



// this produced semicautomatic hierachies
class xml_tag_t {
private:
	loadsave_t *file;
	const char *tag;
public:
	xml_tag_t( loadsave_t *f, const char *t ) : file(f), tag(t) { file->start_tag(tag); }
	~xml_tag_t() { file->end_tag(tag); }
};



#endif
