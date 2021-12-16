#ifndef __CRYDXPSGCM_PRECOMPILECMBMAN__
#define __CRYDXPSGCM_PRECOMPILECMBMAN__

enum {GCM_CMDPC_SIZE					=	0x100000};	// 1MB for precompiled commandbuffer
enum {GCM_CMDPC_GUARDSIZE			=	0x10000};	// 16kB for precompiled commandbuffer

class CCryDXPSPCSegment
{
	uint32							m_Size:24;//assuming never more than 16MB precompiled commandbuffer
	uint32							m_InUse:8;
	bool								TryMerge(CCryDXPSPCSegment* pNext)
											{
												if(!InUse()&&!pNext->InUse())
												{
													m_Size	+= pNext->Size()+sizeof(CCryDXPSPCSegment);
													return true;
												}
												return false;
											}
public:
											CCryDXPSPCSegment(uint32 Size,uint32 InUse=0):
											m_Size(Size-sizeof(CCryDXPSPCSegment)),
											m_InUse(InUse){}
	uint8*							Data(){return reinterpret_cast<uint8*>(this+1);}
	uint32							Size()const{return m_Size;}
	uint32							InUse()const{return m_InUse;}
	void								Use(){uint32 IU=m_InUse;IU++;if(IU>255)IU=255;m_InUse=IU;}

	bool								Equal(uint8* pData,uint32 Size)
											{
												if(Size!=m_Size)
													return false;

												uint8* pData2	=	Data();
												for(uint32 a=0;a<Size;a++)
													if(pData[a]!=pData2[a])
														return false;
												return true;
											}
	void								SplitAt(uint32 Offset)
											{
												if(Offset>=Size())
													return;

												uint8* pData	=	Data();
												new (&pData[Offset])CCryDXPSPCSegment(Size()-Offset);
												m_Size	=	Offset;
											}

	CCryDXPSPCSegment*	Next()
											{
												CCryDXPSPCSegment* pNext;
												do
												{
													uint8* pData	=	Data();
													pNext	=	reinterpret_cast<CCryDXPSPCSegment*>(&pData[Size()]);
													if(m_Size==0)
														return 0;
												}while(TryMerge(pNext));
												return pNext->Size()?pNext:0;
											}
};

class CCryDXPSGCMPrecompileCMBMan
{
private:
	uint8								m_TmpBuffer[GCM_CMDPC_GUARDSIZE];
	uint8*							m_pCmdBuffer;
	uint32							m_BufferOffsetOrigin;
	uint32							m_CmdStart;
	CCryDXPSPCSegment*	m_pFirst;
	int									m_Initialized;

	CCryDXPSPCSegment*	FindInCache(uint8* pData,uint32 S);
	CCryDXPSPCSegment*	Allocate(uint32 S);
public:
											CCryDXPSGCMPrecompileCMBMan():
											m_Initialized(0){}

	void								Init();
	bool								EnableCMD();
	uint32							DisableCMD();
	
	void								Size(class ICrySizer* Sizer);
} _ALIGN(32);//put into a single cache line

#endif

