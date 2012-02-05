#include "pwd_hash.h"

#include "../utils/sha1.h"



void pwd_hash_t::set(SHA1& sha)
{
	sha.Result(hash);
}
