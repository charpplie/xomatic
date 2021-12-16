#include "stdafx.h"
#include "LiveCreateScene.h"

/*

  CLiveCreateScene

*/

CLiveCreateScene::CLiveCreateScene()
{
	m_pEntity = NULL;
}

CLiveCreateScene::~CLiveCreateScene()
{
	while (!m_actors.empty())
	{
		m_actors.back()->Release();
		m_actors.pop_back();
	}
}

//

CLiveCreateActor* CLiveCreateScene::GetActor(const char* name)
{
	size_t actorCount = m_actors.size();
	for (size_t i=0; i<actorCount; ++i)
	{
		if (::stricmp(m_actors[i]->GetName(), name) == 0)
			return m_actors[i];
	}

	return NULL;
}

void CLiveCreateScene::DeleteActors()
{
	size_t actorCount = m_actors.size();
	for (size_t i=0; i<actorCount; ++i)
		m_actors[i]->Release();
	m_actors.clear();
}

void CLiveCreateScene::AddListener(ILiveCreateSceneListener* pListener)
{
	size_t listenerCount = m_listeners.size();
	for (size_t i=0; i<listenerCount; ++i)
	{
		if (m_listeners[i] == pListener)
			return;
	}

	m_listeners.push_back(pListener);
}

void CLiveCreateScene::RemoveListener(ILiveCreateSceneListener* pListener)
{
	size_t listenerCount = m_listeners.size();
	for (size_t i=0; i<listenerCount; ++i)
	{
		if (m_listeners[i] != pListener)
			continue;

		m_listeners[i] = m_listeners.back();
		m_listeners.pop_back();
		return;
	}
}

void CLiveCreateScene::Reset()
{
	m_pEntity = NULL;

	size_t actorCount = m_actors.size();
	for (size_t i=0; i<actorCount; ++i)
		m_actors[i]->Reset();
}

void CLiveCreateScene::Update()
{
	if (!m_pEntity)
		return;

	QuatT origin(IDENTITY);
	origin.t = m_pEntity->GetPos();
	origin.q = m_pEntity->GetRotation();

	size_t actorCount = m_actors.size();
	for (size_t i=0; i<actorCount; ++i)
		m_actors[i]->Update(origin);
}

// ILCScene

ILCActor* CLiveCreateScene::CreateActor(LCString name, const LCSet* pSet)
{
	CLiveCreateActor* pActor = GetActor(name);
	if (pActor)
		return pActor;

	pActor = CLiveCreateActor::Create(name, pSet);
	if (!pActor)
		return NULL;

	m_actors.push_back(pActor);

	size_t listenerCount = m_listeners.size();
	for (size_t i=0; i<listenerCount; ++i)
		m_listeners[i]->OnCreateActor(*this, *pActor);

	return pActor;
}
