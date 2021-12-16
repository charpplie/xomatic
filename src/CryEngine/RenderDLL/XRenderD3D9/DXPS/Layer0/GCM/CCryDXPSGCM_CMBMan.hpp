#ifndef __CRYDXPSGCM_CMBMAN__
#define __CRYDXPSGCM_CMBMAN__

#include "CCryDXPSGCM_SyncMan.hpp"

class CCryDXPSGCMCMBMan
{
private:
	tdResHandle					m_SegmentHandle[GCM_CMD_SEGMENTCOUNT];
	uint32							m_CmdBufferOffset;
	uint32							m_LastUsedSegment;
	int									m_Initialized;	//for debugging

	uint32							Segment(const uint32 Idx)const
											{
												return (Idx/GCM_CMD_SEGMENTSIZE)&GCM_CMD_SEGMENTMASK;
											}
public:
											CCryDXPSGCMCMBMan():
											m_Initialized(0){}

	void								Init();

	void								NextCMDs();
	void								FlushCMDs();
	uint32						  CmdBufferOffset() const{return m_CmdBufferOffset;}

	void*								SyncMemory();

	void								Size(class ICrySizer* Sizer);
};

#endif

