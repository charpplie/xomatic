/************************************************************************/
/* Player Vis Table
/************************************************************************/

#ifndef PLAYER_VIS_TABLE_H
#define PLAYER_VIS_TABLE_H

#define ALLOW_VISTABLE_DEBUGGING 1

#include "IDeferredRaycastManager.h"

typedef int16 VisEntryIndex;
typedef VisEntryIndex VisEntryCount;

static const int		kMaxVisTableLinetestsPerFrame						= 5;
static const float	kVisTableDefaultZAxisOffset											= 1.4f;

enum
{
	eVF_None			= (0),
	eVF_Visible		= (1 << 0),
	eVF_Remove		= (1 << 1),
	eVF_Requested = (1 << 2),
	eVF_Pending		= (1 << 3)
};

typedef uint16 eVisibilityFlags;

typedef struct SVisTableEntry
{
	SVisTableEntry()
	{
		Reset();
	}

	ILINE void Reset()
	{
		entityId							= 0;
		flags									= eVF_None;
		framesSinceLastCheck	= 255;
		lastRequestedLatency	= 0;
		heightOffset					= kVisTableDefaultZAxisOffset;
	}

	EntityId								entityId;
	float										heightOffset;
	eVisibilityFlags				flags;
	uint8										framesSinceLastCheck;
	uint8										lastRequestedLatency;

} SVisTableEntry;


typedef struct SDeferredLinetestReceiver : public IDeferredRaycastReceiver
{
	SDeferredLinetestReceiver()
	{
		SetRaycastHelper();
		visTableIndex = -1;
		visBufferIndex = -1;
	}

	//async raycasts results callback
	virtual void OnDataReceived(const EventPhysRWIResult *pRWIResult);

	//async primitive casts results callback
	virtual void OnDataReceived(const EventPhysPWIResult *pPWIResult) {}

	//reset data callback
	virtual void OnDRWReset() {}

	void SetRaycastHelper()		{ raycastHelper.SetReceiver(this); }

	ILINE void SetFree()			{ visBufferIndex = -1;				}
	ILINE bool IsFree()				{ return visBufferIndex == -1;}	

	ILINE void SetInvalid()		{ visTableIndex = -1;					}
	ILINE bool IsValid()			{ return visTableIndex != -1; }

	CDeferredRaycastHelper	raycastHelper;
	VisEntryIndex						visTableIndex;
	int8										visBufferIndex;

} SVisTableProcessing;


typedef struct SVisTablePriority
{
	SVisTableEntry *	visInfo;
	int16							priority;
	VisEntryIndex			visIndex;
} SVisTablePriority;


//This is used to allow the vis table entries to be double buffered
typedef struct SDeferredLinetestBuffer
{
	SDeferredLinetestBuffer()
	{
		m_numLinetestsCurrentlyProcessing = 0;
	}

	SDeferredLinetestReceiver m_deferredLinetestReceivers[kMaxVisTableLinetestsPerFrame];
	VisEntryCount							m_numLinetestsCurrentlyProcessing;
} SDeferredLinetestBuffer;


class CPlayerVisTable
{
public:
	CPlayerVisTable();
	~CPlayerVisTable();

	bool CanLocalPlayerSee(EntityId entityId);
	bool CanLocalPlayerSee(EntityId entityId, uint8 acceptableFrameLatency, const float heightOffset = kVisTableDefaultZAxisOffset);
	inline SVisTableEntry& GetNthVisTableEntry(int32 n) { return m_visTableEntries[n]; }

	void Update(float dt);

	void Reset();
	SDeferredLinetestBuffer& GetDeferredLinetestBuffer(int n) { return m_linetestBuffers[n]; }

private:
	void	DoVisibilityCheck(const Vec3& localPlayerPosn, SVisTableEntry& visInfo, VisEntryIndex visIndex);
	void	GetLocalPlayerPosn(Vec3& localPlayerPosn);

	VisEntryIndex	GetEntityIndexFromID(EntityId entityId);
	void	ClearRemovedEntities();
	int		AddVisTableEntriesToPriorityList();
	void	UpdatePendingDeferredLinetest(int source, int dest);
	void	RemovePendingDeferredLinetest(int index);
	
	SDeferredLinetestReceiver* GetDeferredLinetestReceiverFromVisTableIndex(VisEntryIndex index, int bufferIndex);
	SDeferredLinetestReceiver* GetAvailableDeferredLinetestReceiver(SDeferredLinetestBuffer& visBuffer);

	inline int GetCurrentLinetestBufferTargetIndex()			{ return m_currentBufferTarget; }

	void RemoveNthEntity(int n);
	
	static const int		kMinUnusedFramesBeforeEntryRemoved = 20;
	static const int		kMaxVisTableEntries							= 128;
	static const int		kDefaultAcceptableLatency				= 10;
	static const int		kNumVisTableBuffers							=	2;

	SDeferredLinetestBuffer			m_linetestBuffers[kNumVisTableBuffers];

	SVisTableEntry			m_visTableEntries[kMaxVisTableEntries];

	SVisTablePriority 	m_visTablePriorities[kMaxVisTableLinetestsPerFrame];

	VisEntryCount				m_numUsedVisTableEntries;
	VisEntryCount				m_numLinetestsThisFrame;

	uint8								m_currentBufferTarget;
	uint8								m_currentBufferProcessing;


#if ALLOW_VISTABLE_DEBUGGING
	int m_numQueriesThisFrame;
#endif
};

#endif