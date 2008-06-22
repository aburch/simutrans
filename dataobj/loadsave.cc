#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "../simtypes.h"
#include "../macros.h"
#include "../simversion.h"
#include "../simmem.h"
#include "../simdebug.h"
#include "loadsave.h"

#include "../utils/simstring.h"

#include <zlib.h>

loadsave_t::mode_t loadsave_t::save_mode = binary;	// default to use for saving

loadsave_t::loadsave_t() : filename()
{
	fp = NULL;
}

loadsave_t::~loadsave_t()
{
	close();
}

bool loadsave_t::rd_open(const char *filename)
{
	close();

	version = 0;
	fp = (FILE *)gzopen(filename, "rb");
	if(!fp) {
		return false;
	}
	saving = false;

	char buf[80];
	gzgets(fp, buf, 80);
	mode = zipped;

	if(strncmp(buf, SAVEGAME_PREFIX, sizeof(SAVEGAME_PREFIX) - 1)) {
		close();
		return false;
	}
	version = int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, &mode, pak_extension);
	if(mode != zipped) {
		gzclose(fp);
		fp = fopen(filename, "rb");
		fgets(buf, 80, fp);
	}
	if(*pak_extension==0) {
		strcpy( pak_extension, "(unknown)" );
	}
	this->filename = filename;
	return true;
}

bool loadsave_t::wr_open(const char *filename, mode_t m, const char *pak_extension)
{
	mode = m;
	close();

	if(mode == zipped) {
		fp = (FILE *)gzopen(filename, "wb");
	}
	else {
		fp = fopen(filename, "wb");
	}

	if(!fp) {
		return false;
	}
	saving = true;

	// get the right extension
	const char *start = pak_extension;
	const char *end = pak_extension + strlen(pak_extension)-1;
	const char *c = pak_extension;
	// find the start
	while(*c<*end) {
		if(*c==':'  ||  *c=='\\'  ||  *c=='/') {
			start = c+1;
		}
		c++;
	}
	assert(start<end);
	tstrncpy(this->pak_extension, start, lengthof(this->pak_extension));
	// delete trailing path seperator
	this->pak_extension[strlen(this->pak_extension)-1] = 0;

	if(is_zipped()) {
		gzprintf(fp, "%s%s%s\n", SAVEGAME_VERSION, "zip", this->pak_extension);
	}
	else {
		fprintf(fp, "%s%s%s\n", SAVEGAME_VERSION, mode == binary ? "bin" : "", pak_extension);
	}

	version = int_version(SAVEGAME_VER_NR, NULL, NULL );
	this->mode = mode;
	this->filename = filename;

	return true;
}


const char *loadsave_t::close()
{
	const char *success = NULL;
	if(fp != NULL) {
		if(is_zipped()) {
			int err_no;
			const char *err_str = gzerror( fp, &err_no );
			if(err_no!=Z_OK  &&  err_no!=Z_STREAM_END) {
				success =  err_no==Z_ERRNO ? strerror(errno) : err_str;
			}
			gzclose(fp);
		}
		else {
			int err_no = ferror(fp);
			fclose(fp);
			if(err_no!=0) {
				success = strerror(err_no);
			}
		}
		fp = NULL;
	}
	return success;
}


/**
 * Checks end-of-file
 * @author Hj. Malthaner
 */
bool loadsave_t::is_eof()
{
	if(is_zipped()) {
		return gzeof(fp) != 0;
	}
	else {
		return feof(fp) != 0;
	}
}


int loadsave_t::lsputc(int c)
{
	if(is_zipped()) {
		return gzputc(fp, c);
	} else {
		return fputc(c, fp);
	}
}

int loadsave_t::lsgetc()
{
	if(is_zipped()) {
		return gzgetc(fp);
	} else {
		return fgetc(fp);
	}
}

long loadsave_t::write(const void *buf, unsigned long len)
{
	if(is_zipped()) {
		return gzwrite(fp, const_cast<void *>(buf), len);
	} else {
		return (long)fwrite(buf, 1, len, fp);
	}
}

long loadsave_t::read(void *buf, unsigned long len)
{
	if(is_zipped()) {
		return gzread(fp, buf, len);
	} else {
		return (long)fread(buf, 1, len, fp);
	}
}


/* read data types (should check also for Intel/Motorola) etc */
void loadsave_t::rdwr_byte(sint8 &c, const char *delim)
{
	if(saving) {
		lsputc(c);
	} else {
		c = (sint8)lsgetc();
	}
	rdwr_delim(delim);
}

void loadsave_t::rdwr_byte(uint8 &c, const char *delim)
{
	sint8 cc=c;
	rdwr_byte(cc,delim);
	c = (uint8)cc;
}


