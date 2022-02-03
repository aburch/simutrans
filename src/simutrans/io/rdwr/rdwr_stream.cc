/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "rdwr_stream.h"


rdwr_stream_t::rdwr_stream_t(bool writing) :
	status(STATUS_INVALID),
	writing(writing)
{
}
