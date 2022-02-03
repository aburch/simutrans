/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>

#include "../sys/simsys.h"
#include "../simtypes.h"
#include "../macros.h"
#include "../simversion.h"
#include "../simmem.h"
#include "../simdebug.h"
#include "../utils/plainstring.h"
#include "../utils/simstring.h"

#include "loadsave.h"

#include "../io/rdwr/bzip2_file_rdwr_stream.h"
#include "../io/rdwr/raw_file_rdwr_stream.h"
#include "../io/rdwr/zlib_file_rdwr_stream.h"
#if USE_ZSTD
#include "../io/rdwr/zstd_file_rdwr_stream.h"
#endif
#include "../io/rdwr/compare_file_rd_stream.h"

#define INVALID_RDWR_ID (-1)

//#undef MULTI_THREAD

// buffer size for read/write - bzip2 gains up to 8M for non-threaded, 1M for threaded. binary, zipped ok with 256K or smaller.
#define LS_BUF_SIZE (1 << 20) // 1 MiB

#define LS_MAX_STRING_LEN  (0x7FFF)
#define LS_STRING_BUF_SIZE (0x8000) // includes the trailing 0

#ifdef MULTI_THREAD
#include "../utils/simthread.h"

static pthread_t ls_thread;
static simthread_barrier_t loadsave_barrier;
static pthread_mutex_t loadsave_mutex;

static pthread_mutex_t readdata_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  readdata_cond  = PTHREAD_COND_INITIALIZER;
static int readdata_flag = 0;  // > 0 read more, < 0 no data needed/error while reading

// parameters passed starting a thread
typedef struct{
	loadsave_t *loadsave_routine;
} loadsave_param_t;
static loadsave_param_t ls;

/*
 * Multi-threaded loading:
 * more complicated synchronization due to different sources of errors
 * - less data available than needed (noticed within load_thread)
 * - more data available than needed (main thread finishes reading but load_thread still waits)
 *
 * Communication of error by variable readdata_flag, protected by readdata_mutex
 *
 * Intended program flow:
 *
 * main                                                      load_thread
 * (data processing)                        (finalize)       fill_buffer
 * < --  thread_barrier_wait ------------------------------------------------>
 * (error handling)                                          pthread_cond_wait
 * if readdata_flag < 0
 *       load_thread already exited
 *       not enough data -> fatal error
 *
 * readdata_flag = 1                        readdata_flag = -1
 * pthread_cond_broadcast --------------------------------------------------->
 * repeat                                                            (error handling)
 *                                                           if readdata_flag < 0
 *                                                         <------ end thread
 *                                                           if error occurred during previous fill_buffer
 *                                                                 readdata_flag = -1
 *                                          (join threads) <------ end thread
 *                                                           repeat
 */
void *load_thread( void *ptr )
{
	loadsave_param_t *lsp = reinterpret_cast<loadsave_param_t *>(ptr);
	int buf = 1;

	while(true) {
		const size_t bytes_read = lsp->loadsave_routine->fill_buffer(buf);

		// always wait to sync with main thread before filling the next buffer
		pthread_mutex_lock(&readdata_mutex);
		simthread_barrier_wait(&loadsave_barrier);

		while(  readdata_flag == 0  ) {
			pthread_cond_wait(&readdata_cond, &readdata_mutex);
		}
		if (readdata_flag < 0) {
			// leave if  no more data needed
			pthread_mutex_unlock(&readdata_mutex);
			break;
		}
		if (bytes_read == 0) {
			// nothing read into buffer, or error occurred
			// flag error to main thread
			readdata_flag = -1;
			pthread_mutex_unlock(&readdata_mutex);
			break;
		}
		readdata_flag = 0;
		pthread_mutex_unlock(&readdata_mutex);

		// switch buffer
		buf = (buf+1)&1;
	}
	return ptr;
}

