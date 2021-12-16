#pragma once

#include "SWFwd.h"
#include "SWCommon.h"

// --== Compile Flags ==--
#define SW_USE_INTEGRATED_METADATA
#define SW_NO_BUILTIN_BRANCH_MANAGING
#define SW_NO_REALTIME_UPDATE
#define SW_NO_DATACOMPRESS

#ifdef SW_NO_REALTIME_UPDATE
#	define SW_USE_DUMMY_LOCK
#endif



#ifdef SW_USE_DUMMY_LOCK
#	define AUTO_LOCK_BIG(lock)   AUTO_LOCK_T(::sw::_DummyLock, lock)
#	define AUTO_LOCK_MMUP(lock)   AUTO_LOCK_T(::CryCriticalSection, lock)
#	undef AUTO_LOCK
#	define AUTO_LOCK(lock) AUTO_LOCK_T(::sw::_DummyLock, lock)
#endif


#define SEGMENT_SIZE_METERS 512
#define SEGMENT_SIZE_UNITS (SEGMENT_SIZE_METERS/GetIEditor()->GetHeightmap()->GetUnitSize())
#define MAX_CACHE_SIZE (256 * 1024 * 1024)
//#define COMPILED_TERRAIN_TEXTURE_VERSION_FILE_NAME  "terrain\\cover.ver"


#include "Utilities.h"

// The Data Structure version of SW level that the current code can process
const sw::TSWDataStructureVersion g_nSWDataStructureVersion = 4;

#include "BaseTypes.h"

typedef unsigned char       byte;


SW_NAMESPACE_BEGIN();


class _DummyLock
{
public:
	_DummyLock(){}

	void Lock(){}
	bool TryLock(){}
	void Unlock(){}

#ifdef _DEBUG
	bool IsLocked(){}
#endif
};


template<typename T>
struct _AutoResetGuard
{
	T* m_guardee;
	T m_oldVal;
	explicit _AutoResetGuard(T* guardee, T const& newVal)
		:m_guardee(guardee)
	{
		m_oldVal = *m_guardee;
		*m_guardee = newVal;
	}

	~_AutoResetGuard() { *m_guardee = m_oldVal; }
};

// Path related const strings
// It is better be Macros, so that they can be used in combined string in sprintf patterns
#define SW_ExternalObjectDirName  "extobj"
#define SW_ExternalObjectDirPath  "extobj\\"
#define SW_SegmentsDirName  "seg"
#define SW_SegmentsDirPath  "seg\\"
#define SW_EditorDataDirPath "seg_editor\\"
#define SW_SegmentMainFile  "_Lock"
#define SW_SegmentDirPattern  "seg%d_%d"
#define SW_SegmentDirRegexp  "seg*_*"
#define SW_SegmentFileExtension ".dat"
#define SW_TempNewFileExtension ".tmpnew~"
#define SW_LevelPakFile "Level.pak"
#define SW_SegPakFile "seg.pak"
#define SW_ENTITY_MAP_FILE_NAME "entities.xml"

#define SW_BRUSH_BASE_NAME "brush\\"
#define SW_WORLD_DATA_BASE_NAME "LevelData\\"

#define SW_SegmentInfoFile "segmentinfo.xml"
#define SW_ObjectInfoFile "objectinfo.xml"
#define SW_LayerInfoFile SW_WORLD_DATA_BASE_NAME"layerinfo.xml"

#define SW_SegmentedLevelInfo "segmentedlevelinfo.xml"

static const char* g_szSegmentMainTag = "Segment";
static const char* g_szSWDStructVerAttr = "SWDataStructureVersion";
static const char* g_szLevelRootTag = "Level";


