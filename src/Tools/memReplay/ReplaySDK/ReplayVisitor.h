#pragma once

#include "ReplayLogDefs.h"

template <typename T>
class ReplayVisitor
{
public:
	ReplayVisitor(T& self) : m_self(&self) {}

	void Replay(ReplayRange range)
	{
		using namespace ReplayEventIds;

		ReplayEventIds::Ids id;
		while (range.ReadNext(id))
		{
			switch (id)
			{
			case RE_Alloc:            ReplayEvent(range.Get<ReplayAllocEvent>()); break;
			case RE_Free:             ReplayEvent(range.Get<ReplayFreeEvent>()); break;
			case RE_Callstack:        ReplayEvent(range.Get<ReplayCallstackEvent>()); break;
			case RE_FrameStart:       ReplayEvent(range.Get<ReplayFrameStartEvent>()); break;
			case RE_Label:            ReplayEvent(range.Get<ReplayLabelEvent>()); break;
			case RE_XenonModuleRef:   ReplayEvent(range.Get<ReplayXenonModuleRefEvent>()); break;
			case RE_AllocVerbose:     ReplayEvent(range.Get<ReplayAllocVerboseEvent>()); break;
			case RE_FreeVerbose:      ReplayEvent(range.Get<ReplayFreeVerboseEvent>()); break;
			case RE_Info:             ReplayEvent(range.Get<ReplayInfoEvent>()); break;
			case RE_PushContext:      ReplayEvent(range.Get<ReplayPushContextEvent>()); break;
			case RE_PopContext:       ReplayEvent(range.Get<ReplayPopContextEvent>()); break;
			case RE_Alloc3:           ReplayEvent(range.Get<ReplayAlloc3Event>()); break;
			case RE_Free3:            ReplayEvent(range.Get<ReplayFree3Event>()); break;
			case RE_PushContext2:     ReplayEvent(range.Get<ReplayPushContext2Event>()); break;
			case RE_PS3ModuleRef:     ReplayEvent(range.Get<ReplayPS3ModuleRefEvent>()); break;
			case RE_AddressProfile:   ReplayEvent(range.Get<ReplayAddressProfileEvent>()); break;
			case RE_PushContext3:     ReplayEvent(range.Get<ReplayPushContext3Event>()); break;
			case RE_Free4:            ReplayEvent(range.Get<ReplayFree4Event>()); break;
			case RE_AllocUsage:       ReplayEvent(range.Get<ReplayAllocUsageEvent>()); break;
			case RE_Info2:            ReplayEvent(range.Get<ReplayInfo2Event>()); break;
			case RE_Screenshot:       ReplayEvent(range.Get<ReplayScreenshotEvent>()); break;
			case RE_SizerPush:        ReplayEvent(range.Get<ReplaySizerPushEvent>()); break;
			case RE_SizerPop:         ReplayEvent(range.Get<ReplaySizerPopEvent>()); break;
			case RE_SizerAddRange:    ReplayEvent(range.Get<ReplaySizerAddRangeEvent>()); break;
			case RE_AddressProfile2:  ReplayEvent(range.Get<ReplayAddressProfile2Event>()); break;
			case RE_BucketMark:				ReplayEvent(range.Get<ReplayBucketMarkEvent>()); break;
			case RE_BucketMark2:			ReplayEvent(range.Get<ReplayBucketMark2Event>()); break;
			}
		}
	}

private:
	template <typename T> void ReplayEvent(const T& ev) { m_self->ReplayEvent(ev); }

private:
	T* m_self;
};
