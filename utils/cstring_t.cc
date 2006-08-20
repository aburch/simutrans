

#include <string.h>
#include <stdio.h>
#include "cstring_t.h"

// ------------- global operators --------------------


/**
 * Concatenates a char array and a string
 * @author Hj. Malthaner
 */

/*
cstring_t operator+ (const char *buf, const cstring_t &other)
{
    const int len = strlen(buf) + other.len() + 1;

    char *tmp = new char[len];

    strcpy(tmp, buf);
    strcat(tmp, other);

    cstring_t result (tmp);
    delete tmp;

    return result;

}
*/

// ------------- cstring class --------------------


cstring_t::cstring_t()
{
    //printf("cstring_t::cstring_t()\n");

    buf = 0;
}


/**
 * Builds a uninitialised string (len() == -1)
 * @author Hj. Malthaner
 */
cstring_t::cstring_t(const char *other)
{
    //printf("cstring_t::cstring_t(const char *other)\n");
    const int len = strlen(other)+1;

    char *newbuf = new char[len];

    strcpy(newbuf, other);
    buf = newbuf;

    //printf(" buf = %s %p\n", buf, buf);
}


/**
 * Builds a string as a copy of a string
 * @author Hj. Malthaner
 */
cstring_t::cstring_t(const cstring_t &other)
{
    //printf("cstring_t::cstring_t(const cstring_t &other)\n");
    const int len = other.len()+1;

    buf = new char[len];

    strcpy(buf, other.buf);

    //printf(" buf = %s %p\n", buf, buf);
}


cstring_t::~cstring_t()
{
    //printf("cstring_t::~cstring_t()\n buf=%s %p\n", buf, buf);
    delete [] buf;
    buf = 0;
}


/**
 * Appends a char array to this string
 * @author Hj. Malthaner
 */
cstring_t cstring_t::operator+ (const char *other) const
{
    //printf("cstring_t cstring_t::operator+ (const char *other) const\n");
    const int len = strlen(buf) + strlen(other) + 1;

    //printf(" buf = %s  other = %s\n", buf, other);

    char *tmp = new char[len];

    strcpy(tmp, buf);
    strcat(tmp, other);

    cstring_t result;
    result.buf = tmp;

    //printf(" buf = %s %p\n", tmp, tmp);

    return result;
}

void cstring_t::set_at(int idx, char x) const
{
    if(idx > 0 && idx < len()) {
	buf[idx] = x;
    }
}



cstring_t & cstring_t::operator= (const cstring_t &other)
{
    // printf("cstring_t & cstring_t::operator= (const cstring &other)\n");
    char *newbuf = NULL;

    if(other.buf) {
      newbuf = new char[other.len()+1];

      strcpy(newbuf, other.buf);
    }
    delete buf;
    buf = newbuf;


    return *this;
}


/**
 * Comparison operator
 * @author Hj. Malthaner
 */
bool cstring_t::operator== (const cstring_t &other) const
{
  return strcmp(buf, other.buf) == 0;
}


bool cstring_t::operator!= (const cstring_t &other) const
{
  // printf("%s, %s\n   %d\n", buf, other.buf, strcmp(buf, other.buf) != 0);

  return strcmp(buf, other.buf) != 0;
}

bool cstring_t::operator== (const char *other) const
{
  return strcmp(buf, other) == 0;
}


bool cstring_t::operator!= (const char *other) const
{
  // printf("%s, %s\n   %d\n", buf, other.buf, strcmp(buf, other.buf) != 0);

  return strcmp(buf, other) != 0;
}


/**
 * Automagic conversion to a const char* for backwards compatibility
 * @author Hj. Malthaner
 */
cstring_t::operator const char *() const
{
    //printf("cstring_t::operator const char *() const\n");
    return buf;
}


/**
 * @return Number of characters in this string
 * @author Hj. Malthaner
 */
int cstring_t::len() const
{
    //printf("cstring_t::len() const\n");
    return buf ? ((int)strlen(buf)) : -1;
}


/**
 * Substring operator
 * @param first first char to include
 * @param last position after last char to include
 * @author Hj. Malthaner
 */
cstring_t cstring_t::substr(int first, int last)
{
  int   len  = last - first;
  char *dest = new char[len+1];
  char *src  = buf+first;

  for(int i=0; i<len; i++) {
    dest[i] = src[i];
  }

  dest[len] = '\0';


  cstring_t result;
  result.buf = dest;

  return result;
}

int cstring_t::find(char x) const
{
    char *p = buf;

    if(p && *p) {
	do {
	    if(*p == x) {
		return p  - buf;
	    }
	} while(*++p);
    }
    return -1;
}

int cstring_t::find(const char *text) const
{
    int l = strlen(text);
    int	n = len() - l + 1;

    for(int i = 0; i < n; i++) {
	if(!strncmp(text, buf, l)) {
	    return i;
	}
    }
    return -1;
}


int cstring_t::find_back(char x) const
{
    if(buf) {
	char *p = buf + len();

        while(p-- != buf) {
	    if(*p == x) {
		return p - buf;
	    }
	}
    }
    return -1;
}


/*
 *  member function:
 *      cstring_t::vprintf()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *	vsprintf mit cstring_t als Ziel.
 *
 *      Wie kriegt man die benötigte Länge raus? feste MaxLänge vorgeben?
 *	Das ist öde - hier schreiben wir probeweise nach /dev/nul um die
 *	Größe zu bestimmen - Performance nicht getestet.
 *
 *  Return type:
 *      int
 *
 *  Argumente:
 *      const char *format
 *      va_list args
 */
int cstring_t::vprintf(const char *format, va_list args)
{
    static FILE *nulfp = NULL;
    int newlen;

    if(!nulfp) {
	// init once
#ifdef _MSC_VER
	nulfp = fopen("nul", "wb");
#else
	nulfp = fopen("/dev/nul", "wb");
#endif
    }
    if(nulfp) {
	newlen = vfprintf(nulfp, format, args);

	delete buf;
	buf = new char[newlen + 1];

	newlen = vsprintf(buf, format, args);
    }
    else {
	char tmpbuf[4096];

	newlen = vsprintf(tmpbuf, format, args);

	delete buf;
	buf = new char[newlen + 1];

	strcpy(buf, tmpbuf);
    }

    return newlen;
}



/*
 *  member function:
 *      cstring_t::printf()
 *
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *	printf mit cstring_t als Ziel.
 *
 *  Return type:
 *      int
 *
 *  Argumente:
 *      const char *format
 *      ...
 */
int cstring_t::printf(const char *format, ...)
{
    va_list args;

    va_start(args, format);

    int len = vprintf(format, args);

    va_end(args);

    return len;
}


cstring_t csprintf(const char *format, ...)
{
    va_list args;
    cstring_t str;

    va_start(args, format);

    str.vprintf(format, args);

    va_end(args);

    return str;
}
