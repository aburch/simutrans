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
#include "../utils/plainstring.h"
#include "loadsave.h"

#include "../utils/simstring.h"

#include <zlib.h>
#include <bzlib.h>

#define INVALID_RDWR_ID (-1)

// buffer size for read/write - bzip2 gains up to 8M for non-threaded, 1M for threaded. binary, zipped ok with 256K or smaller.
#define LS_BUF_SIZE (1024*1024)

#ifdef MULTI_THREAD
#include "../utils/simthread.h"

static pthread_t ls_thread;
static simthread_barrier_t loadsave_barrier;
static pthread_mutex_t loadsave_mutex;

// parameters passed starting a thread
typedef struct{
	loadsave_t *loadsave_routine;
} loadsave_param_t;
static loadsave_param_t ls;


void *loadsave_thread( void *ptr )
{
	loadsave_param_t *lsp = reinterpret_cast<loadsave_param_t *>(ptr);
	int buf = 1;

	while(true) {
		if(  lsp->loadsave_routine->is_saving()  ) {
			// wait to sync with main thread before flushing the buffer
			simthread_barrier_wait(&loadsave_barrier);

			buf = (buf+1)&1;
			if(  lsp->loadsave_routine->get_buf_pos(buf)==0  ) {
				// empty buffer after sync - signal to exit
				break;
			}
			lsp->loadsave_routine->flush_buffer(buf);
		}
		else {
			int res = lsp->loadsave_routine->fill_buffer(buf);
			if(  res != 0  ) {
				// wait to sync with main thread before filling the next buffer
				// in case of error wait once again
				simthread_barrier_wait(&loadsave_barrier);
			}
			if(  res <= 0  ) {
				// nothing read into buffer - exit
				// in case of error leave, too
				break;
			}

			buf = (buf+1)&1;
		}
	}
	return ptr;
}
#endif


struct file_descriptors_t {
	FILE *fp;
	gzFile gzfp;
	BZFILE *bzfp;
	int bse;
	file_descriptors_t() : fp(NULL), gzfp(NULL), bzfp(NULL), bse(BZ_OK+1) {}
};


loadsave_t::mode_t loadsave_t::save_mode = bzip2;	// default to use for saving
loadsave_t::mode_t loadsave_t::autosave_mode = zipped;	// default to use for autosaving

loadsave_t::loadsave_t() : filename()
{
	mode = 0;
	saving = false;
	buffered = false;
	fd = new file_descriptors_t();
}

loadsave_t::~loadsave_t()
{
	set_buffered(false);
	close();
	delete fd;
}


void loadsave_t::set_buffered(bool enable)
{
	if(  enable  ) {
		if(  !buffered  ) {
			buffered = true;
			curr_buff = 0;
			buf_pos[0] = buf_pos[1] = 0;
			buf_len[0] = buf_len[1] = 0;
			ls_buf[0] = new char[LS_BUF_SIZE];
#ifdef MULTI_THREAD
			ls_buf[1] = new char[LS_BUF_SIZE]; // second buffer only when multithreaded

			simthread_barrier_init(&loadsave_barrier, NULL, 2);
			pthread_mutex_init(&loadsave_mutex, NULL);

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

			ls.loadsave_routine = this;

			pthread_create(&ls_thread, &attr, loadsave_thread, (void *)&ls);

			pthread_attr_destroy(&attr);
#endif
		}
	}
	else {
		if(  buffered  ) {
			if(  saving  &&  buf_pos[curr_buff]>0  ) {
#ifdef MULTI_THREAD
				// first sync with thread causes buffer to be flushed
				simthread_barrier_wait(&loadsave_barrier);
				// second sync with empty buffer signals thread to exit
				simthread_barrier_wait(&loadsave_barrier);
#else
				flush_buffer(curr_buff);
#endif
			}
#ifdef MULTI_THREAD
			pthread_join(ls_thread,NULL);

			pthread_mutex_destroy(&loadsave_mutex);
			simthread_barrier_destroy(&loadsave_barrier);

			delete [] ls_buf[1]; // second buffer only when multithreaded
#endif
			delete [] ls_buf[0];
			buffered = false;
		}
	}
}


