#ifndef __CRY_HASH_UTIL_H__
#define __CRY_HASH_UTIL_H__

//-----------------------------------------------------------------------------------
// HASH Tools.
// from Frd's code-base courtesy of AW. /FH
typedef uint32 CryHash;

CryHash HashStringSeed( const char* string, const uint32 seed );
CryHash HashString( const char* string );


#endif // __CRY_HASH_UTIL_H__