/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_LOADSAVE_H
#define DATAOBJ_LOADSAVE_H


#include <stdio.h>
#include <string>

#include "../simtypes.h"

class plainstring;
struct file_descriptors_t;

/**
 * This class replaces the FILE when loading and saving games.
 * </p>
 * Can now read and write 3 formats: text, binary and zipped
 * Input format is automatically detected.
 * Output format has a default, changeable with set_savemode, but can be
 * overwritten in wr_open.
 */
class loadsave_t
{
public:
	enum mode_t {
		binary     = 0,
		text       = 1 << 0,
		xml        = 1 << 1,
		zipped     = 1 << 2,
		bzip2      = 1 << 3,
		zstd       = 1 << 4,
		xml_zipped = xml | zipped,
		xml_bzip2  = xml | bzip2,
		xml_zstd   = xml | zstd
	};

	enum file_error_t {
		FILE_ERROR_OK = 0,
		FILE_ERROR_NOT_EXISTING,
		FILE_ERROR_BZ_CORRUPT,
		FILE_ERROR_GZ_CORRUPT,
		FILE_ERROR_NO_VERSION,
		FILE_ERROR_FUTURE_VERSION,
		FILE_ERROR_UNSUPPORTED_COMPRESSION
	};

private:
	file_error_t last_error;
	int mode; ///< See mode_t
	bool saving;
	bool buffered;
	unsigned curr_buff;
	unsigned buf_pos[2];
	unsigned buf_len[2];
	char* ls_buf[2];
	uint32 version;         ///< savegame version
	int ident;              // only for XML formatting
	char pak_extension[64]; // name of the pak folder during savetime

	std::string filename;   // the current name ...

	file_descriptors_t *fd;

	/// @sa putc
	inline void lsputc(int c);

	/// @sa getc
	inline int lsgetc();

	size_t write(const void * buf, size_t len);
	size_t read(void *buf, size_t len);

	void rdwr_xml_number(sint64 &s, const char *typ);

	loadsave_t(const loadsave_t&);
	loadsave_t& operator=(const loadsave_t&);

	friend void *save_thread( void *ptr );
	friend void *load_thread( void *ptr );

	/**
	 * Reads into buffer number @p buf_num.
	 * @returns number of bytes read or -1 in case of error
	 */
	int fill_buffer(int buf_num);

	void flush_buffer(int buf_num);

public:
	static mode_t save_mode;     ///< default to use for saving
	static mode_t autosave_mode; ///< default to use for autosaves and network mode client temp saves
	static int save_level;       ///< default to use for compression (various libraries allow for size/speed settings)
	static int autosave_level;

	/**
	 * Parses the version information from @p version_text to a version number.
	 * @param[out] pak Pointer to a sufficiently large buffer (>= 64 chars); when the function returns,
	 *                 @p pak contains the pakset extension string. May be NULL.
	 * @retval 0   if an error occurred or the save cannot be loaded
	 * @retval !=0 the save version; in this case we can read the save file.
	 */
	static uint32 int_version(const char *version_text, char *pak);

	loadsave_t();
	~loadsave_t();

	bool rd_open(const char *filename);
	bool wr_open(const char *filename, mode_t mode, int level, const char *pak_extension, const char *savegame_version );
	const char *close();

	file_error_t get_last_error() { return last_error; }

	static void set_savemode(mode_t mode) { save_mode = mode; }
	static void set_autosavemode(mode_t mode) { autosave_mode = mode; }
	static void set_savelevel(int level) { save_level = level;  }
	static void set_autosavelevel(int level) { autosave_level = level;  }

	/**
	 * Checks end-of-file
	 */
	bool is_eof();

	void set_buffered(bool enable);
	unsigned get_buf_pos(int buf_num) const { return buf_pos[buf_num]; }
	bool is_loading() const { return !saving; }
	bool is_saving() const { return saving; }
	bool is_zipped() const { return mode&zipped; }
	bool is_bzip2() const { return mode&bzip2; }
	bool is_zstd() const { return mode&zstd; }
	bool is_xml() const { return mode&xml; }
	const char *get_pak_extension() const { return pak_extension; }

	uint32 get_version_int() const { return version; }
	inline bool is_version_atleast(uint32 major, uint32 save_minor) const { return !is_version_less(major, save_minor); }
	inline bool is_version_less(uint32 major, uint32 save_minor)    const { return version <  major * 1000U + save_minor; }
	inline bool is_version_equal(uint32 major, uint32 save_minor)   const { return version == major * 1000U + save_minor; }

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

	/**
	 * Read/Write plainstring.
	 * @param str the string to be read/written
	 * @post str should not be NULL after reading.
	 */
	void rdwr_str(plainstring& str);

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



// this produced semicautomatic hierarchies
class xml_tag_t {
private:
	loadsave_t *file;
	const char *tag;
public:
	xml_tag_t( loadsave_t *f, const char *t ) : file(f), tag(t) { file->start_tag(tag); }
	~xml_tag_t() { file->end_tag(tag); }
};



#endif