bool loadsave_t::rd_open(const char *filename)
{
	close();

	version = 0;
	mode = zipped;
	fd->fp = fopen(filename, "rb");
	if(  fd->fp==NULL  ) {
		// most likely not existing
		return false;
	}
	// now check for BZ2 format
	char buf[80];
	if(  fread( buf, 1, 80, fd->fp )==80  ) {
		if(  buf[0]=='B'  &&  buf[1]=='Z'  ) {
			mode = bzip2;
		}
		fseek(fd->fp,0,SEEK_SET);
	}

	if(  mode==bzip2  ) {
		fd->bse = BZ_OK+1;
		fd->bzfp = NULL;
		fd->bzfp = BZ2_bzReadOpen( &fd->bse, fd->fp, 0, 0, NULL, 0 );
		bool ok = false;
		if(  fd->bse==BZ_OK  ) {
			// else: use zlib
			MEMZERO(buf);
			if(  BZ2_bzRead( &fd->bse, fd->bzfp, buf, sizeof(SAVEGAME_PREFIX) )==sizeof(SAVEGAME_PREFIX)  &&  fd->bse==BZ_OK  ) {
				// get the rest of the string
				for(  int i=sizeof(SAVEGAME_PREFIX);  buf[i-1]>=32  &&  i<79;  i++  ) {
					buf[i] = lsgetc();
				}
				ok = fd->bse==BZ_OK;
			}
		}
		// BZ-Header but wrong data ...
		if(  !ok  ) {
			close();
			return false;
		}
	}

	if(  mode!=bzip2  ) {
		fclose(fd->fp);
		// and now with zlib ...
		fd->gzfp = gzopen(filename, "rb");
		if(fd->gzfp==NULL) {
			return false;
		}
		gzgets(fd->gzfp, buf, 80);
	}
	saving = false;

	if (strstart(buf, SAVEGAME_PREFIX)) {
		version = int_version(buf + sizeof(SAVEGAME_PREFIX) - 1, &mode, pak_extension);
	} else if (strstart(buf, XML_SAVEGAME_PREFIX)) {
		mode |= xml;
		while (lsgetc() != '<') { /* nothing */ }
		read(buf, sizeof(SAVEGAME_PREFIX) - 1);
		if (!strstart(buf, SAVEGAME_PREFIX)) {
			close();
			// not a simutrans XML file ...
			return false;
		}

		read(buf, sizeof("version=\"") - 1);
		char str[256];
		char *s = str;
		for (int i = 0; i < 255; i++) {
			char c = lsgetc();
			if (c=='\"') {
				break;
			}
			*s++ = c;
		}
		*s = 0;
		int dummy;
		version = int_version(str, &dummy, pak_extension);

		read(buf, sizeof(" pak=\"") - 1);
		if (version > 0) {
			s = pak_extension;
			for (int i = 0; i < 63; i++) {
				char c = lsgetc();
				if (c=='\"') {
					break;
				}
				*s++ = c;
			}
			*s = 0;
			while (lsgetc() != '>');
		}
	} else {
		close();
		return false;
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


bool loadsave_t::wr_open(const char *filename, mode_t m, const char *pak_extension, const char *savegame_version)
{
	mode = m;
	close();

	if(  is_zipped()  ) {
		// using zlib
		fd->gzfp = gzopen(filename, "wb");
	}
	else if(  mode==binary  ) {
		// no compression
		fd->fp = fopen(filename, "wb");
	}
	else if(  is_bzip2()  ) {
		// XML or bzip ...
		fd->fp = fopen(filename, "wb");
		// the additional magic for bzip2
		fd->bse = BZ_OK+1;
		fd->bzfp = NULL;
		if(  fd->fp  ) {
			fd->bzfp = BZ2_bzWriteOpen( &fd->bse, fd->fp, 9, 0, 30 /* default is 30 */ );
			if(  fd->bse!=BZ_OK  ) {
				return false;
			}
		}
	}
	else {
		// uncompressed xml should be here ...
		assert(  mode==xml  );
		fd->fp = fopen(filename, "wb");
	}

	// check whether we could open the file
	if(  is_zipped()  ?  fd->gzfp == NULL  :  fd->fp == NULL  ) {
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
	// delete trailing path separator
	this->pak_extension[strlen(this->pak_extension)-1] = 0;

	version = int_version( savegame_version, NULL, NULL );

	if(  !is_xml()  ) {
		char str[4096];
		size_t len;
		if(  version<=102002  ) {
			len = sprintf( str, SAVEGAME_PREFIX "%s%s%s\n", savegame_version, "zip", this->pak_extension );
		}
		else {
			len = sprintf( str, SAVEGAME_PREFIX "%s-%s\n", savegame_version, this->pak_extension );
		}
		write( str, len );
	}
	else {
		char str[4096];
		int n = sprintf( str, "<?xml version=\"1.0\"?>\n<Simutrans version=\"%s\" pak=\"%s\">\n", savegame_version, this->pak_extension );
		write( str, n );
		ident = 1;
	}

	this->mode = mode;
	this->filename = filename;

	return true;
}


const char *loadsave_t::close()
{
	const char *success = NULL;

	if(  is_xml()  &&  saving  &&  (!is_bzip2()  ||  fd->bse==BZ_OK)
	     &&  (is_zipped()  ?  fd->gzfp != NULL :  fd->fp != NULL) ) {
		// only write when close and no error occurred
		const char *end = "\n</Simutrans>\n";
		write( end, strlen(end) );
	}
	if(  is_zipped()  &&  fd->gzfp) {
		int err_no;
		const char *err_str = gzerror( fd->gzfp, &err_no );
		if(err_no!=Z_OK  &&  err_no!=Z_STREAM_END) {
			success =  err_no==Z_ERRNO ? strerror(errno) : err_str;
		}
		gzclose(fd->gzfp);
		fd->gzfp = NULL;
	}
	if(  is_bzip2()  &&  fd->fp ) {
		if(   saving  ) {
			/* BZLIB seems to eat the last byte, if it is at odd position
				* => we just write a dummy zero padding byte
				*/
			write( "", 1 );
			BZ2_bzWriteClose( &fd->bse, fd->bzfp, 0, NULL, NULL );
		}
		else {
			BZ2_bzReadClose( &fd->bse, fd->bzfp );
		}
		fclose( fd->fp );
		fd->bzfp = fd->fp = NULL;
		fd->bse = BZ_STREAM_END;
	}
	if(  !is_bzip2()  &&  !is_zipped()  &&  fd->fp  ) {
		int err_no = ferror(fd->fp);
		fclose(fd->fp);
		if(err_no!=0) {
			success = strerror(err_no);
		}
	}
	fd->fp = NULL;

	return success;
}


/************* from here on the actual data in/out routines ****************/

/**
 * Checks end-of-file
 * @author Hj. Malthaner
 */
bool loadsave_t::is_eof()
{
	if(  is_bzip2()  ) {
		if(  buffered  ) {
			bool r;
#ifdef MULTI_THREAD
			pthread_mutex_lock(&loadsave_mutex);
#endif
			r = buf_pos[0]>=buf_len[0]  &&  buf_pos[1]>=buf_len[1]  &&  fd->bse!=BZ_OK;
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&loadsave_mutex);
#endif
			return r;
		}
		else {
			// any error is EOF ...
			return fd->bse!=BZ_OK;
		}
	}
	else {
		if(  buffered  ) {
			bool r;
#ifdef MULTI_THREAD
			pthread_mutex_lock(&loadsave_mutex);
#endif
			r = buf_pos[0]>=buf_len[0]  &&  buf_pos[1]>=buf_len[1]  &&  gzeof(fd->gzfp)!=0;
#ifdef MULTI_THREAD
			pthread_mutex_unlock(&loadsave_mutex);
#endif
			return r;
		}
		else {
			return gzeof(fd->gzfp)!=0;
		}
	}
}


void loadsave_t::lsputc(int c)
{
	uint8 ch = c;
	write( &ch, 1 );
}


int loadsave_t::lsgetc()
{
	uint8 c[2];
	if(  read(c,1)  ) {
		return c[0];
	}
	return -1;
}


size_t loadsave_t::write(const void *buf, size_t len)
{
	if(  buffered  ) {
		if(  buf_pos[curr_buff]+len<=LS_BUF_SIZE  ) {
			// room in the buffer, copy it all
			for(  unsigned i=0;  i<len;  i++  ) {
				ls_buf[curr_buff][buf_pos[curr_buff]++] = ((const char*)buf)[i];
			}
			return len;
		}
		else {
			// copy up to full buffer
			unsigned i = 0;
			const unsigned left = LS_BUF_SIZE-buf_pos[curr_buff];
			while(  i<left  ) {
				ls_buf[curr_buff][buf_pos[curr_buff]++] = ((const char*)buf)[i++];
			}

#ifdef MULTI_THREAD
			// sync with thread to flush the buffer
			simthread_barrier_wait(&loadsave_barrier);

			// switch buffers
			curr_buff = (curr_buff+1)&1;
#else
			// not threaded, flush single buffer ourselves
			flush_buffer(curr_buff);
#endif
			// copy the rest
			while(  i<len  ) {
				ls_buf[curr_buff][buf_pos[curr_buff]++] = ((const char*)buf)[i++];
			}
			return len;
		}
	}
	else {
		if(  is_zipped()  ) {
			return gzwrite(fd->gzfp, const_cast<void *>(buf), len);
		}
		else if(  is_bzip2()  ) {
			BZ2_bzWrite( &fd->bse, fd->bzfp, const_cast<void *>(buf), len);
			assert(fd->bse==BZ_OK);
			return len;
		}
		else {
			return fwrite(buf, 1, len, fd->fp);
		}
	}
}


void loadsave_t::flush_buffer(int buf_num)
{
	int bse = fd->bse;
	if(  is_zipped()  ) {
		gzwrite(fd->gzfp, ls_buf[buf_num], buf_pos[buf_num]);
	}
	else if(  is_bzip2()  ) {
		BZ2_bzWrite( &bse, fd->bzfp, ls_buf[buf_num], buf_pos[buf_num]);
		assert(bse==BZ_OK);
	}
	else {
		fwrite(ls_buf[buf_num], 1, buf_pos[buf_num], fd->fp);
	}
#ifdef MULTI_THREAD
	pthread_mutex_lock(&loadsave_mutex);
#endif
	if(  is_bzip2()  ) {
		fd->bse = bse;
	}
	buf_pos[buf_num] = 0;
#ifdef MULTI_THREAD
	pthread_mutex_unlock(&loadsave_mutex);
#endif
}


size_t loadsave_t::read(void *buf, size_t len)
{
	if(  buffered  ) {
		if(  len>=LS_BUF_SIZE*2  ) {
			dbg->fatal("loadsave_t::read()","Request for %d too long", len);
			return 0;
		}
		if(  buf_pos[curr_buff]+len<=buf_len[curr_buff]  ) {
			// room in the buffer, copy it all
			for(  unsigned i=0;  i<len;  i++  ) {
				((char*)buf)[i] = ls_buf[curr_buff][buf_pos[curr_buff]++];
			}
 			return len;
		}
		else {
			// copy up to full buffer
			unsigned i = 0;
			if(  buf_len[curr_buff]>0  ) {
				const unsigned left = buf_len[curr_buff]-buf_pos[curr_buff];
				while(  i<left  ) {
					((char*)buf)[i++] = ls_buf[curr_buff][buf_pos[curr_buff]++];
				}
			}
#ifdef MULTI_THREAD
			// sync with other thread to read more
			simthread_barrier_wait(&loadsave_barrier);

			// switch buffers
			curr_buff = (curr_buff+1)&1;
#else
			// not threaded, read more into single buffer ourselves
			fill_buffer(curr_buff);
#endif
			// check if enough read
			if(  len-i>buf_len[curr_buff]  ) {
				dbg->fatal("loadsave_t::read","savegame corrupt, not enough data");
				return 0;
			}

			// copy the rest
			while(  i<len  ) {
				((char*)buf)[i++] = ls_buf[curr_buff][buf_pos[curr_buff]++];
			}
			return len;
		}
	}
	else {
		if(  is_bzip2()  ) {
			if(  fd->bse==BZ_OK  ) {
				BZ2_bzRead( &fd->bse, fd->bzfp, buf, len);
			}
			return fd->bse==BZ_OK ? len : 0;
		}
		else {
			return gzread(fd->gzfp, buf, len);
		}
	}
}


int loadsave_t::fill_buffer(int buf_num)
{
	int r;
	int bse = fd->bse;

	if(  is_bzip2()  ) {
		if(  bse==BZ_OK  ) {
			r = BZ2_bzRead( &bse, fd->bzfp, ls_buf[buf_num], LS_BUF_SIZE);
			if (  bse != BZ_OK  &&  bse != BZ_STREAM_END  ) {
				r = -1; // an error occurred
			}
		}
		else {
			assert(bse == BZ_STREAM_END);
			r = 0;
		}
	}
	else {
		r = gzread(fd->gzfp, ls_buf[buf_num], LS_BUF_SIZE);
	}
#ifdef MULTI_THREAD
	pthread_mutex_lock(&loadsave_mutex);
#endif
	if(  is_bzip2()  ) {
		fd->bse = bse;
	}
	buf_pos[buf_num] = 0;
	buf_len[buf_num] = r>=0 ? r : 0; // buf_len is unsigned, set to zero in case of error
#ifdef MULTI_THREAD
	pthread_mutex_unlock(&loadsave_mutex);
#endif
	return r;
}


/*************** High level routines to read/write data types *************
 * (check also for Intel/Motorola) etc
 */


void loadsave_t::rdwr_byte(sint8 &c)
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
		c = (sint8)ll;
	}
}


