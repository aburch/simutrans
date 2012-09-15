/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef loadsave_h
#define loadsave_h

// no windows macros please ...
#define NOMINMAX 1

#include <stdio.h>
#include <string>

#include "../simtypes.h"

class plainstring;
struct file_descriptors_t;

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
	bool buffered;
	unsigned curr_buff;
	unsigned buf_pos[2];
	unsigned buf_len[2];
	char* ls_buf[2];
	int version;
	int ident;		// only for XML formatting
	char pak_extension[64];	// name of the pak folder during savetime

	std::string filename;	// the current name ...

	file_descriptors_t *fd;

	// Hajo: putc got a name clash on my system
	inline void lsputc(int c);

	// Hajo: getc got a name clash on my system
	inline int lsgetc();
	size_t write(const void * buf, size_t len);
	size_t read(void *buf, size_t len);

	void rdwr_xml_number(sint64 &s, const char *typ);


	loadsave_t(const loadsave_t&);
	loadsave_t& operator=(const loadsave_t&);

	friend void *loadsave_thread( void *ptr );

	int fill_buffer(int buf_num);
	void flush_buffer(int buf_num);

public:
	static mode_t save_mode;	// default to use for saving
	static uint32 int_version(const char *version_text, int *mode, char *pak);

	loadsave_t();
	~loadsave_t();

	bool rd_open(const char *filename);
	bool wr_open(const char *filename, mode_t mode, const char *pak_extension, const char *svaegame_version );
	const char *close();

	static void set_savemode(mode_t mode) { save_mode = mode; }
	/**
	 * Checks end-of-file
	 * @author Hj. Malthaner
	 */
	bool is_eof();

	void set_buffered(bool enable);
	unsigned get_buf_pos(int buf_num) const { return buf_pos[buf_num]; }
	bool is_loading() const { return !saving; }
	bool is_saving() const { return saving; }
	bool is_zipped() const { return mode&zipped; }
	bool is_bzip2() const { return mode&bzip2; }
	bool is_xml() const { return mode&xml; }
	uint32 get_version() const { return version; }
	const char *get_pak_extension() const { return pak_extension; }

	void rdwr_byte(sint8 &c);
	void rdwr_byte(uint8 &c);
	void rdwr_short(sint16 &i);
	void rdwr_short(uint16 &i);
	void rdwr_long(sint32 &i);
	void rdwr_long(uint32 &i);
	void rdwr_longlong(sint64 &i);
	void rdwr_bool(bool &i);
	void rdwr_double(double &dbl);

	void wr_obj_id(short id);
	short rd_obj_id();
	void wr_obj_id(const char *id_text);
	void rd_obj_id(char *id_buf, int size);

	// s is a malloc-ed string (will be freed and newly allocated on load time!)
	void rdwr_str(const char *&s);

	// s is a buf of size given
	void rdwr_str(char* s, size_t size);

	void rdwr_str(plainstring&);

	// only meaningful for XML
	void start_tag( const char *tag );
	void end_tag( const char *tag );

	// use this for enum types
	template <class X>
	void rdwr_enum(X &x)
	{
		sint32 int_x;

		if(is_saving()) {
			int_x = (sint32)x;
		}
		rdwr_long(int_x);
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
