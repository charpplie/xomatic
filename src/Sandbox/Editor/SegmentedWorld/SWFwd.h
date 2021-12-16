#ifndef __SWFwd_h__
#define __SWFwd_h__

// undef macros in WinBase.h
#undef LockSegment
#undef UnlockSegment

#ifndef SW_NAMESPACE_BEGIN
#	define SW_NAMESPACE_BEGIN() namespace sw{
#	define SW_NAMESPACE_END()   }
#endif

struct DataInspector;
struct TSegmentState;
class CSegmentedWorldDoc;
struct ISegmentedWorldDoc;
struct CSegmentedWorldManager;

namespace sw 
{
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

struct TLocalCoords;
struct TWorldCoords;
class CBaseObject;

#endif // __SWFwd_h__