void loadsave_t::rdwr_byte(uint8 &c)
{
	sint8 cc=c;
	rdwr_byte(cc);
	c = (uint8)cc;
}


void loadsave_t::rdwr_short(sint16 &i)
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
		i = (sint16)ll;
	}
}


void loadsave_t::rdwr_short(uint16 &i)
{
	sint16 ii=i;
	rdwr_short(ii);
	i = (uint16)ii;
}


void loadsave_t::rdwr_long(sint32 &l)
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
		l = (sint32)ll;
	}
}


void loadsave_t::rdwr_long(uint32 &l)
{
	sint32 ll=l;
	rdwr_long(ll);
	l = (uint32)ll;
}


void loadsave_t::rdwr_longlong(sint64 &ll)
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


void loadsave_t::rdwr_bool(bool &i)
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
void loadsave_t::rdwr_str( char* result_buffer, size_t const size)
{
	if(!is_xml()) {
		uint16 len;
		if(saving) {
			len = (uint16)min(32767,strlen(result_buffer));
#ifdef SIM_BIG_ENDIAN
			{
				sint16 ii = endian(len);
				write(&ii, sizeof(sint16));
			}
#else
			write(&len, sizeof(uint16));
#endif
			write(result_buffer, len);
		}
		else {
			read(&len, sizeof(uint16));
			len = endian(len);
			if(  len >= size) {
				dbg->fatal( "loadsave_t::rdwr_str()","string longer (%i) than allowed size (%i)", len, size );
			}
			read(result_buffer, len);
			result_buffer[len] = '\0';
		}
	}
	else {
		// use CDATA tag: <![CDATA[%s]]>
		char *s = result_buffer;
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
			if (!strstart(buffer, "string>")) {
				if (!strstart(buffer, "![CDATA") || lsgetc() != '[') {
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
						if (i >= 8 && strstart(s - 8, "</string")) {
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
				char temp[32767];
				char *s = temp;
				for(  size_t i=0;  i<size+3;  i++  ) {
					*s++ = lsgetc();
					if(  i>=2  &&  strstart(s-3,"]]>")  ) {
						s[-3] = 0;
						strcpy( result_buffer, temp );
						return;
					}
				}
				*s = 0;
				dbg->fatal( "loadsave_t::rdwr_str()","string too long (exceeded %i characters)", size );
			}
		}
	}
}


void loadsave_t::rdwr_str(plainstring& s)
{
	if(  is_loading()  ) {
		const char* buf = NULL;
		rdwr_str(buf);
		if(  buf  ) {
			s = buf;
			free( const_cast<char*>(buf) );
		}
		else {
			s = "";
		}
	}
	else {
		char const* tmp = s.c_str();
		rdwr_str(tmp);
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
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
			read(buf, strlen(tag));
			if (!strstart(buf, tag)) {
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
	if(!saving) {
		dbg->fatal( "loadsave_t::wr_obj_id()", "must be only called during saving!" );
	}
	if(!is_xml()) {
		lsputc( id );
	}
	else {
		sint64 ll=id;
		rdwr_xml_number( ll, "id" );
	}
}


sint16 loadsave_t::rd_obj_id()
{
	if(saving) {
		dbg->fatal( "loadsave_t::rd_obj_id()", "must be only called during reading!" );
		return INVALID_RDWR_ID;
	}
	if(!is_xml()) {
		sint8 idc;
		read(&idc, sizeof(sint8));
		return (sint8)idc;
	}
	else {
		sint64 ll;
		rdwr_xml_number( ll, "id" );
		return (sint16)ll;
	}
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
			if (!strstart(buf, "<id=\"")) {
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
			if (!strstart(buf, "/>")) {
				dbg->fatal( "loadsave_t::rd_obj_id()","id tag not properly closed!" );
			}
		}
	}
}


uint32 loadsave_t::int_version(const char *version_text, int * /*mode*/, char *pak_extension_str)
{
	// major number (0..)
	uint32 v0 = atoi(version_text);
	while(*version_text  &&  *version_text++ != '.')
		;
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		return 0;
	}

	// middle number (.99.)
	uint32 v1 = atoi(version_text);
	while(*version_text  &&  *version_text++ != '.')
		;
	if(!*version_text) {
		dbg->fatal( "loadsave_t::int_version()","Really broken version string!" );
		return 0;
	}

	// minor number (..08)
	uint32 v2 = atoi(version_text);
	uint32 version = v0 * 1000000 + v1 * 1000 + v2;

	while(*version_text  &&  isdigit(*version_text)) {
		version_text++;
	}
	// simutrans-experimental savegame?
	// the next char is either 'b'/'z'/'-',
	// if it is '.' the we try to load a simutrans-experimental savegame
	if(*version_text == '.') {
		// st-exp savegame, we can't load it, return version=0
		strcpy(pak_extension_str,"(st-exp)");
		return 0;
	}

	if(  version<=102002  ) {
		/* the compression and the mode we determined already ourselves (otherwise we cannot read this
		 * => leave the mode alone but for unknown modes!
		 */
		if (strstart(version_text, "bin")) {
			//*mode = binary;
			version_text += 3;
		} else if (strstart(version_text, "zip")) {
			//*mode = zipped;
			version_text += 3;
		}
		else if(  *version_text  ) {
			// illegal version ...
			strcpy(pak_extension_str,"(broken)");
			return 0;
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

	return version;
}