inline const char* GetFileBlockName(ESDBType eType)
{
	if (eType < SDB_ENGINEDATA_BEGIN || eType >= SDB_ENGINEDATA_END)
		return NULL;

	static const char *arrSFileBlockName[SDB_ENGINEDATA_END - SDB_ENGINEDATA_BEGIN] = {
		COMPILED_TERRAIN_TEXTURE_FILE_NAME,
		COMPILED_HEIGHT_MAP_FILE_NAME,
		SW_ENTITY_MAP_FILE_NAME,
		COMPILED_VISAREA_MAP_FILE_NAME,
		COMPILED_MERGED_MESHES_BASE_NAME,
		SW_BRUSH_BASE_NAME,
	};
	return arrSFileBlockName[eType - SDB_ENGINEDATA_BEGIN];
}

// segment data block types
inline const char* GetSegDataBlockName(ESDBType eType)
{
#define _GetSegDataBlockName_ENTRY(n) _szSDB[SDB_##n] = #n;
	struct _PODArrayInitializer{ _PODArrayInitializer(const char**_szSDB){
		_GetSegDataBlockName_ENTRY(Heights);
		_GetSegDataBlockName_ENTRY(LayerIDs);
		_GetSegDataBlockName_ENTRY(Vegetation);
		_GetSegDataBlockName_ENTRY(RGBLayer);
		_GetSegDataBlockName_ENTRY(ModSectors);
		_GetSegDataBlockName_ENTRY(EngData);
		_GetSegDataBlockName_ENTRY(EngRGBData);
		_GetSegDataBlockName_ENTRY(EngEntities);
		_GetSegDataBlockName_ENTRY(EngVisAreas);
		_GetSegDataBlockName_ENTRY(EngMergedMeshes);
		_GetSegDataBlockName_ENTRY(EngBrushes);
		_GetSegDataBlockName_ENTRY(Map);
		_GetSegDataBlockName_ENTRY(MapS2);
		_GetSegDataBlockName_ENTRY(MapS3);
	} };
	static const char *s_pcSDB[SDB_COUNT];
	static _PODArrayInitializer __init(s_pcSDB);

	return s_pcSDB[eType];

#undef _GetSegDataBlockName_ENTRY
}

inline const char* GetWorldDataBlockName(EWDBType eType)
{
	static const char* s_pcWDB[WDB_COUNT] = { 
		"Level", 
		"TerrainTextureLayers", 
		"VegetationMap",
		"TimeOfDay",
		"Environment",
		"LevelMisc",
	};

	return s_pcWDB[eType];
}

inline const char* GetWorldDataFileName(EWDBType eType)
{
	static const char* s_pWDFileNames[WDB_COUNT] = {
		SW_WORLD_DATA_BASE_NAME"Level.xml",		
		SW_WORLD_DATA_BASE_NAME"TerrainTextureLayers.xml",
		SW_WORLD_DATA_BASE_NAME"VegetationMap.xml",
		SW_WORLD_DATA_BASE_NAME"TimeOfDay.xml",
		SW_WORLD_DATA_BASE_NAME"Environment.xml",
		SW_WORLD_DATA_BASE_NAME"LevelMisc.dat",
	};

	return s_pWDFileNames[eType];
}

enum {
	WS_ENGINE_TERRAIN	=	0x00000001,
	WS_ENGINE_OBJECTS	=	0x00000002,
	WS_EDITOR_TERRAIN	=	0x00000004,
	WS_EDITOR_OBJECTS	=	0x00000008,
	WS_LOAD_ALL				=	0x0000000F,
	WS_UPDATE_STATUS	=	0x01000000,
	WS_FINALIZE					=	0x02000000,
};

enum 
{
	SF_WithSubmit = 0x1,
	SF_AsIntegrate = 0x10,	// TODO: remove this, integrate is no longer a form of save procedure
	SF_ForceSaveAll = 0x100,
	SF_LevelCreation = 0x1000,

	SF_RegularSave = SF_WithSubmit,
	SF_NoSubmitSave = 0,
	SF_IntegrateSave = SF_WithSubmit | SF_AsIntegrate,
};
typedef int ESaveFlag;




#ifdef SW_USE_DUMMY_LOCK
extern _DummyLock g_cs;
extern _DummyLock g_csBigLock;
#else
extern CryCriticalSection g_cs;
extern CryCriticalSection g_csBigLock;
#endif


