/*
 * loadsave.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef loadsave_h
#define loadsave_h

#include <zlib.h>
#include <stdio.h>

#include "../utils/cstring_t.h"

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
    enum mode_t { text, binary, zipped };
private:
    static mode_t save_mode;	// default to use for saving

    mode_t mode;
    bool saving;
    int version;

	cstring_t filename;	// the current name ...

    FILE *fp;

    static int int_version(const char *version_text, mode_t *mode);

    // Hajo: putc got a name clash on my system
    int lsputc(int c);

    // Hajo: getc got a name clash on my system
    int lsgetc();
    long write(const void * buf, unsigned long len);
    long read (void *buf, unsigned long len);

public:
    loadsave_t();
    ~loadsave_t();

    bool rd_open(const char *filename);
    bool wr_open(const char *filename, mode_t mode = save_mode);
    void close();

    static void set_savemode(mode_t mode) { save_mode = mode; }
    /**
     * Checks end-of-file
     * @author Hj. Malthaner
     */
    bool is_eof();

    void* gib_file() { return fp; }
    bool is_loading() const { return !saving; }
    bool is_saving() const { return saving; }
    bool is_zipped() const { return mode == zipped; }
    bool is_binary() const { return mode == binary; }
    bool is_text() const { return mode == text; }
    int get_version() const { return version; }

    void rdwr_byte(signed char &c, const char *delim);
    void rdwr_byte(unsigned char &c, const char *delim);
    void rdwr_short(short &i, const char *delim);
    void rdwr_short(unsigned short &i, const char *delim);
    void rdwr_long(long &i, const char *delim);
    void rdwr_long(unsigned long &i, const char *delim);
    void rdwr_long(int &i, const char *delim);
    void rdwr_long(unsigned &i, const char *delim);
    void rdwr_longlong(sint64 &i, const char *delim);
    void rdwr_bool(bool &i, const char *delim);
    void rdwr_double(double &dbl, const char *delim);             //01-Dec-01     Markus Weber    Added

    void wr_obj_id(short id);
    short rd_obj_id();
    void wr_obj_id(const char *id_text);
    void rd_obj_id(char *id_buf, int size);

    // s is a malloc-ed string
    void rdwr_str(const char *&s, const char *null_s);
    // s is a buf of size given
    void rdwr_str(char *s, int size);
    void rdwr_delim(const char *delim);

    // use this for enum types
    template <class X>
    void rdwr_enum(X &x, const char *delim)
    {
        int int_x;

        if(is_saving()) {
            int_x = (int)x;
        }
        rdwr_long(int_x, delim);
        if(is_loading()) {
            x = (X)int_x;
        }
    }


    /**
     * Read string into preallocated buffer.
     * @author Hj. Malthaner
     */
    void rd_str_into(char *s, const char *null_s);
};

#endif
