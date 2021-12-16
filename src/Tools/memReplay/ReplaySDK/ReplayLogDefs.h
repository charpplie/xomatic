#ifndef REPLAYLOGDEFS_H
#define REPLAYLOGDEFS_H

namespace ReplayEventIds
{
	enum Ids
	{
		RE_Alloc,
		RE_Free,
		RE_Callstack,
		RE_FrameStart,
		RE_Label,
		RE_XenonModuleRef,
		RE_AllocVerbose,
		RE_FreeVerbose,
		RE_Info,
		RE_PushContext,
		RE_PopContext,
		RE_Alloc3,
		RE_Free3,
		RE_PushContext2,
		RE_PS3ModuleRef,
		RE_AddressProfile,
		RE_PushContext3,
		RE_Free4,
		RE_AllocUsage,
		RE_Info2,
		RE_Screenshot,
		RE_SizerPush,
		RE_SizerPop,
		RE_SizerAddRange,
		RE_AddressProfile2,
		RE_Alloc64,
		RE_Free64,
		RE_BucketMark,
		RE_BucketMark2,
	};
};

namespace MemStatContextTypes
{
	enum Type
	{
		MSC_MAX = 0,
		MSC_CGF = 1,
		MSC_MTL = 2,
		MSC_DBA = 3,
		MSC_CHR = 4,
		MSC_LMG = 5,
		MSC_AG = 6,
		MSC_Texture = 7,
		MSC_ParticleLibrary = 8,

		MSC_Physics = 9,
		MSC_Terrain = 10,
		MSC_Shader = 11,
		MSC_Other = 12,
		MSC_RenderMesh = 13,
		MSC_Entity = 14,
		MSC_Navigation = 15,
		MSC_ScriptCall = 16,

		MSC_CDF = 17,

		MSC_RenderMeshType = 18,

		MSC_ANM = 19,
		MSC_CGA = 20,
		MSC_CAF = 21,
		MSC_ArchetypeLib = 22,

		MSC_SoundProject = 23,
		MSC_EntityArchetype=24,

		MSC_LUA = 25,
		MSC_D3D = 26,
		MSC_ParticleEffect = 27,
		MSC_SoundBuffer = 28,
		MSC_FSB = 29,  // Sound bank data

		MSC_Last = 30,  // Last Enum
	};


	const char* ToString(Type type);
}

namespace MemStatContextFlags
{
	enum Flags
	{
		MSF_None = 0,
		MSF_Instance = 1,
	};
}

#pragma pack(push)
#pragma pack(1)
struct ReplayEventHeader
{
	u8 sequenceCheck;
	u8 eventId;
	u16 eventLength;

	friend void SwapEndian(ReplayEventHeader& ev)
	{
		SwapEndian(ev.eventLength);
	}
};

struct ReplayAllocEvent
{
	enum { EventId = ReplayEventIds::RE_Alloc };

	u32 callstackId;
	u32 ptr;
	s32 size;

	friend void SwapEndian(ReplayAllocEvent& ev)
	{
		SwapEndian(ev.callstackId);
		SwapEndian(ev.ptr);
		SwapEndian(ev.size);
	}
};

struct ReplayFreeEvent
{
	enum { EventId = ReplayEventIds::RE_Free };

	u32 ptr;

	friend void SwapEndian(ReplayFreeEvent& ev)
	{
		SwapEndian(ev.ptr);
	}
};

struct ReplayCallstackEvent
{
	enum { EventId = ReplayEventIds::RE_Callstack };

	u32 id;
	u32 callstack[256];

	friend void SwapEndian(ReplayCallstackEvent& ev)
	{
		SwapEndian(ev.id);
		for (int i = 0; i < 256; ++ i)
		{
			SwapEndian(ev.callstack[i]);
		}
	}
};

struct ReplayFrameStartEvent
{
	enum { EventId = ReplayEventIds::RE_FrameStart };

	u32 frameId;

	friend void SwapEndian(ReplayFrameStartEvent& ev)
	{
		SwapEndian(ev.frameId);
	}
};

