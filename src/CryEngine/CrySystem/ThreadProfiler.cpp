////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   frameprofilerender.cpp
//  Version:     v1.00
//  Created:     24/6/2003 by Timur.
//  Description: Rendering of ThreadProfile.
// -------------------------------------------------------------------------
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "ThreadProfiler.h"
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IConsole.h>
#include <CryThread.h>
#include <ITimer.h>

#include <IThreadTask.h>

#ifdef THREAD_SAMPLER








#include "ThreadSampler.h"
#include "ThreadSampler.cpp"

typedef TThreadSampler::TSpanList    SamplerSpans;
typedef TThreadSampler::SnapshotInfo SamplerSnapshotInfo;



namespace
{
	int profile_threads = 0;
}

//////////////////////////////////////////////////////////////////////////
class CSomeTask : public IThreadTask
{
public:
	virtual void OnUpdate()
	{
		float x = 0;
		while (true)
		{
			for (int i = 0; i < 1000000;i++)
				x++;
			//Sleep(20);
		}
	}
	virtual void Stop() {}

	virtual SThreadTaskInfo* GetTaskInfo() { return &m_TaskInfo; }
protected:
	SThreadTaskInfo m_TaskInfo;
};

CThreadProfiler::CThreadProfiler()
{
	m_bStarted = false;
	m_pSampler = 0;

	// Register console var.
	REGISTER_CVAR(profile_threads,0,0,
		"Enables Threads Profiler (should be deactivated for final product)\n"
		"The C++ function CryThreadSetName(threadid,\"Name\") needs to be called on thread creation.\n"
		"o=off, 1=only active thready, 2+=show all threads\n"
		"Threads profiling may not work on all combinations of OS and CPUs (does not not on Win64)\n"
		"Usage: profile_threads [0/1/2+]");

	/*
	SThreadTaskParams params;
	params.name = "SomeTask";
	params.nFlags = THREAD_TASK_BLOCKING;
	params.nPreferedThread = 1;
	GetISystem()->GetIThreadTaskManager()->RegisterTask( new CSomeTask,params );
	params.name = "SomeTask2";
	params.nPreferedThread = 1;
	GetISystem()->GetIThreadTaskManager()->RegisterTask( new CSomeTask,params );
	*/
}

CThreadProfiler::~CThreadProfiler()
{
	SAFE_DELETE(m_pSampler);
}

//////////////////////////////////////////////////////////////////////////
struct SThreadProfilerRenderInfo
{
	const char *sThreadName;
	
	ColorB clrLine;
	ColorB clrThreadName;

	float fTextSize;
	float fLeftGraphOffset;
	float fLineHeight;
	float width;

	float y;

	SamplerSpans *pSpans;
	int threadTime;
};

//////////////////////////////////////////////////////////////////////////
static void RenderThreadSpan( SThreadProfilerRenderInfo &ri )
{
	IRenderAuxGeom *pAux = gEnv->pRenderer->GetIRenderAuxGeom();

	SamplerSpans &spans = *ri.pSpans;

	float half = 7;

	float fColorThreadName[4] = { ri.clrThreadName.r/255.0f,ri.clrThreadName.g/255.0f,ri.clrThreadName.b/255.0f,1.f};
	gEnv->pRenderer->Draw2dLabel( 1,ri.y,ri.fTextSize,fColorThreadName,false,"%s, %d(ms)",ri.sThreadName,ri.threadTime );

	float ly = ri.y + ri.fLineHeight/2.0f;
	pAux->DrawLine( Vec3(ri.fLeftGraphOffset,ly,0),ri.clrLine,Vec3(ri.width,ly,0),ri.clrLine );
	pAux->DrawLine( Vec3(ri.fLeftGraphOffset,ly-half,0),ri.clrLine,Vec3(ri.fLeftGraphOffset,ly+half,0),ri.clrLine );
	
	for (int i = 0; i < (int)spans.size(); i++)
	{
		float start = ri.fLeftGraphOffset + spans[i].start;
		float end = ri.fLeftGraphOffset + spans[i].end;
		Vec3 quad[4];
		quad[0] = Vec3(start,ly-half,1);
		quad[1] = Vec3(end,  ly-half,1);
		quad[2] = Vec3(end,  ly+half,1);
		quad[3] = Vec3(start,ly+half,1);

		//pAux->DrawLine( Vec3(spans[i].start,ly+1,0),clr,Vec3(spans[i].end,ly+1,0),clr );
		pAux->DrawTriangle( quad[0],ri.clrLine,quad[2],ri.clrLine,quad[1],ri.clrLine );
		pAux->DrawTriangle( quad[0],ri.clrLine,quad[3],ri.clrLine,quad[2],ri.clrLine );
	}
}


