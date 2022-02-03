/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_LOADSAVE_H
#define DATAOBJ_LOADSAVE_H


#include <stdio.h>
#include <string>

#include "../simtypes.h"
#include "../io/classify_file.h"
#include "../io/rdwr/rdwr_stream.h"


class plainstring;


/**
 * This class replaces the FILE when loading and saving games.
 *
 * Can now read and write 3 formats: text, binary and zipped
 * Input format is automatically detected.
 * Output format has a default, changeable with set_savemode, but can be
 * overwritten in wr_open.
 */
class loadsave_t
{
private:
	// during reading, a fatal error will rename the file to "oldnam"-error, so one can try again
	void NORETURN fatal(const char* who, const char* format, ...);

	struct buf_t
	{
		size_t pos;
		size_t len;
		char *buf;
	};

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

	enum file_status_t {
		FILE_STATUS_OK = 0,
		FILE_STATUS_ERR_NOT_EXISTING,
		FILE_STATUS_ERR_CORRUPT,
		FILE_STATUS_ERR_NO_VERSION,
		FILE_STATUS_ERR_FUTURE_VERSION,
		FILE_STATUS_ERR_UNSUPPORTED_COMPRESSION
	};

protected:
	int mode; ///< See mode_t
	bool buffered;
	unsigned curr_buff;
	buf_t buff[2];

	int indent;              // only for XML formatting
	file_info_t finfo;
	std::string filename;

	rdwr_stream_t *stream;

	/// @sa putc
	inline void lsputc(int c);

	/// @sa getc
	inline int lsgetc();

	size_t read(void *buf, size_t len);
	size_t write(const void *buf, size_t len);
	void write_indent();

	void rdwr_xml_number(sint64 &s, const char *typ);

	loadsave_t(const loadsave_t&);
	loadsave_t& operator=(const loadsave_t&);

	friend void *save_thread( void *ptr );
	friend void *load_thread( void *ptr );

	/**
	 * Reads into buffer number @p buf_num.
	 * @returns number of bytes read or -1 in case of error
	 */
	size_t fill_buffer(int buf_num);

	void flush_buffer(int buf_num);

	bool is_xml() const { return mode&xml; }

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

	/// Open save file for reading. File format is detected automatically.
	file_status_t rd_open(const char *filename);

	/// Open save file for writing.
	file_status_t wr_open(const char *filename, mode_t mode, int level, const char *pak_extension, const char *savegame_version );

	/// Close an open save file. Returns an error message if saving was unsuccessful, the empty string otherwise.
	const char *close();

	static void set_savemode(mode_t mode) { save_mode = mode; }
	static void set_autosavemode(mode_t mode) { autosave_mode = mode; }
	static void set_savelevel(int level) { save_level = level;  }
	static void set_autosavelevel(int level) { autosave_level = level;  }

	/**
	 * Checks end-of-file
	 */
	bool is_eof();

	void set_buffered(bool enable);
	unsigned get_buf_pos(int buf_num) const { return buff[buf_num].pos; }
	bool is_loading() const { return stream && !stream->is_writing(); }
	bool is_saving() const { return stream && stream->is_writing(); }
	const char *get_pak_extension() const { return finfo.pak_extension; }

	uint32 get_version_int() const { return finfo.version; }
	inline bool is_version_atleast(uint32 major, uint32 save_minor) const { return !is_version_less(major, save_minor); }
	inline bool is_version_less(uint32 major, uint32 save_minor)    const { return finfo.version <  major * 1000U + save_minor; }
	inline bool is_version_equal(uint32 major, uint32 save_minor)   const { return finfo.version == major * 1000U + save_minor; }

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

	/// @p s is a buf of size given
	/// @p size includes space for the trailing 0
	void rdwr_str(char *s, size_t size);

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

	friend class compare_loadsave_t; // to access stream
};


/**
 * Class used to produce hash of savegame_version
 */
class stream_loadsave_t : public loadsave_t
{
public:
	stream_loadsave_t(rdwr_stream_t *stream);
};

/**
 * Class used to compare two savegames
 */
class compare_loadsave_t : public loadsave_t
{
public:
	compare_loadsave_t(loadsave_t *file1, loadsave_t *file2);
	~compare_loadsave_t();
};

// this produces semi-automatic hierarchies
class xml_tag_t
{
private:
	loadsave_t *file;
	const char *tag;

public:
	xml_tag_t( loadsave_t *f, const char *t ) : file(f), tag(t) { file->start_tag(tag); }
	~xml_tag_t() { file->end_tag(tag); }
};



#endif
