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

loadsave_t::mode_t loadsave_t::save_mode = bzip2;	// default to use for saving

loadsave_t::loadsave_t(bool experimental) : filename()
{
	fp = NULL;
	save_experimental = experimental;
}

loadsave_t::loadsave_t() : filename()
{
	mode = 0;
	saving = false;
	fp = NULL;
	save_experimental = true;
	bzfp = NULL;
	bse = BZ_OK+1;
}

loadsave_t::~loadsave_t()
{
	close();
}

bool loadsave_t::rd_open(const char *filename)
{
	close();

	version = 0;
	mode = zipped;
	experimental_version = 0;
	fp = fopen(filename, "rb");
	if(  fp==NULL  ) {
		// most likely not existing
		return false;
	}
	// now check for BZ2 format
	char buf[80];
	if(  fread( buf, 1, 80, fp )==80  ) {
		if(  buf[0]=='B'  &&  buf[1]=='Z'  ) {
			mode = bzip2;
		}
		fseek(fp,0,SEEK_SET);
	}

	if(  mode==bzip2  ) {
		bse = BZ_OK+1;
		bzfp = NULL;
		bzfp = BZ2_bzReadOpen( &bse, fp, 0, 0, NULL, 0 );
		bool ok = false;
		if(  bse==BZ_OK  ) {
			// else: use zlib
			MEMZERO(buf);
			if(  BZ2_bzRead( &bse, bzfp, buf, sizeof(SAVEGAME_PREFIX) )==sizeof(SAVEGAME_PREFIX)  &&  bse==BZ_OK  ) {
				// get the rest of the string
				for(  int i=sizeof(SAVEGAME_PREFIX);  buf[i-1]>=32  &&  i<79;  i++  ) {
					buf[i] = lsgetc();
				}
				ok = bse==BZ_OK;
			}
		}
		// BZ-Header but wrong data ...
		if(  !ok  ) {
			close();
			return false;
		}
	}

	if(  mode!=bzip2  ) {
		fclose(fp);
		// and now with zlib ...
		fp = (FILE *)gzopen(filename, "rb");
		if(fp==NULL) {
			return false;
		}
		gzgets(fp, buf, 80);
	}
	saving = false;

	if(strncmp(buf, SAVEGAME_PREFIX, sizeof(SAVEGAME_PREFIX) - 1)) {
		if(strncmp(buf, XML_SAVEGAME_PREFIX, sizeof(XML_SAVEGAME_PREFIX)-1)!=0) {
			close();
			return false;
		}
		else {
			mode = xml|zipped;
			while(  lsgetc()!='<'  ) { /* nothing */ }
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
			
			combined_version versions = int_version(str, &dummy, pak_extension);
			version = versions.version;
			experimental_version = versions.experimental_version;

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
			while(  lsgetc()!='>'  )  ;
		}
	}
	else {
		combined_version versions = int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, &mode, pak_extension);
		version = versions.version;
		experimental_version = versions.experimental_version;
	}
	if(mode==text) {
		close();
		dbg->error("loadsave_t::rd_open()","text mode no longer supported." );
		return false;
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

	if(  is_zipped()  ) {
		// using zlib
		fp = (FILE *)gzopen(filename, "wb");
	}
	else if(  mode==binary  ) {
		// no compression
		fp = fopen(filename, "wb");
		if(  is_bzip2()  ) {
			// the additional magic for bzip2
			bse = BZ_OK+1;
			bzfp = NULL;
			if(  fp  ) {
				bzfp = BZ2_bzWriteOpen( &bse, fp, 9, 0, 30 /* default is 30 */ );
				if(  bse!=BZ_OK  ) {
					return false;
				}
			}
		}
	}
	else if(  is_bzip2()  ) {
		// XML or bzip ...
		fp = fopen(filename, "wb");
		// the additional magic for bzip2
		bse = BZ_OK+1;
		bzfp = NULL;
		if(  fp  ) {
			bzfp = BZ2_bzWriteOpen( &bse, fp, 9, 0, 30 /* default is 30 */ );
			if(  bse!=BZ_OK  ) {
				return false;
			}
		}
	}
	else {
		// uncompressed xml should be here ...
		assert(  mode==xml  );
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
	
	// Use Experimental version numbering if appropriate.
	char *savegame_version;
	char *savegame_ver_nr;
	if(save_experimental)
	{
		savegame_version = EXPERIMENTAL_SAVEGAME_VERSION;
		savegame_ver_nr = COMBINED_VER_NR;
	}
	else
	{
		savegame_version = SAVEGAME_VERSION;
		savegame_ver_nr = SAVEGAME_VER_NR;
	}

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

	loadsave_t::combined_version combined_version = int_version(SAVEGAME_VER_NR, NULL, NULL );
	version = combined_version.version;

	if(  !is_xml()  ) {
		char str[4096];
		size_t len;
		if(  version<102002  ) {
			len = sprintf( str, "%s%s%s\n", savegame_version, "zip", this->pak_extension );
		}
		else {
			len = sprintf( str, "%s-%s\n", savegame_version, this->pak_extension );
		}
		write( str, len );
	}
	else {
		char str[4096];
		int n = sprintf( str, "<?xml version=\"1.0\"?>\n<Simutrans version=\"%s\" pak=\"%s\">\n", savegame_ver_nr, this->pak_extension );
		write( str, n );
		ident = 1;
	}

	loadsave_t::combined_version versions = int_version(savegame_ver_nr, NULL, NULL );
	version = versions.version;
	experimental_version = versions.experimental_version;

	this->mode = mode;
	this->filename = filename;

	return true;
}


const char *loadsave_t::close()
{
	const char *success = NULL;
	if(fp != NULL) {
		if(  is_xml()  &&  saving  &&  fp!=NULL  &&  (!is_bzip2()  ||  bse==BZ_OK)  ) {
			// only write when close and no error occurred
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
		else if(  is_bzip2()  ) {
			if(   saving  ) {
				BZ2_bzWriteClose( &bse, bzfp, 0, NULL, NULL );
			}
			else {
				BZ2_bzReadClose( &bse, bzfp );
			}
			fclose( fp );
			bzfp = fp = NULL;
			bse = BZ_STREAM_END;
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


/************* from here on the actual data in/out routines ****************/

/**
 * Checks end-of-file
 * @author Hj. Malthaner
 */
bool loadsave_t::is_eof()
{
	if(is_bzip2()) {
		// any error is EOF ...
		return bse!=BZ_OK;
	}
	else {
		return gzeof(fp) != 0;
	}
}


void loadsave_t::lsputc(int c)
{
	if(is_zipped()) {
		gzputc(fp, c);
	}
	else if(is_bzip2()) {
		uint8 ch = c;
		write( &ch, 1 );
	}
	else {
		fputc(c, fp);
	}
}

int loadsave_t::lsgetc()
{
	if(is_bzip2()) {
		uint8 c[2];
		if(  bse==BZ_OK  ) {
			BZ2_bzRead( &bse, bzfp, c, 1);
		}
		return bse==BZ_OK ? c[0] : -1;
	}
	else {
		return gzgetc(fp);
	}
}

long loadsave_t::write(const void *buf, size_t len)
{
	if(is_zipped()) {
		return gzwrite(fp, const_cast<void *>(buf), len);
	}
	else if(is_bzip2()) {
		BZ2_bzWrite( &bse, bzfp, const_cast<void *>(buf), len);
		assert(bse==BZ_OK);
		return (long)len;
	}
	else {
		return (long)fwrite(buf, 1, len, fp);
	}
}

long loadsave_t::read(void *buf, size_t len)
{
	if(is_bzip2()) {
		if(  bse==BZ_OK  ) {
			BZ2_bzRead( &bse, bzfp, const_cast<void *>(buf), len);
		}
		// little trick: zero if not ok ...
		return (long)len&~(bse-BZ_OK);
	}
	else {
		return gzread(fp, buf, len);
	}
}


/*************** High level routines to read/write data types *************
 * (check also for Intel/Motorola) etc
 */



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


void loadsave_t::rdwr_short(sint16 &i, const char *)
{
	if(!is_xml()) {
		if (saving) {
#ifdef SIM_BIG_ENDIAN
			sint16 ii = endian(i);
			write(&ii, sizeof(sint16));
#else
			write(&i, sizeof(sint16));
#endif
		} else {
#ifdef SIM_BIG_ENDIAN
			uint16 ii;
			read(&ii, sizeof(sint16));
			i = endian(ii);
#else
			read(&i, sizeof(sint16));
#endif
		}
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


void loadsave_t::rdwr_long(sint32 &l, const char *)
{
	if(!is_xml()) {
		if (saving) {
#ifdef SIM_BIG_ENDIAN
			uint32 ii = endian(l);
			write(&ii, sizeof(uint32));
#else
			write(&l, sizeof(sint32));
#endif
		} else {
#ifdef SIM_BIG_ENDIAN
			uint32 ii;
			read(&ii, sizeof(uint32));
			l = endian(ii);
#else
			read(&l, sizeof(sint32));
#endif
		}
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


void loadsave_t::rdwr_longlong(sint64 &ll, const char *)
{
	if(!is_xml()) {
		if (saving) {
#ifdef SIM_BIG_ENDIAN
			sint64 ii = endian(ll);
			write(&ii, sizeof(sint64));
#else
			write(&ll, sizeof(sint64));
#endif
		} else {
#ifdef SIM_BIG_ENDIAN
			uint64 ii;
			read(&ii, sizeof(sint64));
			ll = endian(ii);
#else
			read(&ll, sizeof(sint64));
#endif
		}
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
		}
		else {
			read(&dbl, sizeof(double));
		}
	}
	else {
		// so far only with 3 digit precision, but this is ok for only two locations used
		sint64 ll= (sint64)((dbl*1000.0)+0.5);
		rdwr_xml_number( ll, "d1000" );
		dbl = (((double)ll)+0.000001)/1000.0;
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
		// bool xml
		if(saving) {
			write( "                                                                ", min(64,ident) );
			if(  i  ) {
				write( "<bool>true</bool>\n", sizeof("<bool>true</bool>\n")-1 );
			}
			else {
				write( "<bool>false</bool>\n", sizeof("<bool>false</bool>\n")-1 );
			}
		}
		else {
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
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
			while(  lsgetc()!='<'  ) { /* nothing */ }
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
		size_t len = sprintf( nr, "%*s<%s>%.0f</%s>\n", ident, "", typ, (double)s, typ );
		write( nr, len );
	}
	else {
		const int len = (int)strlen(typ);
		assert(len<256);
		// find start of tag
		while(  lsgetc()!='<'  ) { /* nothing */ }
		// check for correct tag
		char buffer[256];
		read( buffer, len );
		buffer[len] = 0;
		if(  strcmp(typ,buffer)!=0  ) {
			dbg->fatal( "loadsave_t::rdwr_str()","expected \"<%s>\", got \"<%s>\"", typ, buffer );
		}
		while(  lsgetc()!='>'  )  ;
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
					while(  lsgetc()!='<'  ) { /* nothing */ }
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
		while(  lsgetc()!='>'  )  ;
	}
}


// s is a malloc-ed string (will be freed and newly allocated on load time!)
void loadsave_t::rdwr_str(const char *&s)
{
	if(!is_xml()) {
		sint16 size;
		if(saving) {
			size = s ? (sint16)min(32767,strlen(s)) : 0;
#ifdef SIM_BIG_ENDIAN
			{
				uint16 ii = endian(size);
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
#ifdef SIM_BIG_ENDIAN
			{
				uint16 ii;
				read(&ii, sizeof(uint16));
				size = endian(ii);
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
			write( "                                                                ", min(64,ident) );
			write( "<![CDATA[", 9 );
			if(s) {
				write( s, strlen(s) );
			}
			write( "]]>\n", 4 );
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
void loadsave_t::rdwr_str(char* s, size_t const size)
{
	if(!is_xml()) {
		uint16 len;
		if(saving) {
			len = (uint16)min(32767,strlen(s));
#ifdef SIM_BIG_ENDIAN
			{
				sint16 ii = endian(len);
				write(&ii, sizeof(sint16));
			}
#else
			write(&len, sizeof(uint16));
#endif
			write(s, len);
		}
		else {
			read(&len, sizeof(uint16));
			len = endian(len);
			if(  len >= size) {
				dbg->fatal( "loadsave_t::rdwr_str()","string longer (%i) than allowed size (%i)", len, size );
			}
			read(s, len);
			s[len] = '\0';
		}
	}
	else {
		// use CDATA tag: <![CDATA[%s]]>
		if(saving) {
			write( "                                                                ", min(64,ident) );
			write( "<![CDATA[", 9 );
			if(s) {
				write( s, strlen(s) );
			}
			write( "]]>\n", 4 );
		}
		else {
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
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
				for(  size_t i=0;  i<size;  i++  ) {
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
					while(  lsgetc()!='>'  )  ;
				}
			}
			else {
				char last_three_chars[4];
				sint8 len = 0;	// maximum is three
				for(  size_t i=0;  i<size;  ) {
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
							i ++;
						}
						len = 0;
						*s++ = c;
						i++;
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


void loadsave_t::start_tag(const char *tag)
{
	if(  is_xml()  ) {
		if(saving) {
			write( "                                                                ", min(64,ident) );
			write( "<", 1 );
			write( tag, strlen(tag) );
			write( ">\n", 2 );
			ident ++;
		}
		else {
			char buf[256];
			const size_t len = strlen(tag);
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
			read( buf, len );
			if(  strncmp(buf,tag,len)!=0  ) {
				dbg->fatal( "loadsave_t::start_tag()","expected \"%s\", got \"%s\"", tag, buf );
			}
			while(  lsgetc()!='>'  )  ;
		}
	}
}


void loadsave_t::end_tag(const char *tag)
{
	if(  is_xml()  ) {
		if(saving) {
			ident --;
			write( "                                                                ", min(64,ident) );
			write( "</", 2 );
			write( tag, strlen(tag) );
			write( ">\n", 2 );
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


void loadsave_t::wr_obj_id(sint16 id)
{
	if(saving) {
		if(!is_xml()) {
			lsputc( id );
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
			id = (sint16)idc;
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
			write( id_text, strlen(id_text) );
			lsputc( 10 );
		}
		else {
			write( "<id=\"", 5 );
			write( id_text, strlen(id_text) );
			write( "\">\n", 3 );
		}
	}
}


void loadsave_t::rd_obj_id(char *id_buf, int size)
{
	if(!saving) {
		if(  !is_xml()  ) {
			int i=0;
			*id_buf = 0;
			while(  i<size  &&  id_buf[i-1]!=10  ) {
				id_buf[i++] = lsgetc();
			}
			id_buf[i-1] = 0;
		}
		else {
			char buf[6];
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
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


loadsave_t::combined_version loadsave_t::int_version(const char *version_text, int *mode, char *pak_extension_str)
{	
	(void) mode; //unused
	uint32 experimental_version = 0;
	// major number (0..)
	uint32 v0 = atoi(version_text);
	while(*version_text && *version_text++ != '.');
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		combined_version dud;
		dud.version = 0;
		dud.experimental_version = 0;
		return dud;
	}

	// middle number (.99.)
	uint32 v1 = atoi(version_text);
	while(*version_text && *version_text++ != '.');
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		combined_version dud;
		dud.version = 0;
		dud.experimental_version = 0;
		return dud;
	}

	// minor number (..08)
	uint32 v2 = atoi(version_text);

	// Experimental version
	uint16 count = 0;
	while(*version_text && *version_text++ != '.')
	{
		count++;
	}
	if(!*version_text) 
	{
		// Decrement the pointer if this is not an Experimental version.
		//*version_text -= count;
		while(count > 0)
		{
			*version_text --;
			count--;
		}
	}
	else
	{
		experimental_version = atoi(version_text);
		while(count > 0)
		{
			*version_text --;
			count--;
		}
	}

	uint32 version = v0 * 1000000 + v1 * 1000 + v2;

	while(  isdigit(*version_text) || *version_text == '.'  ) {
		version_text++;
	}

	if(  version<=102002  ) {
		/* the compression and the mode we determined already ourselves (otherwise we cannot read this
		 * => leave the mode alone but for unknown modes!
		 */
		if(!strncmp(version_text, "bin", 3)) {
			//*mode = binary;
			version_text += 3;
		}
		else if(!strncmp(version_text, "zip", 3)) {
			//*mode = zipped;
			version_text += 3;
		}
		else if(  *version_text  ) {
			// illegal version ...
			version = 999999999;
		}
	}
	else {
		// skip the minus sign
		if (*version_text=='-') {
			version_text++;
		}
	}

	if(  pak_extension_str  ) {
		if(  *version_text  )  {
			// also pak extension was saved
			if(version>=99008) {
				while(  *version_text>=32  ) {
					*pak_extension_str = *version_text;
					pak_extension_str++;
					version_text++;
				}
			}
		}
		*pak_extension_str = 0;
	}
	combined_version loadsave_version;
	loadsave_version.version = version;
	loadsave_version.experimental_version = experimental_version;

	return loadsave_version;
}