void loading_trigger_fill_buffer()
{
	// sync with other thread, tell to read more data
	simthread_barrier_wait(&loadsave_barrier);

	pthread_mutex_lock(&readdata_mutex);
	if (readdata_flag < 0) {
		pthread_mutex_unlock(&readdata_mutex);
		// reading thread exited due to error
		dbg->fatal("loadsave_t::read", "savegame corrupt, not enough data");
	}
	readdata_flag = 1; // more data please

	pthread_cond_broadcast(&readdata_cond);
	pthread_mutex_unlock(&readdata_mutex);
}

void loading_finalize()
{
	simthread_barrier_wait(&loadsave_barrier);
	// reader thread waits, signal end of loadingdata
	pthread_mutex_lock(&readdata_mutex);
	readdata_flag = -1; // no more data

	pthread_cond_broadcast(&readdata_cond);
	pthread_mutex_unlock(&readdata_mutex);
}


/*
 * Multi-threaded saving:
 *
 * - synchronization is done with barriers
 * - end-of-saving is signaled to thread with get_buf_pos(buf)==0,
 *   which is protected by loadsave_mutex
 */
void *save_thread( void *ptr )
{
	loadsave_param_t *lsp = reinterpret_cast<loadsave_param_t *>(ptr);
	int buf = 1;

	while(true) {
		// wait to sync with main thread before flushing the buffer
		simthread_barrier_wait(&loadsave_barrier);

		buf = (buf+1)&1;
		if(  lsp->loadsave_routine->get_buf_pos(buf)==0  ) {
			// empty buffer after sync - signal to exit
			break;
		}
		lsp->loadsave_routine->flush_buffer(buf);
	}
	return ptr;
}

void saving_trigger_flush()
{
	// sync with thread to flush the buffer
	simthread_barrier_wait(&loadsave_barrier);
}

void saving_finalize()
{
	// first sync with thread causes buffer to be flushed
	simthread_barrier_wait(&loadsave_barrier);
	// second sync with empty buffer signals thread to exit
	simthread_barrier_wait(&loadsave_barrier);
}

#endif


loadsave_t::mode_t loadsave_t::save_mode = bzip2;      // default to use for saving
loadsave_t::mode_t loadsave_t::autosave_mode = zipped; // default to use for autosaving
int loadsave_t::save_level = 6;
int loadsave_t::autosave_level = 1;


void NORETURN loadsave_t::fatal(const char* who, const char* format, ...)
{
	va_list argptr;
	va_start(argptr, format);

	static char formatbuffer[512];
	const char* fn = filename.c_str();
	sprintf(formatbuffer,
		"FATAL ERROR during reading of \"%s\"\n"
		"The file has been renamed to \"%s-error\"\n\n"
		"%s: %s\n"
		"\n"
		"Aborting program execution ...\n"
		"Please try to restart Simutrans again!\n",
		fn, fn,
		who, format);

	static char buffer[8192];
	vsprintf(buffer, formatbuffer, argptr);
	va_end(argptr);

	close();
	std::string fn_new = filename + "-error";
	dr_remove(fn_new.c_str());
	dr_rename(fn, fn_new.c_str());
	dbg->custom_fatal(buffer);
}


loadsave_t::loadsave_t() :
	mode(binary),
	buffered(false),
	stream(NULL)
{
	curr_buff = 0;
}


loadsave_t::~loadsave_t()
{
	set_buffered(false);

	const bool saving = is_saving();
	const char *errmsg = close();

	if(  errmsg  ) {
		dbg->error( "loadsave_t::~loadsave_t", "Could not %s save file: %s", (saving ? "save" : "load"), errmsg );
	}
}


