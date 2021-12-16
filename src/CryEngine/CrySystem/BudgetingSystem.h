#ifndef _C_BUDGETING_SYSTEM_
#define _C_BUDGETING_SYSTEM_

#pragma once


#include <IBudgetingSystem.h>


struct IRenderer;
struct IRenderAuxGeom;
struct ITimer;
struct ISoundSystem;
struct IStreamEngine;


class CBudgetingSystem : public IBudgetingSystem
{
public:
	CBudgetingSystem();

	// interface
	VIRTUAL void SetSysMemLimit( int sysMemLimitInMB );
	VIRTUAL void SetVideoMemLimit( int videoMemLimitInMB );
	VIRTUAL void SetFrameTimeLimit( float frameLimitInMS );
	VIRTUAL void SetSoundChannelsPlayingLimit( int soundChannelsPlayingLimit );
	VIRTUAL void SetSoundMemLimit( int SoundMemLimit );
	VIRTUAL void SetSoundCPULimit( int SoundCPULimit );
	VIRTUAL void SetNumDrawCallsLimit( int numDrawCallsLimit );
	VIRTUAL void SetStreamingThroughputLimit( float streamingThroughputLimit );
	VIRTUAL void SetBudget( int sysMemLimitInMB, int videoMemLimitInMB, 
		float frameTimeLimitInMS, int soundChannelsPlayingLimit, int SoundMemLimitInMB, int SoundCPULimit, int numDrawCallsLimit );

	VIRTUAL int GetSysMemLimit() const;
	VIRTUAL int GetVideoMemLimit() const;
	VIRTUAL float GetFrameTimeLimit() const;
	VIRTUAL int GetSoundChannelsPlayingLimit() const;
	VIRTUAL int GetSoundMemLimit() const;
	VIRTUAL int GetSoundCPULimit() const;
	VIRTUAL int GetNumDrawCallsLimit() const;
	VIRTUAL float GetStreamingThroughputLimit() const;
	VIRTUAL void GetBudget( int& sysMemLimitInMB, int& videoMemLimitInMB, 
		float& frameTimeLimitInMS, int& soundChannelsPlayingLimit, int& SoundMemLimitInMB, int& SoundCPULimitInPercent, int& numDrawCallsLimit ) const;

	VIRTUAL void MonitorBudget();
	VIRTUAL void Render(float x, float y);

	VIRTUAL void GetXbox360MemoryEstimation( int &systemMemInMB,int &videoMemTextureInMB,int &videoMemMeshInMB );

	virtual void Release();

protected:
	virtual ~CBudgetingSystem();
	
	void DrawText( float& x, float& y, float* pColor, const char * format, ... ) PRINTF_PARAMS(5, 6);
	
	void MonitorSystemMemory( float& x, float& y );
	void MonitorVideoMemory( float& x, float& y );
	void MonitorFrameTime( float& x, float& y );
	void MonitorSoundChannels( float& x, float& y );
	void MonitorSoundMemory( float& x, float& y );
	void MonitorSoundCPU( float& x, float& y );
	void MonitorDrawCalls( float& x, float& y );
	void MonitorPolyCount( float& x, float& y );
	void MonitorStreaming( float& x, float& y );

	void GetColor( float scale, float* pColor );
	void DrawMeter( float& x, float& y, float scale );

	void RegisterWithPerfHUD();

//static perfhud render callback
	static void PerfHudRender(float x, float y);

protected:
	IRenderer* m_pRenderer;
	IRenderAuxGeom* m_pAuxRenderer;
	ITimer* m_pTimer;
	ISoundSystem* m_pISoundSystem; 
	IStreamEngine* m_pStreamEngine;

	int m_sysMemLimitInMB;
	int m_videoMemLimitInMB;
	float m_frameTimeLimitInMS;
	int m_soundChannelsPlayingLimit;
	int m_soundMemLimitInMB;
	int m_soundCPULimitInPercent;
	int m_numDrawCallsLimit;
	int m_numPolysLimit;
	float m_streamingThroughputLimit;
	int m_width;
	int m_height;
	int m_enableXenonEstimation;

};


#endif // #ifndef _C_BUDGETING_SYSTEM_