/* shorts */
void loadsave_t::rdwr_short(sint16 &i, const char *delim)
{
	if(!is_text()) {
#ifdef BIG_ENDIAN
		if(saving) {
			sint16 ii = (sint16)endian_uint16((uint16 *)&i);
			write(&ii, sizeof(sint16));
		} else {
			uint16 ii;
			read(&ii, sizeof(sint16));
			i = (sint16)endian_uint16(&ii);
		}
#else
		if(saving) {
			write(&i, sizeof(sint16));
		} else {
			read(&i, sizeof(sint16));
		}
#endif
	} else {
		if(saving) {
			fprintf(fp, "%d", i);
		} else {
			fscanf(fp, "%hd", &i);
		}
	}
	rdwr_delim(delim);
}

void loadsave_t::rdwr_short(uint16 &i, const char *delim)
{
	sint16 ii=i;
	rdwr_short(ii,delim);
	i = (uint16)ii;
}


/* long words*/
void loadsave_t::rdwr_long(sint32 &l, const char *delim)
{
	if(!is_text()) {
#ifdef BIG_ENDIAN
		if(saving) {
			uint32 ii = (sint32)endian_uint32((uint32 *)&l);
			write(&ii, sizeof(uint32));
		} else {
			uint32 ii;
			read(&ii, sizeof(uint32));
			l = (sint32)endian_uint32(&ii);
		}
#else
		if(saving) {
			write(&l, sizeof(sint32));
		} else {
			read(&l, sizeof(sint32));
		}
#endif
	} else {
		if(saving) {
 			if (sizeof(sint32) == sizeof(int)) {
 				fprintf(fp, "%d", l);
 			} else {
 				fprintf(fp, "%ld", l);
 			}
		} else {
 			if (sizeof(sint32) == sizeof(int)) {
 				fscanf(fp, "%d", &l);
 			} else {
 				fscanf(fp, "%ld", &l);
 			}
		}
	}
	rdwr_delim(delim);
}

void loadsave_t::rdwr_long(uint32 &l, const char *delim)
{
	sint32 ll=l;
	rdwr_long(ll,delim);
	l = (uint32)ll;
}

/* long long (64 Bit) */
void loadsave_t::rdwr_longlong(sint64 &ll, const char *delim)
{
	if(!is_text()) {
#ifdef BIG_ENDIAN
		if(saving) {
			sint64 ii = (sint64)endian_uint64((uint64 *)&ll);
			write(&ii, sizeof(sint64));
		} else {
			uint64 ii;
			read(&ii, sizeof(sint64));
			ll = (sint64)endian_uint64(&ii);
		}
#else
		if(saving) {
			write(&ll, sizeof(sint64));
		} else {
			read(&ll, sizeof(sint64));
		}
#endif
	} else {
		if(saving) {
			fprintf(fp, "%f", (double)ll);
		} else {
			double dbl;
			fscanf(fp, "%lf", &dbl);
			ll = (sint64)dbl;
		}
	}
	rdwr_delim(delim);
}



void loadsave_t::rdwr_bool(bool &i, const char *delim)
{
	if(saving) {
		lsputc(i ? '1' : '0');
	} else {
		i = lsgetc() == '1';
	}
	rdwr_delim(delim);
}


void loadsave_t::rdwr_str(const char *&s, const char *null_s)
{
	if(!is_text()) {
		sint16 size;
		if(saving) {
			size = s ? strlen(s) : 0;
#ifdef BIG_ENDIAN
			{
				uint16 ii = endian_uint16((uint16 *)&size);
				write(&ii, sizeof(sint16));
			}
#else
			write(&size, sizeof(sint16));
#endif
			if(size > 0) {
				write(s, size);
			}
		}
		else {
#ifdef BIG_ENDIAN
			{
				uint16 ii;
				read(&ii, sizeof(uint16));
				size = (sint16)endian_uint16(&ii);
			}
#else
			read(&size, sizeof(sint16));
#endif
			char *sneu = NULL;
			if(size > 0) {
				sneu = (char *)guarded_malloc(size + 1);
				read(sneu, size);
				sneu[size] = '\0';
			}
			if(s) {
				free(const_cast<char *>(s));
			}
			s = sneu;
		}
	} else {
		if(saving) {
			fprintf(fp, "%s\n", s ? s : null_s);
		}
		else {
			char buffer[256];

			fgets(buffer, 255, fp);
			buffer[strlen(buffer)-1] = 0;

			if(s) {
				free(const_cast<char *>(s));
			}
			if(buffer[0] != 0 && strcmp(buffer, null_s)) {
				s = strdup(buffer);
			}
			else {
				s = NULL;
			}
		}
	}
}