void loadsave_t::set_buffered(bool enable)
{
	if(  enable  ) {
		if(  !buffered  ) {
			buffered = true;
			curr_buff = 0;
			buff[0].pos = buff[1].pos = 0;
			buff[0].len = buff[1].len = 0;
			buff[0].buf = new char[LS_BUF_SIZE];

#ifdef MULTI_THREAD
			buff[1].buf = new char[LS_BUF_SIZE]; // second buffer only when multithreaded

			simthread_barrier_init(&loadsave_barrier, NULL, 2);
			pthread_mutex_init(&loadsave_mutex, NULL);
			pthread_mutex_init(&readdata_mutex, NULL);
			readdata_flag = 0;

			pthread_attr_t attr;
			pthread_attr_init(&attr);
			pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

			ls.loadsave_routine = this;

			pthread_create(&ls_thread, &attr, is_saving() ? save_thread : load_thread, (void *)&ls);

			pthread_attr_destroy(&attr);
#endif
		}
	}
	else {
		if(  buffered  ) {
			if(  is_saving()  &&  buff[curr_buff].pos>0  ) {
#ifdef MULTI_THREAD
				saving_finalize();
#else
				flush_buffer(curr_buff);
#endif
			}
#ifdef MULTI_THREAD
			if(  !is_saving()  ) {
				loading_finalize();
			}
			pthread_join(ls_thread,NULL);

			pthread_mutex_destroy(&loadsave_mutex);
			pthread_mutex_destroy(&readdata_mutex);
			simthread_barrier_destroy(&loadsave_barrier);

			delete[] buff[1].buf; // second buffer only when multithreaded
#endif
			delete[] buff[0].buf;
			buffered = false;
		}
	}
}


loadsave_t::file_status_t loadsave_t::rd_open(const char *filename_utf8)
{
	close();

	const file_classify_status_t cl_status = classify_save_file(filename_utf8, &finfo);

	if (cl_status != FILE_CLASSIFY_OK) {
		// file likely does not exist
		dbg->warning("loadsave_t::rd_open", "File '%s' does not exist or is not accessible", filename_utf8);
		return FILE_STATUS_ERR_NOT_EXISTING;
	}
	else if(  finfo.version == INVALID_FILE_VERSION  ) {
		return FILE_STATUS_ERR_NO_VERSION;
	}
	else if(  finfo.version > (SIM_VERSION_MAJOR*1000 + SIM_SERVER_MINOR)  ) {
		/*
		 * Reading future versions will almost certainly lead to exceptions; so we close here.
		 * It would be nice to give a detailed message what failed (like the fatal error does)
		 * But this error may happening also in regular installations after running a nighly
		 * so we just record the failure.
		 */
		return FILE_STATUS_ERR_FUTURE_VERSION;
	}

	// now open the file
	assert(stream == NULL);
	mode = 0;

	switch (finfo.file_type) {
	case file_info_t::TYPE_XML_ZSTD:
		mode = xml;
		// fallthrough
	case file_info_t::TYPE_ZSTD:
		mode |= zstd;
#if USE_ZSTD
		stream = new zstd_file_rdwr_stream_t(filename_utf8, false, 0); break;
#else
		dbg->warning("loadsave_t::rd_open", "Cannot read from '%s': Unsupported save file compression 'zstd'", filename_utf8);
		return FILE_STATUS_ERR_UNSUPPORTED_COMPRESSION;
#endif

	case file_info_t::TYPE_XML_BZIP2:
		mode = xml;
		// fallthrough
	case file_info_t::TYPE_BZIP2:
		mode |= bzip2;
		stream = new bzip2_file_rdwr_stream_t(filename_utf8, false); break;

	case file_info_t::TYPE_XML_ZIPPED:
		mode = xml;
		// fallthrough
	case file_info_t::TYPE_ZIPPED:
		mode |= zipped;
		stream = new zlib_file_rdwr_stream_t(filename_utf8, false, 0); break;

	case file_info_t::TYPE_XML:
		mode = xml;
		// fallthrough
	default:
		stream = new raw_file_rdwr_stream_t(filename_utf8, false); break;
	}

	if (!stream || stream->get_status() != rdwr_stream_t::STATUS_OK) {
		const char *errmsg = close();

		// file likely does not exist any longer
		if (errmsg) {
			dbg->warning("loadsave_t::rd_open", "Cannot read from '%s': %s", filename_utf8, errmsg);
		}
		else {
			dbg->warning("loadsave_t::rd_open", "Cannot read from '%s'", filename_utf8);
		}

		return FILE_STATUS_ERR_NOT_EXISTING;
	}

	// skip header
	size_t header_size = finfo.header_size;

	while (header_size != 0) {
		char buf[128];
		const size_t sz = min(header_size, 128);
		stream->read(buf, sz);
		header_size -= sz;
	}

	filename = filename_utf8;

	return FILE_STATUS_OK;
}


