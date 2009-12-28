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
	buf = NULL;
	if(other) {
		const size_t len = strlen(other)+1;
		buf = new char[len];
		strcpy(buf, other);
	}
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
		const size_t tmplen = strlen(buf)+strlen(other) + 1;
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


cstring_t & cstring_t::operator= (const cstring_t &other)
{
	if(buf!=NULL) {
		delete [] buf;
	}
	buf = NULL;
	if (other.len() >= 0) {
		buf = new char[other.len()+1];
		strcpy(buf, other.buf);
	}
	return *this;
}


cstring_t & cstring_t::operator= (const char *str)
{
	if(buf!=NULL) {
		delete [] buf;
	}
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
	return other == NULL ? buf==0 : strcmp(buf, other) == 0;
}


bool cstring_t::operator!= (const char *other) const
{
  // printf("%s, %s\n   %d\n", buf, other.buf, strcmp(buf, other.buf) != 0);

  return strcmp(buf, other) != 0;
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

long cstring_t::find(char x) const
{
	char *p = buf;

	if(p && *p) {
		do {
			if(*p == x) {
				return (long)(size_t)(p - buf);
			}
		} while(*++p);
	}
	return -1;
}

long cstring_t::find(const char *text) const
{
	size_t l = strlen(text);
	size_t n = len() - l + 1;

	for(size_t i = 0; i < n; i++) {
		if(!strncmp(text, buf, l)) {
			return (long)i;
		}
	}
	return -1;
}

long cstring_t::find_back(char x) const
{
	if(buf) {
		char *p = buf + len();

		while(p-- != buf) {
			if(*p == x) {
				return (long)(size_t)(p - buf);
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