struct ReplayLabelEvent
{
	enum { EventId = ReplayEventIds::RE_Label };

	char label[32];

	friend void SwapEndian(ReplayLabelEvent& ev)
	{
	}
};

struct ReplayXenonModuleRefEvent
{
	enum { EventId = ReplayEventIds::RE_XenonModuleRef };

	char name[256];
	char path[256];
	char signature[512];

	friend void SwapEndian(ReplayXenonModuleRefEvent& ev) {}
};

struct ReplayPS3ModuleRefEvent
{
	enum { EventId = ReplayEventIds::RE_PS3ModuleRef };

	char name[256];

	friend void SwapEndian(ReplayPS3ModuleRefEvent& ev) {}
};

struct ReplayAllocVerboseEvent
{
	enum { EventId = ReplayEventIds::RE_AllocVerbose };

	u32 callstackId;
	u32 ptr;
	s32 sizeRequested;
	s32 sizeConsumed;
	s32 sizeGlobal;

	friend void SwapEndian(ReplayAllocVerboseEvent& ev)
	{
		SwapEndian(ev.callstackId);
		SwapEndian(ev.ptr);
		SwapEndian(ev.sizeRequested);
		SwapEndian(ev.sizeConsumed);
		SwapEndian(ev.sizeGlobal);
	}
};

struct ReplayFreeVerboseEvent
{
	enum { EventId = ReplayEventIds::RE_FreeVerbose };

	u32 ptr;
	s32 sizeGlobal;

	friend void SwapEndian(ReplayFreeVerboseEvent& ev)
	{
		SwapEndian(ev.ptr);
		SwapEndian(ev.sizeGlobal);
	}
};

struct ReplayInfoEvent
{
	enum { EventId = ReplayEventIds::RE_Info };

	u32 preTrackSize;
	u32 trackingSize;

	friend void SwapEndian(ReplayInfoEvent& ev)
	{
		SwapEndian(ev.preTrackSize);
		SwapEndian(ev.trackingSize);
	}
};

struct ReplayInfo2Event
{
	enum { EventId = ReplayEventIds::RE_Info2 };

	u32 preTrackSize;
	u32 trackingSize;
	u32 bucketsFree;

	friend void SwapEndian(ReplayInfo2Event& ev)
	{
		SwapEndian(ev.preTrackSize);
		SwapEndian(ev.trackingSize);
		SwapEndian(ev.bucketsFree);
	}
};

struct ReplayPushContextEvent
{
	enum { EventId = ReplayEventIds::RE_PushContext };

	u32 threadId;
	char name[256];

	u32 contextType;

	friend void SwapEndian(ReplayPushContextEvent& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.contextType);
	}
};

struct ReplayPushContext2Event
{
	enum { EventId = ReplayEventIds::RE_PushContext2 };

	u32 threadId;
	u32 contextType;

	char name[1];

	friend void SwapEndian(ReplayPushContext2Event& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.contextType);
	}
};

struct ReplayPushContext3Event
{
	enum { EventId = ReplayEventIds::RE_PushContext3 };

	u32 threadId;
	u32 contextType;
	u32 flags;

	char name[1];

	friend void SwapEndian(ReplayPushContext3Event& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.contextType);
		SwapEndian(ev.flags);
	}
};

struct ReplayPopContextEvent
{
	enum { EventId = ReplayEventIds::RE_PopContext };

	u32 threadId;

	friend void SwapEndian(ReplayPopContextEvent& ev)
	{
		SwapEndian(ev.threadId);
	}
};

struct ReplayAlloc3Event
{
	enum { EventId = ReplayEventIds::RE_Alloc3 };

	u32 threadId;
	u32 ptr;
	u32 sizeRequested;
	u32 sizeConsumed;
	s32 sizeGlobal;

	u32 callstackLength;
	u32 callstack[1];