loadsave_t::file_status_t loadsave_t::wr_open( const char *filename_utf8, mode_t m, int level, const char *pak_extension, const char *savegame_version )
{
	mode = m;
	close();

#if !USE_ZSTD
	if( mode & zstd ) {
		mode &= ~zstd;
		mode |= bzip2;
		dbg->warning( "loadsave_t::wr_open", "Compiled without zstd support, using bzip2!" );
	}
#endif

	assert(stream == NULL);

	switch (mode & ~xml) {
#if USE_ZSTD
	case zstd: stream = new zstd_file_rdwr_stream_t(filename_utf8, true, level); break;
#endif
	case bzip2:  stream = new bzip2_file_rdwr_stream_t(filename_utf8, true);       break;
	case zipped: stream = new zlib_file_rdwr_stream_t(filename_utf8, true, level); break;
	case binary: stream = new raw_file_rdwr_stream_t(filename_utf8, true);         break;
	default:
		dbg->error("loadsave_t::wr_open", "Unsupported save file compression");
		return FILE_STATUS_ERR_UNSUPPORTED_COMPRESSION;
	}

	if (stream->get_status() != rdwr_stream_t::STATUS_OK) {
		dbg->error("loadsave_t::wr_open", "Cannot open '%s' for writing!", filename_utf8);
		return (stream->get_status() == rdwr_stream_t::STATUS_ERR_NOT_EXISTING) ? FILE_STATUS_ERR_NOT_EXISTING : FILE_STATUS_ERR_CORRUPT;
	}

	set_buffered( true );

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
	tstrncpy(finfo.pak_extension, start, lengthof(finfo.pak_extension));
	// delete trailing path separator
	finfo.pak_extension[strlen(finfo.pak_extension)-1] = 0;

	finfo.version = int_version( savegame_version, NULL );

	if(  !is_xml()  ) {
		char str[4096];
		size_t len;
		if(  is_version_less(102, 3)  ) {
			len = sprintf( str, SAVEGAME_PREFIX "%s%s%s\n", savegame_version, "zip", finfo.pak_extension );
		}
		else {
			len = sprintf( str, SAVEGAME_PREFIX "%s-%s\n", savegame_version, finfo.pak_extension );
		}
		write( str, len );
	}
	else {
		char str[4096];
		int n = sprintf( str, "<?xml version=\"1.0\"?>\n<Simutrans version=\"%s\" pak=\"%s\">\n", savegame_version, finfo.pak_extension );
		write( str, n );
		indent = 1;
	}

	return FILE_STATUS_OK;
}


const char *loadsave_t::close()
{
	if (!stream) {
		return NULL;
	}

	if(  is_xml()  && stream->is_writing() && stream->get_status() == rdwr_stream_t::STATUS_OK) {
		// only write when close and no error occurred
		const char *end = "\n</Simutrans>\n";
		write( end, strlen(end) );
	}

	if (buffered) {
		// flush buffers
		set_buffered(false);
	}

	const char *errmsg = NULL;

	switch (stream->get_status()) {
	case rdwr_stream_t::STATUS_EOF:
	case rdwr_stream_t::STATUS_OK: errmsg = NULL; break;

	case rdwr_stream_t::STATUS_ERR_CORRUPT:        errmsg = "Corrupt save file";         break;
	case rdwr_stream_t::STATUS_ERR_DEPRECATED:     errmsg = "Save file version too old"; break;
	case rdwr_stream_t::STATUS_ERR_FUTURE_VERSION: errmsg = "Save file version too new"; break;
	case rdwr_stream_t::STATUS_ERR_NO_VERSION:     errmsg = "Unversioned save file";     break;
	case rdwr_stream_t::STATUS_ERR_FULL:           errmsg = "No space left on device";   break;
	case rdwr_stream_t::STATUS_ERR_NOT_EXISTING:   errmsg = "File not found";            break;
	case rdwr_stream_t::STATUS_INVALID:            errmsg = "<Invalid status>";          break;
	}

	delete stream;
	stream = NULL;

	return errmsg;
}


