#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../simtypes.h"
#include "../simversion.h"
#include "../simmem.h"
#include "loadsave.h"

#include "../utils/simstring.h"

loadsave_t::mode_t loadsave_t::save_mode = binary;	// default to use for saving

loadsave_t::loadsave_t() :
	filename()
{
	assert(sizeof(int) == 4);
	assert(sizeof(short) == 2);
	assert(sizeof(long) == 4);
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

	if(strncmp(buf, SAVEGAME_PREFIX, sizeof(SAVEGAME_PREFIX) - 1)) {
		close();
		return false;
	}
	version = int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, &mode);
	if(mode != zipped) {
		gzclose(fp);
		fp = fopen(filename, "rb");
		fgets(buf, 80, fp);
	}
	this->filename = filename;
	return true;
}

bool loadsave_t::wr_open(const char *filename, mode_t m)
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

	if(is_zipped()) {
		gzprintf(fp, "%s%s\n", SAVEGAME_VERSION, "zip");
	}
	else {
		fprintf(fp, "%s%s\n", SAVEGAME_VERSION, mode == binary ? "bin" : "");
	}

	version = int_version(SAVEGAME_VER_NR, NULL);
	this->mode = mode;
	this->filename = filename;

	return true;
}


void loadsave_t::close()
{
	if(fp != NULL) {
		if(is_zipped()) {
			gzclose (fp);
		}
		else {
			fclose(fp);
		}
		fp = NULL;
	}
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

long loadsave_t::read (void *buf, unsigned long len)
{
    if(is_zipped()) {
	return gzread(fp, buf, len);
    } else {
	return (long)fread(buf, 1, len, fp);
    }
}


/* read data types (should check also for Intel/Motorola) etc */
void loadsave_t::rdwr_byte(signed char &c, const char *delim)
{
    if(saving) {
        lsputc(c);
    } else {
	c = (signed char)lsgetc();
    }
    rdwr_delim(delim);
}

void loadsave_t::rdwr_byte(unsigned char &c, const char *delim)
{
	signed char cc=c;
	rdwr_byte(cc,delim);
	c = (unsigned char)cc;
}


/* shorts */
void loadsave_t::rdwr_short(short &i, const char *delim)
{
    if(!is_text()) {

	if(saving) {
	    write(&i, sizeof(short));
        } else {
	    read(&i, sizeof(short));
	}
    } else {
	if(saving) {
	    fprintf(fp, "%d", i);
        } else {
	    fscanf(fp, "%hd", &i);
	}
    }
    rdwr_delim(delim);
}

void loadsave_t::rdwr_short(unsigned short &i, const char *delim)
{
	short ii=i;
	rdwr_short(ii,delim);
	i = (unsigned short)ii;
}


/* long words*/
void loadsave_t::rdwr_long(long &l, const char *delim)
{
    if(!is_text()) {
	if(saving) {
	    write(&l, sizeof(long));
        } else {
	    read(&l, sizeof(long));
	}
    } else {
	if(saving) {
    	    fprintf(fp, "%ld", l);
        } else {
	    fscanf(fp, "%ld", &l);
	}
    }
    rdwr_delim(delim);
}

void loadsave_t::rdwr_long(unsigned long &l, const char *delim)
{
	long ll=l;
	rdwr_long(ll,delim);
	l = (unsigned long)l;
}

void loadsave_t::rdwr_long(int &l, const char *delim)
{
	long ll = l;
	rdwr_long(ll,delim);
	l = (int)ll;
}

void loadsave_t::rdwr_long(unsigned &l, const char *delim)
{
	long ll = l;
	rdwr_long(ll,delim);
	l = (unsigned)ll;
}

/* long long (64 Bit) */
void loadsave_t::rdwr_longlong(sint64 &ll, const char *delim)
{
    if(!is_text()) {
	if(saving) {
	    write(&ll, sizeof(sint64));
        } else {
	    read(&ll, sizeof(sint64));
	}
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
	short size;
	if(saving) {
            size = s ? strlen(s) : 0;
	    write(&size, sizeof(short));
            if(size > 0) {
	        write(s, size);
            }
	} else {
	    read(&size, sizeof(short));
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
	short len;
	if(saving) {
	    len = strlen(s);
	    write(&len, sizeof(short));
	    write(s, len);
	} else {
	    read(&len, sizeof(short));
            //assert(len < size);
	    read(s, len);
	    s[len] = '\0';
	}
    } else {
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

void loadsave_t::wr_obj_id(short id)
{
    if(saving) {
	if(!is_text()) {
            char idc = (char)id;
	    write(&idc, sizeof(char));
        } else {
	    fprintf(fp, "%d\n", id);
        }
    }
}

short loadsave_t::rd_obj_id()
{
    short   id;

    if(!saving) {
	if(!is_text()) {
            char idc;
	    read(&idc, sizeof(char));
            id = idc;
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

int loadsave_t::int_version(const char *version_text, mode_t *mode)
{
    int v0 = atoi(version_text);

    while(*version_text && *version_text++ != '.');
    if(!*version_text) {
        return -1;
    }
    int v1 = atoi(version_text);
    while(*version_text && *version_text++ != '.');
    if(!*version_text) {
        return -1;
    }
    int v2 = atoi(version_text);
    if(mode) {
        while(*version_text && *version_text != 'b' && *version_text != 'z')
	    version_text++;
        if(!strncmp(version_text, "bin", 3)) {
	    *mode = binary;
        } else if(!strncmp(version_text, "zip", 3)) {
	    *mode = zipped;
	} else {
	    *mode = text;
	}
    }
    return v0 * 1000000 + v1 * 1000 + v2;
}


/**
 * Read string into preallocated buffer.
 * @author Hj. Malthaner
 */
void loadsave_t::rd_str_into(char *s, const char * /*null_s*/)
{
    short size;
    read(&size, sizeof(short));

    if(size > 0) {
	read(s, size);
    }
    s[size] = '\0';
}
