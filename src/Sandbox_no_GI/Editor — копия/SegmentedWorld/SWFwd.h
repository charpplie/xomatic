#ifndef __SWFwd_h__
#define __SWFwd_h__

#if _MSC_VER > 1000
#pragma once
#endif



#undef LockSegment
#undef UnlockSegment

#ifndef SW_NAMESPACE_BEGIN
#	define SW_NAMESPACE_BEGIN() namespace sw{
#	define SW_NAMESPACE_END()   }
#endif

struct DataInspector;
struct TSegmentState;
class CSegmentedWorldDoc;
typedef CSegmentedWorldDoc CSWDoc;
struct ISegmentedWorldDoc;
typedef ISegmentedWorldDoc ISWDoc;
struct CSegmentedWorldManager;
typedef CSegmentedWorldManager CSWManager;
namespace sw {
	typedef uint32 SegmentID;
	typedef uint32 TSWDataStructureVersion;
	class CIgnoreChangesAutoGuard;
	class CSWModeFakeAutoGuard;
	struct TWorldName;
	struct TViewStates;
	enum EVersionedDataType; 
	enum EVersionType;
	struct TCheckOutContext;
	class ISWVersionControl;
	typedef _smart_ptr<ISWVersionControl> TVersionControlPtr;
	class CSCDataPersistence;
	typedef CSCDataPersistence ISWDataPersistence;
	typedef ISWDataPersistence* TPersistenceWeakPtr;
	typedef _smart_ptr<ISWDataPersistence> TPersistencePtr;

	struct TWorldData;
	struct TSegmentData;
	struct TSegDataBlock;
	struct TLayerData;
	struct TObjectData;
	class CSegmentDataAggr;
	class CObjectDataAggr;
}

using sw::TSWDataStructureVersion;

struct TLocalCoords;
struct TWorldCoords;
typedef std::pair<GUID, sw::TObjectData*> TObjectDataPair;
typedef std::map<GUID, sw::TLayerData*, guid_less_predicate> TLayerMap;
typedef std::pair<GUID, sw::TLayerData*> TLayerPair;

//class IDBConnection;

class CBaseObject;

#endif // __SWFwd_h__