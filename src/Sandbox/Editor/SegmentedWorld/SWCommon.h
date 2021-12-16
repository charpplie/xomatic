#ifndef __SWCommon_h__
#define __SWCommon_h__

#if _MSC_VER > 1000
#pragma once
#endif

#include "DocMultiArchive.h"

namespace sw{

	static const TSWDataStructureVersion SWDStructVer_Invalid = (TSWDataStructureVersion)-1;

	// world data block types
	enum EWDBType
	{
		WDB_LEVELGENERAL = DMAS_GENERAL,
		WDB_TERRAIN_LAYERS = DMAS_TERRAIN_LAYERS,
		WDB_VEGETATION = DMAS_VEGETATION,
		WDB_TIMEOFDAY = DMAS_TIME_OF_DAY,
		WDB_ENVIRONMENT = DMAS_ENVIRONMENT,
		WDB_LEVELNAMEDDATA = DMAS_GENERAL_NAMED_DATA,
		WDB_XMLCOUNT,
		//WDB_LEVELPAK = WDB_XMLCOUNT,
		WDB_COUNT = WDB_XMLCOUNT,
		WDB_UNSPECIFIED = -1,
	};

	// segmented data block types
	enum ESDBType
	{
		SDB_SOURCEDATA_BEGIN = 0,
		SDB_Heights = SDB_SOURCEDATA_BEGIN,
		SDB_LayerIDs,
		SDB_Vegetation,
		SDB_RGBLayer,
		SDB_ModSectors,
		SDB_SOURCEDATA_END,

		SDB_ENGINEDATA_BEGIN = SDB_SOURCEDATA_END,
		SDB_EngRGBData = SDB_ENGINEDATA_BEGIN,
		SDB_EngData,
		SDB_EngEntities,
		SDB_EngVisAreas,
		SDB_EngMergedMeshes,
		SDB_EngBrushes,
		SDB_ENGINEDATA_END,

		SDB_MAP_BEGIN = SDB_ENGINEDATA_END,
		SDB_Map = SDB_MAP_BEGIN,
		SDB_MapS2,
		SDB_MapS3,
		SDB_MAP_END,

		SDB_COUNT = SDB_MAP_END,
	};

	template<class TRaw>
	struct EnumIteratorT
	{
		//typedef ESDBType TRaw;
		typedef EnumIteratorT<TRaw> TSelf;
		typedef int TStorage;

		explicit EnumIteratorT(TStorage rhs) : val(rhs) {}
		EnumIteratorT(TRaw rhs) : val(rhs) {}
		operator TRaw()const { return (TRaw)val; }
		TSelf operator++() { ++val;return *this; }
		TSelf operator++(int) { TSelf tmp = *this; ++val; return tmp; }
		bool operator == (TRaw rhs)const { return val == rhs; }
		bool operator != (TRaw rhs)const { return val != rhs; }
	private:
		TStorage val;
	};
	typedef EnumIteratorT<ESDBType> ESDBTypeIterator;

	inline bool IsSourceDataBlock( ESDBType eType ) { return eType >= SDB_SOURCEDATA_BEGIN && eType < SDB_SOURCEDATA_END; }
	inline bool IsEngineDataBlock( ESDBType eType ) { return eType >= SDB_ENGINEDATA_BEGIN && eType < SDB_ENGINEDATA_END; }
	inline bool IsMinimapDataBlock( ESDBType eType ) { return eType >= SDB_MAP_BEGIN && eType < SDB_MAP_END; }

	struct TWorldName
	{
		string Name;
		bool isSegmented;
	};
}


#endif // __SWCommon_h__
