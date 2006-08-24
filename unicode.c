/* Unicode/UTF stuff
 * taken from PalmDict (see sourceforge)
*/
// UTF-8 encouding values
#define UTF8_VALUE1     0x00        // Value for set bits for single byte UTF-8 Code.
#define UTF8_MASK1      0x80        // Mask (i.e. bits not set by the standard) 0xxxxxxx
#define UTF8_WRITE1     0xff80      // Mask of bits we cannot allow if we are going to write one byte code
#define UTF8_VALUE2     0xc0        // Two byte codes
#define UTF8_MASK2      0xe0        // 110xxxxx 10yyyyyy
#define UTF8_WRITE2     0xf800      // Mask of mits we cannot allow if we are going to write two byte code
#define UTF8_VALUE3     0xe0        // Three byte codes
#define UTF8_WRITE3     0xffff0000      // Mask of mits we cannot allow if we are going to write three byte code (16 Bit)
#define UTF8_MASK3      0xf0        // 1110xxxx 10yyyyyy 10zzzzzz
#define UTF8_VALUE4     0xf0        // Four byte values
#define UTF8_MASK4      0xf8        // 11110xxx ----    (These values are not supported).
#define UTF8_VALUEC     0x80        // Continueation byte (10xxxxxx).
#define UTF8_MASKC      0xc0


// find next unicode character
int unicode_get_next_character( const char *text, int cursor_pos)
{
	const unsigned char *ptr=text;
	ptr += cursor_pos;

	if (UTF8_VALUE1 == (*ptr & UTF8_MASK1)  ||  *ptr<0x80  ) {
		return cursor_pos + 1;
	}
	else {
		if (UTF8_VALUE2 == (*ptr & UTF8_MASK2)) 	{
			return cursor_pos + 2;
		}
		else {
			if (UTF8_VALUE3 == (*ptr & UTF8_MASK3)) {
				return cursor_pos + 3;
			}
		}
	}
	// should never ever get here!
	return cursor_pos + 1;
}


// find previous character border
int unicode_get_previous_character( const char *text, int cursor_pos)
{
	int pos=0, last_pos=0;
	// find previous character border
	while(  pos<cursor_pos  &&  text[pos]!=0  ) {
		last_pos = pos;
		pos = unicode_get_next_character( text, pos );
	}
	return last_pos;
}


/* converts UTF8 to Unicode with save recovery
 */
unsigned short utf82unicode (unsigned char const *ptr, int *iLen )
{
	unsigned short iUnicode=0;

	if (UTF8_VALUE1 == (*ptr & UTF8_MASK1)) {
		iUnicode = ptr[0];
		(*iLen) += 1;
	}
	else {
		if (UTF8_VALUE2 == (*ptr & UTF8_MASK2)) 	{
			// error handling ...
			if(  ptr[1]<=127  ) {
				(*iLen) += 1;
				return ptr[0];
			}
			//    iUnicode = (((*ptr)[1] & 0x1f) << 6) | ((*ptr)[2] & 0x3f);
			iUnicode = ptr[0] & 0x1f;
			iUnicode <<= 6;
			iUnicode |= ptr[1] & 0x3f;
			(*iLen) += 2;
		}
		else {
			if (UTF8_VALUE3 == (*ptr & UTF8_MASK3)) {
				// error handling ...
				if(  (ptr[1]<=127)    ||   (ptr[2]<=127)  ) {
					(*iLen) += 1;
					return ptr[0];
				}
				iUnicode = ptr[0] & 0x0f;
				iUnicode <<= 6;
				iUnicode |= ptr[1] & 0x3f;
				iUnicode <<= 6;
				iUnicode |= ptr[2] & 0x3f;
				(*iLen) += 3;
			}
			else {
				// 4 Byte unicode not supported
				(*iLen) += 1;
				return ptr[0];
			}
		}
	}
	return iUnicode;
}


// Converts Unicode to UTF8 sequence
int	unicode2utf8( unsigned unicode, unsigned char *out )
{
	if(  unicode<0x0080u  )
	{
		*out++ = unicode;
		return 1;
	}
	else if(  unicode<0x0800u  )
	{
		*out++ = 0xC0|(unicode>>6);
		*out++ = 0x80|(unicode&0x3F);
		return 2;
	}
	else // if(  lUnicode<0x10000l  ) immer TRUE!
	{
		*out++ = 0xE0|(unicode>>12);
		*out++ = 0x80|((unicode>>6)&0x3F);
		*out++ = 0x80|(unicode&0x3F);
		return 3;
	}
}
