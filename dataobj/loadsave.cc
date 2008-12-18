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

loadsave_t::mode_t loadsave_t::save_mode = zipped;	// default to use for saving

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
		if(strncmp(buf, XML_SAVEGAME_PREFIX, sizeof(XML_SAVEGAME_PREFIX)-1)!=0) {
			close();
			return false;
		}
		else {
			mode = xml|zipped;
			while(  lsgetc()!='<'  );
			read( buf, sizeof(SAVEGAME_PREFIX) - 1 );
			if(  strncmp(buf, SAVEGAME_PREFIX, sizeof(SAVEGAME_PREFIX) - 1)  ) {
				close();
				// not a simutrans XML file ...
				return false;
			}

			read( buf, sizeof("version=\"")-1 );
			char str[256];
			char *s = str;
			for(  int i=0;  i<255;  i++ ) {
				char c = lsgetc();
				if(c=='\"') {
					break;
				}
				*s++ = c;
			}
			*s = 0;
			int dummy;
			version = int_version(str, &dummy, pak_extension);

			read( buf, sizeof(" pak=\"")-1 );
			s = pak_extension;
			for(  int i=0;  i<63;  i++ ) {
				char c = lsgetc();
				if(c=='\"') {
					break;
				}
				*s++ = c;
			}
			*s = 0;
			while(  lsgetc()!='>'  );
		}
	}
	else {
		version = int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, &mode, pak_extension);
	}
	if(mode==text) {
		close();
		dbg->error("loadsave_t::rd_open()","text mode no longer supported." );
		return false;
	}
	mode |= zipped;

	/*
	if(mode != zipped) {
		gzclose(fp);
		fp = fopen(filename, "rb");
		fgets(buf, 80, fp);
	}
	*/
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

	if(  mode & zipped  ) {
		fp = (FILE *)gzopen(filename, "wb");
	}
	else if(  mode & binary ) {
		fp = fopen(filename, "wb");
	}
	else {
		fp = fopen(filename, "w");
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

	if(  !is_xml()  ) {
		if(is_zipped()) {
			gzprintf(fp, "%s%s%s\n", SAVEGAME_VERSION, "zip", this->pak_extension);
		}
		else {
			fprintf(fp, "%s%s%s\n", SAVEGAME_VERSION, mode == binary ? "bin" : "", this->pak_extension);
		}
	}
	else {
		char str[4096];
		int n = sprintf( str, "<?xml version=\"1.0\"?>\n<Simutrans version=\"%s\" pak=\"%s\">\n", SAVEGAME_VER_NR, this->pak_extension );
		write( str, n );
		ident = 1;
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
		if(  is_xml()  ) {
			const char *end = "\n</Simutrans>\n";
			write( end, strlen(end) );
		}
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
void loadsave_t::rdwr_byte(sint8 &c, const char *)
{
	if(!is_xml()) {
		if(saving) {
			lsputc(c);
		}
		else {
			c = (sint8)lsgetc();
		}
	}
	else {
		sint64 ll = c;
		rdwr_xml_number( ll, "i8" );
		c = ll;
	}
}

void loadsave_t::rdwr_byte(uint8 &c, const char *)
{
	sint8 cc=c;
	rdwr_byte(cc,NULL);
	c = (uint8)cc;
}


/* shorts */
void loadsave_t::rdwr_short(sint16 &i, const char *)
{
	if(!is_xml()) {
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
	}
	else {
		sint64 ll = i;
		rdwr_xml_number( ll, "i16" );
		i = ll;
	}
}

void loadsave_t::rdwr_short(uint16 &i, const char *)
{
	sint16 ii=i;
	rdwr_short(ii,NULL);
	i = (uint16)ii;
}


/* long words*/
void loadsave_t::rdwr_long(sint32 &l, const char *)
{
	if(!is_xml()) {
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
	}
	else {
		sint64 ll = l;
		rdwr_xml_number( ll, "i32" );
		l = ll;
	}
}

void loadsave_t::rdwr_long(uint32 &l, const char *)
{
	sint32 ll=l;
	rdwr_long(ll,NULL);
	l = (uint32)ll;
}

/* long long (64 Bit) */
void loadsave_t::rdwr_longlong(sint64 &ll, const char *)
{
	if(!is_xml()) {
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
	}
	else {
		rdwr_xml_number( ll, "i64" );
	}
}



void loadsave_t::rdwr_double(double &dbl)
{
	if(!is_xml()) {
		if(saving) {
			write(&dbl, sizeof(double));
		} else {
			read(&dbl, sizeof(double));
		}
	}
	else {
		// so far only with 3 digit precision, but this is ok for only two locations used
		sint64 ll= (sint64)((dbl*1000.0)+0.5);
		rdwr_xml_number( ll, "d1000" );
		dbl = ((double)ll)/1000.0;
	}
}



void loadsave_t::rdwr_bool(bool &i, const char *)
{
	if(  !is_xml()  ) {
		if(saving) {
			lsputc(i ? '1' : '0');
		}
		else {
			i = lsgetc()=='1';
		}
	}
	else {
		// boll xml
		if(saving) {
			if(is_zipped()) {
				gzprintf(fp, "%*s<bool>%s</bool>\n", ident, "", i ? "true" : "false" );
			} else {
				fprintf(fp, "%*s<bool>%s</bool>\n", ident, "", i ? "true" : "false" );
			}
		}
		else {
			// find start of tag
			while(  lsgetc()!='<'  );
			// check for correct tag
			char buffer[7];
			read( buffer, 5 );
			buffer[5] = 0;
			if(  strcmp("bool>",buffer)!=0  ) {
				dbg->fatal( "loadsave_t::rdwr_str()","expected \"<bool>\", got \"<%s\"", buffer );
			}
			read( buffer, 4 );
			buffer[4] = 0;
			i = strcmp("true",buffer)==0;
			while(  lsgetc()!='<'  );
			read( buffer, 6 );
			buffer[6] = 0;
			if(  strcmp("/bool>",buffer)!=0  ) {
				dbg->fatal( "loadsave_t::rdwr_str()","expected \"</bool>\", got \"<%s\"", buffer );
			}
		}
	}
}



void loadsave_t::rdwr_xml_number(sint64 &s, const char *typ)
{
	if(saving) {
		static char nr[256];
		sprintf( nr, "%*s<%s>%.0lf</%s>\n", ident, "", typ, (double)s, typ );
		this->write( nr, strlen(nr) );
	}
	else {
		const int len = (int)strlen(typ);
		assert(len<256);
		// find start of tag
		while(  lsgetc()!='<'  );
		// check for correct tag
		char buffer[256];
		read( buffer, len );
		buffer[len] = 0;
		if(  strcmp(typ,buffer)!=0  ) {
			dbg->fatal( "loadsave_t::rdwr_str()","expected \"<%s>\", got \"<%s>\"", typ, buffer );
		}
		while(  lsgetc()!='>'  );
		// read number;
		s = 0;
		bool minus = false;
		while(!is_eof()) {
			char c = lsgetc();
			if(c>='0'  &&  c<='9'  ) {
				s = (s*10)+(c-'0');
			}
			else {
				if(s==0) {
					if(  c=='-') {
						minus = true;
						continue;
					}
					else if(c=='+') {
						minus = false;
						continue;
					}
				}
				if(c==' ') {
					while(  lsgetc()!='<'  );
					break;
				}
				else if(c=='<') {
					break;
				}
				else {
					dbg->fatal( "loadsave_t::rdwr_xml_number()", "type %s, found %c in number!", typ, c );
				}
			}
		}
		if(minus) {
			s = -s;
		}
		if(  lsgetc()!='/'  ) {
			dbg->fatal( "loadsave_t::rdwr_xml_number()", "missing '/' (not closing tag)" );
		}
		read( buffer, len );
		buffer[6] = 0;
		if(  strcmp(typ,buffer)!=0  ) {
			dbg->fatal( "loadsave_t::rdwr_str()","expected \"</%s>\", got \"</%s>\"", typ, buffer );
		}
		while(  lsgetc()!='>'  );
	}
}




// s is a malloc-ed string (will be freed and newly allocated on load time!)
void loadsave_t::rdwr_str(const char *&s)
{
	if(!is_xml()) {
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
				sneu = MALLOCN(char, size + 1);
				read(sneu, size);
				sneu[size] = '\0';
			}
			if(s) {
				free(const_cast<char *>(s));
			}
			s = sneu;
		}
	}
	else {
		// use CDATA tag: <![CDATA[%s]]>
		if(saving) {
			const char *str = s ? s: "";
			if(is_zipped()) {
				gzprintf(fp, "%*s<![CDATA[%s]]>\n", ident, "", str );
			} else {
				fprintf(fp, "%*s<![CDATA[%s]]>\n", ident, "", str );
			}
		}
		else {
			char buffer[4096];
			rdwr_str( buffer, 4096 );
			if(s) {
				free(const_cast<char *>(s));
			}
			s = buffer[0]!=0 ? strdup(buffer) : NULL;
		}
	}
}



