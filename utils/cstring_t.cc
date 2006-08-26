

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


cstring_t::cstring_t(const char *other)
{
	const int len = strlen(other)+1;
	buf = new char[len];
	strcpy(buf, other);
}


cstring_t::cstring_t(const cstring_t &other)
{
	const int len = other.len()+1;
	buf = NULL;
	if(len>0) {
		buf = new char[len];
		strcpy(buf, other.buf);
	}
}


cstring_t::~cstring_t()
{
	//printf("cstring_t::~cstring_t()\n buf=%s %p\n", buf, buf);
	if(buf) {
		delete [] buf;
		buf = 0;
	}
}


/**
 * Appends a char array to this string
 * @author Hj. Malthaner
 */
cstring_t cstring_t::operator+ (const char *other) const
{
    //printf("cstring_t cstring_t::operator+ (const char *other) const\n");
	if(buf) {
		const int tmplen = strlen(buf)+strlen(other) + 1;
		char *tmp = new char[tmplen];

		strcpy(tmp, buf);
		strcat(tmp, other);
		cstring_t result(tmp);
		delete [] tmp;
		return result;
	}
	return cstring_t(other);
}



void cstring_t::set_at(int idx, char x) const
{
    if(idx > 0 && idx < len()) {
	buf[idx] = x;
    }
}


// copy operator (act like constructor)
cstring_t & cstring_t::operator= (const cstring_t &other)
{
	delete [] buf;
	buf = NULL;
	if(other.len()>0) {
		buf = new char[other.len()+1];
		strcpy(buf, other.buf);
	}
	return *this;
}


cstring_t & cstring_t::operator= (const char *str)
{
	delete [] buf;
	buf = NULL;
	if(str) {
		buf = new char[strlen(str)+1];
		strcpy(buf, str);
	}
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


  cstring_t result(dest);
  delete [] dest;

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


// replaces all characters
int cstring_t::replace_character(char old,char ch)
{
	int many=0;
	char *p = buf;

	if(p && *p) {
		do {
			if(*p == old) {
				*p = ch;
				many ++;
			}
		} while(*++p);
	}
	return many;
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
	if(buf) {
		delete [] buf;
	}
	buf = new char[newlen + 1];
	newlen = vsprintf(buf, format, args);
    }
    else {
	char tmpbuf[4096];

	newlen = vsprintf(tmpbuf, format, args);
	if(buf) {
		delete [] buf;
	}
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