SW_NAMESPACE_END();

namespace std {
	template<>
	struct hash<GUID>
	{
		size_t operator()(GUID kval) const { return kval.Data1; }
	};
	template <>
	struct equal_to<GUID> : public binary_function<GUID, GUID, bool> {
		bool operator()(const GUID& __x, const GUID& __y) const { return (bool)(__x == __y); }
	};
}



using sw::CSegmentDataAggr;
using sw::CObjectDataAggr;
using sw::TWorldData;
using sw::TSegmentData;
using sw::TLayerData;
using sw::TObjectData;
using sw::SegmentID;




//---------------------- world and local segment coordinates

struct TWorldCoords;

// in heightmap coordinate system
struct TLocalCoords {
	int lx, ly;
	inline TLocalCoords() {}
	inline TLocalCoords(int x, int y) { lx = x; ly = y; }
	TLocalCoords(const TWorldCoords &wc);

	inline TLocalCoords& operator=(const TLocalCoords& rhs) { lx=rhs.lx; ly=rhs.ly; return *this; }
	inline bool	operator==(const TLocalCoords& rhs) const { return lx==rhs.lx && ly==rhs.ly; }
	inline bool	operator!=(const TLocalCoords& rhs) const { return lx!=rhs.lx || ly!=rhs.ly; }
};

// in world coordinate system
struct TWorldCoords {
	int wx, wy;
	inline TWorldCoords() {}
	inline TWorldCoords(int x, int y) { wx = x; wy = y; }
	TWorldCoords(const TLocalCoords &lc);

	inline TWorldCoords& operator=(const TWorldCoords& rhs) { wx=rhs.wx; wy=rhs.wy; return *this; }
	inline bool	operator==(const TWorldCoords& rhs) const { return wx==rhs.wx && wy==rhs.wy; }
	inline bool	operator!=(const TWorldCoords& rhs) const { return wx!=rhs.wx || wy!=rhs.wy; }

	operator Vec2i() const { return Vec2i(wx,wy); }
	operator CPoint() const { return CPoint(wx,wy); }
};

typedef std::vector<TWorldCoords> TWorldCoordsArray; 
inline SegmentID MakeSegmentID(const TWorldCoords &wc)
{
	return ((wc.wy << 16) | (wc.wx & 0xFFFF));
}
inline TWorldCoords FromSegmentID(SegmentID segid)
{
	TWorldCoords wc;
	wc.wx = segid & 0xFFFF;
	wc.wy = (segid>>16) & 0xFFFF;
	return wc;
}
inline SegmentID MakeSegmentID(int wx, int wy)
{
	return ((wy << 16) | (wx & 0xFFFF));
}
inline void GetWorldCoords(SegmentID nSegmentID, int &wx, int &wy)
{
	wy = (int)((short)((nSegmentID >> 16)&0xFFFF));
	wx = (int)((short)(nSegmentID&0xFFFF));
}

inline void RGB2Seg(int &x, int &y)
{
}

inline void Seg2RGB(int &x, int &y)
{
}


//////////////////////////////////////////////////////////////////////////
// Data Inspector
// Helps profilers or loggers to get data from private members

struct DataInspector
{
#define DataInspector_Entry(fieldtype,fieldname) DataInspector_EntryMapAs(fieldtype,fieldname,fieldname)

#define DataInspector_EntryMapAs(fieldtype,fieldname,mapas) \
	template<class T> static inline fieldtype& mapas (T& host) { return host.##fieldname; } \
	template<class T> static inline fieldtype& mapas (T* host) { assert(host); return mapas (*host); }

	DataInspector_Entry(int, m_nLockedBy);
	DataInspector_Entry(string,m_strName);
	//DataInspector_Entry(int64,m_nAppliedHRID);
	DataInspector_Entry(int64,m_nLoadedHRID);
	DataInspector_Entry(TWorldCoords, m_wc);


#undef DataInspector_Entry
};