// read a string into a preallocated buffer
void loadsave_t::rdwr_str(char *s, int size)
{
	if(!is_xml()) {
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
			if(len>size) {
				dbg->fatal( "loadsave_t::rdwr_str()","string longer (%i) than allowed size (%i)", len, size );
			}
			read(s, len);
			s[len] = '\0';
		}
	}
	else {
		// use CDATA tag: <![CDATA[%s]]>
		if(saving) {
			const char *str = s ? s: "";
			if(is_zipped()) {
				gzprintf(fp, "%*s<![CDATA[%s]]>\n", ident, "", str );
			} else {
				fprintf(fp, "%*s<![CDATA[%s]]>\n", ident, "", str );
			}
		}
		else {
			// find start of tag
			while(  lsgetc()!='<'  );
			// check for correct tag
			char buffer[10];
			read( buffer, 7 );
			bool string = true;
			if(  strncmp("string>",buffer,7)!=0  ) {
				if(  strncmp("![CDATA",buffer,7)!=0  ||  lsgetc()!='['  ) {
					buffer[7] = 0;
					dbg->fatal( "loadsave_t::rdwr_str()","expected str \"<![CDATA[\", got \"%s\"", buffer );
				}
				string = false;
			}
			// now parse input
			if(string) {
				const char *ptr = NULL;
				for(  int i=0;  i<size;  i++  ) {
					char c = lsgetc();
					if(  c=='<'  ) {
						ptr = s;
					}
					if(  c=='>'  ) {
						if(  i>=8  &&  strncmp( s-8, "</string>", 8 )==0  ) {
							s[-8] = 0;
							ptr = s-8;
							break;
						}
					}
					*s++ = c;
				}
				*s = 0;
				// go until closing
				if(  ptr==0  ||  *ptr!=0  ) {
					while(  lsgetc()!='>'  );
				}
			}
			else {
				char last_three_chars[4];
				char len = 0;
				for(  int i=0;  i<size;  i++  ) {
					char c = lsgetc();
					if(  c==']'  &&  (  len==0  ||  (len==1  &&  last_three_chars[0] == ']') )  ) {
						last_three_chars[len++] = c;
					}
					else if(  c=='>'  &&  len==2  ) {
						len ++;
						break;
					}
					else {
						// evt. add closing brackets
						while(  len-->0  ) {
							*s++ = ']';
						}
						len = 0;
						*s++ = c;
					}
				}
				*s = 0;
				if(  len!=3  ) {
					dbg->fatal( "loadsave_t::rdwr_str()","string too long (exceeded %i characters)", size );
				}
			}
		}
	}
}