/************* from here on the actual data in/out routines ****************/

/**
 * Checks end-of-file
 */
bool loadsave_t::is_eof()
{
#ifdef MULTI_THREAD
	if (buffered) {
		pthread_mutex_lock( &loadsave_mutex );
	}
#endif

	const bool eof = (!buffered || (buff[0].pos >= buff[0].len && buff[1].pos >= buff[1].len)) &&
		stream->get_status() == rdwr_stream_t::STATUS_EOF;

#ifdef MULTI_THREAD
	if (buffered) {
		pthread_mutex_unlock(&loadsave_mutex);
	}
#endif

	return eof;
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
	if (!buffered) {
		return stream->write(buf, len);
	}

	if(  buff[curr_buff].pos+len<=LS_BUF_SIZE  ) {
		// room in the buffer, copy it all
		for(  unsigned i=0;  i<len;  i++  ) {
			buff[curr_buff].buf[buff[curr_buff].pos++] = ((const char*)buf)[i];
		}
		return len;
	}
	else {
		// copy up to full buffer
		unsigned i = 0;
		const unsigned left = LS_BUF_SIZE-buff[curr_buff].pos;
		while(  i<left  ) {
			buff[curr_buff].buf[buff[curr_buff].pos++] = ((const char*)buf)[i++];
		}

#ifdef MULTI_THREAD
		saving_trigger_flush();

		// switch buffers
		curr_buff = (curr_buff+1)&1;
#else
		// not threaded, flush single buffer ourselves
		flush_buffer(curr_buff);
#endif
		// copy the rest
		while(  i<len  ) {
			buff[curr_buff].buf[buff[curr_buff].pos++] = ((const char*)buf)[i++];
		}
		return len;
	}
}


void loadsave_t::flush_buffer(int buf_num)
{
#ifdef MULTI_THREAD
	pthread_mutex_lock(&loadsave_mutex);
#endif

	// Cannot abort the saving process, so just ignore any further flushes
	// if the previous flush has failed.
	// loadsave_t::close() handles propagation of the error message.
	if (stream->get_status() == rdwr_stream_t::STATUS_OK) {
		stream->write(buff[buf_num].buf, buff[buf_num].pos);
	}
	buff[buf_num].pos = 0;

#ifdef MULTI_THREAD
	pthread_mutex_unlock(&loadsave_mutex);
#endif
}


void loadsave_t::write_indent()
{
	static const int max_indent = 64;
	static const char spaces[max_indent] = "                                                               ";

	write(spaces, min(indent, max_indent) );
}


size_t loadsave_t::read(void *buf, size_t len)
{
	if (!buffered) {
		return stream->read( buf, len);
	}

	if(  len>=LS_BUF_SIZE*2  ) {
		dbg->fatal("loadsave_t::read()","Request for %d too long", len);
	}
	if(  buff[curr_buff].pos+len<=buff[curr_buff].len  ) {
		// room in the buffer, copy it all
		for(  unsigned i=0;  i<len;  i++  ) {
			((char*)buf)[i] = buff[curr_buff].buf[buff[curr_buff].pos++];
		}

		return len;
	}
	else {
		// copy up to full buffer
		unsigned i = 0;
		if(  buff[curr_buff].len>0  ) {
			const unsigned left = buff[curr_buff].len-buff[curr_buff].pos;
			while(  i<left  ) {
				((char*)buf)[i++] = buff[curr_buff].buf[buff[curr_buff].pos++];
			}
		}
#ifdef MULTI_THREAD
		loading_trigger_fill_buffer();

		// switch buffers
		curr_buff = (curr_buff+1)&1;
#else
		// not threaded, read more into single buffer ourselves
		fill_buffer(curr_buff);
#endif
		// check if enough read
		if(  len-i>buff[curr_buff].len  ) {
			dbg->fatal("loadsave_t::read","savegame corrupt, not enough data");
		}

		// copy the rest
		while(  i<len  ) {
			((char*)buf)[i++] = buff[curr_buff].buf[buff[curr_buff].pos++];
		}

		return len;
	}
}


