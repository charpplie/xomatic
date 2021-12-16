

//! check CRC against all installed network cards
//! /patam inMacAdd MAC Adresse in the format %02X%02X%02X%02X%02X%02X
//! /return true=CRC is ok, false=CRC is not valid (correct network card ist not installed of netbios is not present)
bool NetbiosHelper_CheckCRC( unsigned char inCrc[4] );

//! get CRC of first network card
//! /patam inMacAdd MAC Adresse in the format %02X%02X%02X%02X%02X%02X
//! /return true=operation was successfull, false=operation failed, no network card installed of netbios is not present
bool NetbiosHelper_GetCRC( unsigned char outCrc[4] );