//////////////////////////////////////////////////////////////////////////
void CThreadProfiler::Render()
{
	if (profile_threads != 0 && !m_bStarted)
	{
		Start();
		return;
	}
	if (profile_threads == 0 && m_bStarted)
	{
		Stop();
		return;
	}

	if (!m_bStarted)
		return;
	if (!m_pSampler)
		return;

	int width = gEnv->pRenderer->GetWidth();
	int height = gEnv->pRenderer->GetHeight();
	gEnv->pRenderer->Set2DMode(true,width,height);

	width -= 20; // End of thread graph.

	IRenderAuxGeom *pAux = gEnv->pRenderer->GetIRenderAuxGeom();

	float x1 = 0;
	float y1 = 0;
	float x2 = 100;
	float y2 = 100;

	float graph_offset = 150;

	ColorB col(255,0,255,255);

	float fTextSize = 1.1f;
	float colHeader[4] = { 0,1,1,1 };
	float colThreadName[4] = { 1,1,0,1 };
	float colInfo1[4] = { 0,1,1,1 };

	float base_y = 0;
	float szy = 16;

	uint32 processId = -1;

#ifdef WIN32
	processId = GetCurrentProcessId();
#endif

	SamplerSpans spans;
	SamplerSnapshotInfo snapshotInfo;

	spans.resize(0);

	base_y = height - (m_nDisplayedThreads)*szy - 5;

	snapshotInfo.nProcessorCount = 0;
	if (m_pSampler->MakeSnapshot(snapshotInfo)==true)
	{
		//gEnv->pRenderer->Draw2dLabel( 1,base_y,fTextSize,colInfo1,false,"Context Switches per sec:%d",contextSwitches );
		//base_y += szy*2;
	}

	int nContextSwitchesTotal = 0;
	for (int cpu = 0; cpu < snapshotInfo.nProcessorCount; cpu++ )
	{
		nContextSwitchesTotal += snapshotInfo.pContextSwitches[cpu];
	}
	gEnv->pRenderer->Draw2dLabel( 1,base_y-szy,fTextSize,colHeader,false,"Context Switches: %d",nContextSwitchesTotal );
	for (int cpu = 0; cpu < snapshotInfo.nProcessorCount; cpu++ )
	{
		gEnv->pRenderer->Draw2dLabel( (float)(200+cpu*100),(float)(base_y-szy),fTextSize,colHeader,false,"Core%d: %d",cpu,snapshotInfo.pContextSwitches[cpu] );
	}

	char str[128];

	float timeNow = gEnv->pTimer->GetAsyncCurTime();

	int numThreads = (int)m_pSampler->threads.size();

	SThreadProfilerRenderInfo ri;
	ri.fTextSize = fTextSize;
	ri.fLineHeight = szy;
	ri.sThreadName = "";
	ri.width = (float)width;
	ri.y = 0;
	ri.clrLine = ColorB(255,255,0,255);
	ri.clrThreadName = ColorB(255,255,0,255);
	ri.fLeftGraphOffset = graph_offset;
	ri.pSpans = &spans;

	int nShowThreads = numThreads;
#ifdef WIN32
	nShowThreads += 1;
#endif

	m_nDisplayedThreads = 0;
	for (int ti = 0; ti < nShowThreads; ti++)
	{
		uint32 threadId = -1;
		if (ti < numThreads)
		{
			threadId = m_pSampler->threads[ti];
		}

		uint32 totalTime = 0;
		int processor = 0;
		uint32 span_color = 0;
		spans.resize(0);
		m_pSampler->CreateSpanListForThread( processId,threadId, spans, (uint32)(width-graph_offset), 2, &totalTime,&processor,&span_color );

		if ((!spans.empty() && totalTime >= 1) && ti < numThreads)
		{
			m_lastActive[ti] = gEnv->pTimer->GetAsyncCurTime();
		}

		if (span_color != 0)
		{
			ri.clrLine = span_color;
			ri.clrThreadName = span_color;
		}
		else
		{
			ri.clrLine = ColorB(255,255,0,255);
			ri.clrThreadName = ColorB(255,255,0,255);
		}

		ri.y = base_y + m_nDisplayedThreads*szy;
		ri.threadTime = totalTime;

		if (threadId != -1)
		{
			// Hide threads that are not active for 10 seconds.
			if ((timeNow - m_lastActive[ti]) > 10 && profile_threads == 1)
				continue;

			strcpy( str,CryThreadGetName(threadId) );
			if (str[0] == 0)
				sprintf_s( str,"  * %x",threadId );
			ri.sThreadName = str;
		}
		else
		{
			ri.sThreadName = "Others";
			ri.clrLine = ColorB(180,180,180,255);
			ri.clrThreadName = ColorB(180,180,180,255);
		}

		RenderThreadSpan( ri );

		m_nDisplayedThreads++;
	}



































	gEnv->pRenderer->Set2DMode( false,0,0 );
}

void CThreadProfiler::Start()
{
	uint32 nProcessId = 0;



	m_pSampler = new TThreadSampler;
	nProcessId = GetCurrentProcessId();


	m_bStarted = true;
	m_pSampler->EnumerateThreads( nProcessId );
	m_lastActive.resize(m_pSampler->threads.size());
	m_nDisplayedThreads = (int)m_pSampler->threads.size() + 1;
}

void CThreadProfiler::Stop()
{
	m_bStarted = false;
	SAFE_DELETE(m_pSampler);
}


#endif // THREAD_SAMPLER