void loadsave_t::wr_obj_id(sint16 id)
{
	if(saving) {
		if(!is_xml()) {
			uint8 idc = (uint8)id;
			write(&idc, sizeof(uint8));
		}
		else {
			sint64 ll=id;
			rdwr_xml_number( ll, "id" );
		}
	}
}


sint16 loadsave_t::rd_obj_id()
{
	sint16 id;
	if(!saving) {
		if(!is_xml()) {
			sint8 idc;
			read(&idc, sizeof(sint8));
			id = (sint8)idc;
		}
		else {
			sint64 ll;
			rdwr_xml_number( ll, "id" );
			return (sint16)ll;
		}
	}
	return id;
}


void loadsave_t::wr_obj_id(const char *id_text)
{
	if(saving) {
		if(  !is_xml()  ) {
			if(is_zipped()) {
				gzprintf(fp, "%s\n", id_text);
			} else {
				fprintf(fp, "%s\n", id_text);
			}
		}
		else {
			if(is_zipped()) {
				gzprintf(fp, "<id=\"%s\"/>\n", id_text);
			} else {
				fprintf(fp, "<id=\"%s\"/>\n", id_text);
			}
		}
	}
}


void loadsave_t::rd_obj_id(char *id_buf, int size)
{
	if(!saving) {
		if(  !is_xml()  ) {
			if(is_zipped()) {
				gzgets(fp, id_buf, size);
			}
			else {
				fgets(id_buf, size, fp);
			}
			id_buf[strlen(id_buf) - 1] = '\0';
		}
		else {
			char buf[6];
			// find start of tag
			while(  lsgetc()!='<'  );
			read( buf, 6 );
			buf[5] = 0;
			if(  strncmp(buf,"<id=\"",5)!=0  ) {
				dbg->fatal( "loadsave_t::rd_obj_id()","expected id str \"<id=\"\", got \"%s\"", buf );
			}
			// now parse input
			for(  int i=0;  i<size;  i++  ) {
				char c = lsgetc();
				if(  c=='\"'  ) {
					break;
				}
				else {
					*id_buf++ = c;
				}
			}
			*id_buf = 0;
			read( buf, 2 );
			if(  strncmp(buf,"/>",2)!=0  ) {
				dbg->fatal( "loadsave_t::rd_obj_id()","id tag not properly closed!" );
			}
		}
	}
}


