#ifndef CSTRING_T_H
#define CSTRING_T_H

#include <stdarg.h>


#ifdef _MSC_VER
#define STRICMP stricmp
#define STRNICMP strnicmp
#else
#define STRICMP strcasecmp
#define STRNICMP strncasecmp
#endif

/**
 * A simple string class.
 * @author Hj. Malthaner
 * @date 12-Jan-2002
 */
class cstring_t
{
private:
    char *buf;

public:


    /**
     * Builds a uninitialised string (len() == -1)
     * @author Hj. Malthaner
     */
    cstring_t();


    /**
     * Builds a string as a copy of a char array
     * @author Hj. Malthaner
     */
    cstring_t(const char *other);


    /**
     * Builds a string as a copy of a string
     * @author Hj. Malthaner
     */
    cstring_t(const cstring_t &other);


    ~cstring_t();


    /**
     * Concatenates this string and a char array
     * @author Hj. Malthaner
     */
    cstring_t operator+ (const char *) const;


    /**
     * Assignement operator
     * @author Hj. Malthaner
     */
    cstring_t & operator= (const cstring_t &);
    cstring_t & operator=(const char *);


    /**
     * Comparison operator
     * @author Hj. Malthaner
     */
    bool operator== (const cstring_t &) const;


    bool operator!= (const cstring_t &) const;


    bool operator== (const char *) const;


    bool operator!= (const char *) const;


    /**
     * Automagic conversion to a const char* for backwards compatibility
     * @author Hj. Malthaner
     */
    operator const char *() const;


    /**
     * Varargs like printf(...)  don't work with automagic conversion
     * so we need a explict conversion method
     * @author Hj. Malthaner
     */
    const char * chars() const {return buf;};

    /**
     * vsprintf() for a string
     *
     * @author V. Meyer
     */
    int vprintf(const char *format, va_list args);

    /**
     * sprintf() for a string
     *
     * @author V. Meyer
     */
    int printf(const char *format, ...);

    /**
     * @return Number of characters in this string
     * @author Hj. Malthaner
     */
    int len() const;


    /**
     * Substring operator
     * @param first first char to include
     * @param last position after last char to include
     * @author Hj. Malthaner
     */
    cstring_t substr(int first, int last);


    cstring_t right(int newlen)
    {
	int oldlen = len();

	if(newlen > oldlen)
	    return *this;
	else
	    return substr(oldlen - newlen, oldlen);
    }
    cstring_t left(int newlen)
    {
	int oldlen = len();

	if(newlen > oldlen)
	    return *this;
	else
	    return substr(0, newlen);
    }
    cstring_t mid(int start, int newlen = -1)
    {
	int oldlen = len();

	if(newlen == -1 || start + newlen > oldlen)
	    return substr(start, start + oldlen);
	else
	    return substr(start, start + newlen);
    }

	int replace_character( char old_ch, char new_ch);

    void set_at(int idx, char) const;

    int find(const char *) const;

    int find(char ) const;

    int find_back(char ) const;
};


/**
 * Concatenates a char array and a string
 * @author Hj. Malthaner
 */
//cstring_t operator+ (const char *, const cstring_t &);


cstring_t csprintf(const char *format, ...);

#endif
