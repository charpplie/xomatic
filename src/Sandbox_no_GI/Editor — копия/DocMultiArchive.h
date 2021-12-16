#ifndef DocMultiArchive_h__
#define DocMultiArchive_h__

#pragma once



class CXmlArchive;

// Segmented World need XML Archive of editor level being split into multiple Archives
// Because multiArchive is a super set of single archive, Doc can always use it, with or without SW Level
// so some of the interface functions was written as always using MultiArchive
// However, to decouple other sandbox interfaces with SW stuff(prevent important header files from being polluted),
// it's better put it here as a generic implement and be included in both sides

// the current avaliable slots for MultiArchive
// used by segmented world code through enum mapping (to EWDBType)
enum EDocMultiArchiveSlot
{
	DMAS_GENERAL = 0,
	DMAS_TERRAIN_LAYERS,
	DMAS_VEGETATION,
	DMAS_TIMEOFDAY,
	DMAS_GENERAL_NAMED_DATA,
	DMAS_COUNT,
};


typedef CXmlArchive* TDocMultiArchive [DMAS_COUNT];
inline void FillXmlArArray(TDocMultiArchive &arrXmlAr, CXmlArchive* pXmlAr)
{
	for (int i = 0 ; i < DMAS_COUNT; i++)
	{
		arrXmlAr[i] = pXmlAr;
	}
}



#endif // DocMultiArchive_h__
