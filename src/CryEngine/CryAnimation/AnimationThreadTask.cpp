#include <stdafx.h>
#include <IThreadTask.h>
#include "AnimationThreadTask.h"

/*
CAnimationThreadTask
*/

void CAnimationThreadTask::Begin()
{
	CAnimationThreadPool::Instance().AddTask(this);
}

// IThreadTask

void CAnimationThreadTask::OnUpdate()
{
	Execute();

	IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
	pThreadTaskManager->UnregisterTask(this);
}

/*
CAnimationThreadPool
*/

CAnimationThreadPool::CAnimationThreadPool() :
	m_hThreadPool(-1)
{
}

//

bool CAnimationThreadPool::Initialize()
{
	// NOTE: We only need MT on 360 for performance reason.
	// The system is being rewritten and it seems to give problems only on PC.
	// So leave it disabled for anything but 360 until the new system will be submitted.
#ifndef XENON
	return true;
#endif

	// Disable MT if Editor.
	if (gEnv->bEditor)
		return true;

	ThreadPoolDesc threadPoolDesc;
	threadPoolDesc.sPoolName = "Animation";

//	if (!threadPoolDesc.CreateThread(BIT(5)))
//		return false;
	if (!threadPoolDesc.CreateThread(BIT(3)))
		return false;

	IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
	assert(pThreadTaskManager);

	m_hThreadPool = pThreadTaskManager->CreateThreadsPool(threadPoolDesc);
	if (m_hThreadPool < 0)
		return false;

	return true;
}

void CAnimationThreadPool::AddTask(CAnimationThreadTask* pTask)
{
	if (!Console::GetInst().ca_thread || m_hThreadPool < 0)
	{
		pTask->Execute();
		return;
	}

	IThreadTaskManager* pThreadTaskManager = gEnv->pSystem->GetIThreadTaskManager();
	assert(pThreadTaskManager);

	pTask->m_threadTaskInfo.m_params.name = "Animation";
	pTask->m_threadTaskInfo.m_params.nFlags = THREAD_TASK_ASSIGN_TO_POOL;
	pTask->m_threadTaskInfo.m_params.nThreadsGroupId = m_hThreadPool;
	pTask->m_threadTaskInfo.m_pThread = NULL;
	pThreadTaskManager->RegisterTask(pTask, pTask->m_threadTaskInfo.m_params);
}