	friend void SwapEndian(ReplayAlloc3Event& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.ptr);
		SwapEndian(ev.sizeRequested);
		SwapEndian(ev.sizeConsumed);
		SwapEndian(ev.sizeGlobal);
		SwapEndian(ev.callstackLength);
		for (int i = 0; i != ev.callstackLength; ++ i)
		{
			SwapEndian(ev.callstack[i]);
		}
	}
};

struct ReplayFree3Event
{
	enum { EventId = ReplayEventIds::RE_Free3 };

	u32 threadId;
	u32 ptr;
	s32 sizeGlobal;

	friend void SwapEndian(ReplayFree3Event& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.ptr);
		SwapEndian(ev.sizeGlobal);
	}
};

struct ReplayFree4Event
{
	enum { EventId = ReplayEventIds::RE_Free4 };

	u32 threadId;
	u32 ptr;
	s32 sizeGlobal;

	u32 callstackLength;
	u32 callstack[1];

	friend void SwapEndian(ReplayFree4Event& ev)
	{
		SwapEndian(ev.threadId);
		SwapEndian(ev.ptr);
		SwapEndian(ev.sizeGlobal);
		SwapEndian(ev.callstackLength);
		for (int i = 0; i != ev.callstackLength; ++ i)
		{
			SwapEndian(ev.callstack[i]);
		}
	}
};

struct ReplayAddressProfileEvent
{
	enum { EventId = ReplayEventIds::RE_AddressProfile };

	u32 rsxStart;

	friend void SwapEndian(ReplayAddressProfileEvent& ev)
	{
		SwapEndian(ev.rsxStart);
	}
};

struct ReplayAddressProfile2Event
{
	enum { EventId = ReplayEventIds::RE_AddressProfile2 };

	u32 rsxStart;
	u32 rsxLength;

	friend void SwapEndian(ReplayAddressProfile2Event& ev)
	{
		SwapEndian(ev.rsxStart);
		SwapEndian(ev.rsxLength);
	}
};

struct ReplayAllocUsageEvent
{
	enum { EventId = ReplayEventIds::RE_AllocUsage };

	u32 ptr;
	u32 used;

	friend void SwapEndian(ReplayAllocUsageEvent& ev)
	{
		SwapEndian(ev.ptr);
		SwapEndian(ev.used);
	}
};

struct ReplayScreenshotEvent
{
	enum { EventId = ReplayEventIds::RE_Screenshot };

	u8 bmp[1];

	friend void SwapEndian(ReplayScreenshotEvent& ev)
	{
	}
};

struct ReplaySizerPushEvent
{
	enum { EventId = ReplayEventIds::RE_SizerPush };

	char name[1];

	friend void SwapEndian(ReplaySizerPushEvent& ev) {}
};

struct ReplaySizerPopEvent
{
	enum { EventId = ReplayEventIds::RE_SizerPop };

	friend void SwapEndian(ReplaySizerPopEvent& ev) {}
};

struct ReplaySizerAddRangeEvent
{
	enum { EventId = ReplayEventIds::RE_SizerAddRange };

	u32 ptr;
	u32 size;
	u32 count;

	friend void SwapEndian(ReplaySizerAddRangeEvent& ev)
	{
		SwapEndian(ev.ptr);
		SwapEndian(ev.size);
		SwapEndian(ev.count);
	}
};

struct ReplayBucketMarkEvent
{
	enum { EventId = ReplayEventIds::RE_BucketMark };

	u32 ptr;
	u32 length;
	s32 index;

	friend void SwapEndian(ReplayBucketMarkEvent& ev)
	{
		SwapEndian(ev.ptr);
		SwapEndian(ev.length);
		SwapEndian(ev.index);
	}
};

struct ReplayBucketMark2Event
{
	enum { EventId = ReplayEventIds::RE_BucketMark2 };

	u32 ptr;
	u32 length;
	s32 index;
	u32 alignment;

	friend void SwapEndian(ReplayBucketMark2Event& ev)
	{
		SwapEndian(ev.ptr);
		SwapEndian(ev.length);
		SwapEndian(ev.index);
		SwapEndian(ev.alignment);
	}
};

#pragma pack(pop)

#endif