void loadsave_t::rdwr_str(char *s, int /*size*/)
{
	if(!is_text()) {
		sint16 len;
		if(saving) {
			len = strlen(s);
#ifdef BIG_ENDIAN
			{
				sint16 ii = (sint16)endian_uint16((uint16 *)&len);
				write(&ii, sizeof(sint16));
			}
#else
			write(&len, sizeof(short));
#endif
			write(s, len);
		}
		else {
			read(&len, sizeof(short));
#ifdef BIG_ENDIAN
			len = (sint16)endian_uint16((uint16 *)&len);
#endif
			//assert(len < size);
			read(s, len);
			s[len] = '\0';
		}
	}
	else {
		if(saving) {
			fprintf(fp, "%s\n", s);
		}
		else {
			char buffer[256];

			fgets(buffer, 255, fp);
			buffer[strlen(buffer)-1] = 0;
			//assert(strlen(buffer) < size);
			strcpy(s, buffer);
		}
	}
}


void loadsave_t::rdwr_double(double &dbl, const char *delim)                //01-Dec-01     Markus Weber    Added
{
	if(!is_text()) {
		if(saving) {
			write(&dbl, sizeof(double));
		} else {
			read(&dbl, sizeof(double));
		}
	} else {
		if(saving) {
			fprintf(fp, "%f", dbl);
		} else {
			fscanf(fp, "%lf", &dbl);
		}
	}
	rdwr_delim(delim);
}


void loadsave_t::wr_obj_id(sint16 id)
{
	if(saving) {
		if(!is_text()) {
			uint8 idc = (uint8)id;
			write(&idc, sizeof(uint8));
		} else {
			fprintf(fp, "%d\n", id);
		}
	}
}


sint16 loadsave_t::rd_obj_id()
{
	sint16   id;
	if(!saving) {
		if(!is_text()) {
			sint8 idc;
			read(&idc, sizeof(sint8));
			id = (sint8)idc;
		} else {
			if(fgetc(fp) == 'N') {
				fgetc(fp); // '\n' lesen
				id = -1;
			}
			else {
				fseek(fp, -1, SEEK_CUR);
				fscanf(fp, "%hd\n", &id);
			}
		}
	}
	return id;
}


void loadsave_t::wr_obj_id(const char *id_text)
{
	if(saving) {
		if(is_zipped()) {
			gzprintf(fp, "%s\n", id_text);
		} else {
			fprintf(fp, "%s\n", id_text);
		}
	}
}


void loadsave_t::rd_obj_id(char *id_buf, int size)
{
	if(!saving) {
		if(is_zipped()) {
			gzgets(fp, id_buf, size);
		} else {
			fgets(id_buf, size, fp);
		}
		id_buf[strlen(id_buf) - 1] = '\0';
	}
}


void loadsave_t::rdwr_delim(const char *delim)
{
	if(is_text()) {
		if(saving) {
			fprintf(fp, "%s", delim);
		} else {
			fscanf(fp, delim);
		}
	}
}


uint32 loadsave_t::int_version(const char *version_text, mode_t *mode, char *pak_extension)
{
	// major number (0..)
	uint32 v0 = atoi(version_text);
	while(*version_text && *version_text++ != '.')
		;
	if(!*version_text) {
		return -1;
	}

	// middle number (.99.)
	uint32 v1 = atoi(version_text);
	while(*version_text && *version_text++ != '.')
		;
	if(!*version_text) {
		return -1;
	}

	// minor number (..08)
	uint32 v2 = atoi(version_text);
	uint32 version = v0 * 1000000 + v1 * 1000 + v2;

	if(mode) {
		while(*version_text && *version_text != 'b' && *version_text != 'z') {
			version_text++;
		}
		if(!strncmp(version_text, "bin", 3)) {
			*mode = binary;
		} else if(!strncmp(version_text, "zip", 3)) {
			*mode = zipped;
		} else {
			*mode = text;
		}

		// also pak extension was saved
		if(version>=99008) {
			if(*mode!=text) {
				version_text += 3;
			}
			while(*version_text && *version_text>=32) {
				*pak_extension++ = *version_text++;
			}
		}
		*pak_extension = 0;
	}

	return version;
}


/**
 * Read string into preallocated buffer.
 * @author Hj. Malthaner
 */
void loadsave_t::rd_str_into(char *s, const char * /*null_s*/)
{
	sint16 size;
	read(&size, sizeof(sint16));
#ifdef BIG_ENDIAN
	size = (sint16)endian_uint16((uint16 *)&size);
#endif
	if(size > 0) {
		read(s, size);
		s[size] = '\0';
	}
	else {
		s[0] = '\0';
	}
}