size_t loadsave_t::fill_buffer( int buf_num )
{
	assert(buffered);

	const size_t sz = stream->read(buff[ buf_num ].buf, LS_BUF_SIZE);

#ifdef MULTI_THREAD
	pthread_mutex_lock(&loadsave_mutex);
#endif

	const rdwr_stream_t::status_t status = stream->get_status();
	const bool stream_ok = (status == rdwr_stream_t::STATUS_EOF || status == rdwr_stream_t::STATUS_OK);

	assert((status == rdwr_stream_t::STATUS_OK) == (sz == LS_BUF_SIZE));
	buff[buf_num].pos = 0;
	buff[buf_num].len = stream_ok ? sz : 0; // buf_len is unsigned, set to zero in case of error

#ifdef MULTI_THREAD
	pthread_mutex_unlock(&loadsave_mutex);
#endif
	return sz;
}


/*************** High level routines to read/write data types *************
 * (check also for Intel/Motorola) etc
 */


void loadsave_t::rdwr_byte(sint8 &c)
{
	if(!is_xml()) {
		if(is_saving()) {
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
		if (is_saving()) {
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
		if (is_saving()) {
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
		if (is_saving()) {
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
		if(is_saving()) {
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
		if(is_saving()) {
			lsputc(i ? '1' : '0');
		}
		else {
			i = lsgetc()=='1';
		}
	}
	else {
		// bool xml
		if(is_saving()) {
			write_indent();
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
				fatal( "loadsave_t::rdwr_bool()","expected \"<bool>\", got \"<%s\"", buffer );
			}
			read( buffer, 4 );
			buffer[4] = 0;
			i = strcmp("true",buffer)==0;
			while(  lsgetc()!='<'  ) { /* nothing */ }
			read( buffer, 6 );
			buffer[6] = 0;
			if(  strcmp("/bool>",buffer)!=0  ) {
				fatal( "loadsave_t::rdwr_bool()","expected \"</bool>\", got \"<%s\"", buffer );
			}
		}
	}
}


void loadsave_t::rdwr_xml_number(sint64 &s, const char *typ)
{
	if(is_saving()) {
		static char nr[256];
		size_t len = sprintf( nr, "%*s<%s>%.0f</%s>\n", indent, "", typ, (double)s, typ );
		write( nr, len );
	}
	else {
		const int len = (int)strlen(typ);
		assert(len<256);

		// find start of tag and handle eof correctly
		while( 1 ) {
			int ch = lsgetc();
			if( ch == '<' ) {
				break;
			}
			if( ch < 0 ) {
				fatal( "loadsave_t::rdwr_xml_number()", "Reached end of file while trying to read <%s>", typ );
			}
		}

		// check for correct tag
		char buffer[256];
		read( buffer, len );
		buffer[len] = 0;
		if(  strcmp(typ,buffer)!=0  ) {
			fatal( "loadsave_t::rdwr_xml_number()","expected \"<%s>\", got \"<%s>\"", typ, buffer );
		}
		while(  lsgetc()!='>'  )  ;
		// read number;
		s = 0;
		bool minus = false;
		while(!is_eof()) {
			const int c = lsgetc();

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
					fatal( "loadsave_t::rdwr_xml_number()", "type %s, found %c in number!", typ, c );
				}
			}
		}
		if(minus) {
			s = -s;
		}

		if(  lsgetc()!='/'  ) {
			fatal( "loadsave_t::rdwr_xml_number()", "missing '/' (not closing tag)" );
		}
		read( buffer, len );
		buffer[6] = 0;
		if(  strcmp(typ,buffer)!=0  ) {
			fatal( "loadsave_t::rdwr_xml_number()","expected \"</%s>\", got \"</%s>\"", typ, buffer );
		}
		while(  lsgetc()!='>'  )  ;
	}
}


// s is a malloc-ed string (will be freed and newly allocated on load time!)
void loadsave_t::rdwr_str(const char *&s)
{
	if(!is_xml()) {
		sint16 size;
		if(is_saving()) {
			size = s ? (sint16)min(LS_MAX_STRING_LEN,strlen(s)) : 0;
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
		if(is_saving()) {
			write_indent();
			write( "<![CDATA[", 9 );
			if(s) {
				write( s, strlen(s) );
			}
			write( "]]>\n", 4 );
		}
		else {
			char buffer[LS_STRING_BUF_SIZE];
			rdwr_str( buffer, LS_STRING_BUF_SIZE );
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
		if(is_saving()) {
			len = (uint16)min(LS_MAX_STRING_LEN,strlen(result_buffer));
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
				fatal( "loadsave_t::rdwr_str()","string longer (%i) than allowed size (%i)", len, size );
			}
			read(result_buffer, len);
			result_buffer[len] = '\0';
		}
	}
	else {
		// use CDATA tag: <![CDATA[%s]]>
		char *s = result_buffer;
		if(is_saving()) {
			write_indent();
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
					fatal( "loadsave_t::rdwr_str()","expected str \"<![CDATA[\", got \"%s\"", buffer );
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
				static char temp[LS_STRING_BUF_SIZE + 3];
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
				fatal( "loadsave_t::rdwr_str()","string too long (exceeded %i characters)", size );
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
		if(is_saving()) {
			write_indent();
			write( "<", 1 );
			write( tag, strlen(tag) );
			write( ">\n", 2 );
			indent ++;
		}
		else {
			char buf[256];
			// find start of tag
			while(  lsgetc()!='<'  ) { /* nothing */ }
			read( buf, strlen(tag) );
			if(  !strstart(buf, tag)  ) {
				fatal( "loadsave_t::start_tag()","expected \"%s\", got \"%s\"", tag, buf );
			}
			while(  lsgetc()!='>'  )  ;
		}
	}
}


void loadsave_t::end_tag(const char *tag)
{
	if(  is_xml()  ) {
		if(is_saving()) {
			indent --;
			write_indent();
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
	if(!is_saving()) {
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
	if(is_saving()) {
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
	if(is_saving()) {
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
	if(!is_saving()) {
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
				fatal( "loadsave_t::rd_obj_id()","expected id str \"<id=\"\", got \"%s\"", buf );
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
				fatal( "loadsave_t::rd_obj_id()","id tag not properly closed!" );
			}
		}
	}
}


uint32 loadsave_t::int_version(const char *version_text, char *pak_extension_str)
{
	// major number (0..)
	uint32 v0 = atoi(version_text);
	while(*version_text  &&  *version_text++ != '.')
		;
	if(!*version_text) {
		return 0;
	}

	// middle number (.99.)
	uint32 v1 = atoi(version_text);
	while(*version_text  &&  *version_text++ != '.')
		;
	if(!*version_text) {
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
		// Simutrans Extended savegame, we can't load it, return version=0
		if (pak_extension_str) {
			std::strcpy(pak_extension_str,"(st-exp)");
		}
		return 0;
	}

	if(  version<=102002  ) {
		/* the compression and the mode we determined already ourselves
		 * (otherwise we cannot read this => leave the mode alone but for unknown modes!)
		 */
		if (strstart(version_text, "bin")) {
			version_text += 3;
		}
		else if (strstart(version_text, "zip")) {
			version_text += 3;
		}
		else if(  *version_text  ) {
			// illegal version ...
			if (pak_extension_str) {
				std::strcpy(pak_extension_str,"(broken)");
			}
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


stream_loadsave_t::stream_loadsave_t(rdwr_stream_t *stream)
{
	this->stream = stream;
	finfo.version = int_version(VERSION_NUMBER, NULL);
}


compare_loadsave_t::compare_loadsave_t(loadsave_t *file1, loadsave_t *file2)
{
	stream = new compare_file_rd_stream_t(file1->stream, file2->stream);
	finfo.version = int_version(VERSION_NUMBER, NULL);
	set_buffered(false);
	file1->set_buffered(false);
	file2->set_buffered(false);
}


compare_loadsave_t::~compare_loadsave_t()
{
	delete stream;
	stream = NULL;
}