uint32 loadsave_t::int_version(const char *version_text, int *mode, char *pak_extension)
{
	// major number (0..)
	uint32 v0 = atoi(version_text);
	while(*version_text && *version_text++ != '.')
		;
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		return 0;
	}

	// middle number (.99.)
	uint32 v1 = atoi(version_text);
	while(*version_text && *version_text++ != '.')
		;
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		return 0;
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



void loadsave_t::start_tag(const char *tag)
{
	if(  is_xml()  ) {
		if(saving) {
			if(is_zipped()) {
				gzprintf(fp, "%*s<%s>\n", ident, "", tag);
			} else {
				fprintf(fp, "%*s<%s>\n", ident, "", tag);
			}
			ident ++;
		}
		else {
			char buf[256];
			const int len = strlen(tag);
			// find start of tag
			while(  lsgetc()!='<'  );
			read( buf, len );
			if(  strncmp(buf,tag,len)!=0  ) {
				dbg->fatal( "loadsave_t::start_tag()","expected \"%s\", got \"%s\"", tag, buf );
			}
			while(  lsgetc()!='>'  );
		}
	}
}


void loadsave_t::end_tag(const char *tag)
{
	if(  is_xml()  ) {
		if(saving) {
			ident --;
			if(is_zipped()) {
				gzprintf(fp, "%*s</%s>\n", ident, "", tag);
			} else {
				fprintf(fp, "%*s</%s>\n", ident, "", tag);
			}
		}
		else {
			// just use start tag with the end character ...
			char buf[256];
			tstrncpy( buf+1, tag, 254 );
			buf[0] = '/';
			start_tag(buf);
		}
	}
}


