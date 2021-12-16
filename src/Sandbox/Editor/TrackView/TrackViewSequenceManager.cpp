//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2012.
//
//  Created: 21/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewSequenceManager.h"
#include "TrackViewUndo.h"
#include "Material/MaterialManager.h"
#include "AnimationContext.h"
#include "GameEngine.h"

////////////////////////////////////////////////////////////////////////////
CTrackViewSequenceManager::CTrackViewSequenceManager() : m_nextSequenceId(0)
{
	GetIEditor()->RegisterNotifyListener(this);
	GetIEditor()->GetMaterialManager()->AddListener(this);
	GetIEditor()->GetObjectManager()->AddObjectEventListener(functor(*this, &CTrackViewSequenceManager::OnObjectEvent));
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequenceManager::~CTrackViewSequenceManager()
{
	GetIEditor()->GetObjectManager()->RemoveObjectEventListener(functor(*this, &CTrackViewSequenceManager::OnObjectEvent));
	GetIEditor()->GetMaterialManager()->RemoveListener(this);
	GetIEditor()->UnregisterNotifyListener(this);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
	switch(event)
	{		
	case eNotify_OnCloseScene:
		// Fall through
	case eNotify_OnBeginLoad:
		m_bUnloadingLevel = true;
		break;
	case eNotify_OnEndNewScene:		
		// Fall through
	case eNotify_OnEndSceneOpen:
		// Fall through		
	case eNotify_OnEndLoad:
		// Fall through		
	case eNotify_OnLayerImportEnd:
		m_bUnloadingLevel = false;
		SortSequences();
		break;
	}
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence *CTrackViewSequenceManager::GetSequenceByName(CString name) const
{
	for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
	{
		CTrackViewSequence *pSequence = (*iter).get();

		if (pSequence->GetName() == name)
		{
			return pSequence;
		}
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence *CTrackViewSequenceManager::GetSequenceByAnimSequence(IAnimSequence* pAnimSequence) const
{
	for (auto iter = m_sequences.begin(); iter != m_sequences.end(); ++iter)
	{
		CTrackViewSequence *pSequence = (*iter).get();

		if (pSequence->m_pAnimSequence == pAnimSequence)
		{
			return pSequence;
		}
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewSequence * CTrackViewSequenceManager::GetSequenceByIndex(unsigned int index) const
{
	if (index >= m_sequences.size())
	{
		return nullptr;
	}

	return m_sequences[index].get();
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::CreateSequence(CString name)
{
	CGameEngine* pGameEngine = GetIEditor()->GetGameEngine();
	if (!pGameEngine || !pGameEngine->IsLevelLoaded())
	{
		return;
	}

	CTrackViewSequence *pExistingSequence = GetSequenceByName(name);
	if (pExistingSequence)
	{
		return;
	}

	CUndo undo("Create TrackView Sequence");
	GetIEditor()->GetObjectManager()->NewObject("SequenceObject", 0, name);
}

////////////////////////////////////////////////////////////////////////////
IAnimSequence *CTrackViewSequenceManager::OnCreateSequenceObject(CString name)
{
	CTrackViewSequence *pExistingSequence = GetSequenceByName(name);
	if (pExistingSequence)
	{
		return pExistingSequence->m_pAnimSequence;
	}

	IAnimSequence *pNewCryMovieSequence = GetIEditor()->GetMovieSystem()->CreateSequence(name, ++m_nextSequenceId);
	CTrackViewSequence *pNewSequence = new CTrackViewSequence(pNewCryMovieSequence);
	
	m_sequences.push_back(std::unique_ptr<CTrackViewSequence>(pNewSequence));		
	CUndo::Record(new CUndoSequenceAdd(pNewSequence));

	SortSequences();
	OnSequenceAdded(pNewSequence);

	return pNewCryMovieSequence;
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::DeleteSequence(CTrackViewSequence *pSequence)
{
	const int numSequences = m_sequences.size();
	for (int sequenceIndex = 0; sequenceIndex < numSequences; ++sequenceIndex)
	{
		if (m_sequences[sequenceIndex].get() == pSequence)
		{
			CUndo undo("Delete TrackView Sequence");
			CSequenceObject *pSequenceObject = static_cast<CSequenceObject*>(pSequence->m_pAnimSequence->GetOwner());
			GetIEditor()->GetObjectManager()->DeleteObject(pSequenceObject);
		}
	}
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnDeleteSequenceObject(CString name)
{	
	CTrackViewSequence *pSequence = GetSequenceByName(name);	
	assert(pSequence);
	
	if (pSequence)
	{
		const bool bUndoWasSuspended = GetIEditor()->IsUndoSuspended();

		if (bUndoWasSuspended)
		{
			GetIEditor()->ResumeUndo();
		}
		
		if (m_bUnloadingLevel)
		{
			// While unloading, there is no recording so 
			// only make the undo object destroy the sequence
			std::unique_ptr<CUndoSequenceRemove> sequenceRemove(new CUndoSequenceRemove(pSequence));
		}
		else if (CUndo::IsRecording())
		{
			CUndo::Record(new CUndoSequenceRemove(pSequence));
		}

		if (bUndoWasSuspended)
		{
			GetIEditor()->SuspendUndo();
		}		
	}
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::SortSequences()
{
	std::stable_sort(m_sequences.begin(), m_sequences.end(), 
		[](const std::unique_ptr<CTrackViewSequence> &a, const std::unique_ptr<CTrackViewSequence> &b) -> bool {
			CString aName = a.get()->GetName();
			CString bName = b.get()->GetName();
			return aName < bName;
	});
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnSequenceAdded(CTrackViewSequence *pSequence)
{
	for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnSequenceAdded(pSequence);
	}

	GetIEditor()->GetUndoManager()->AddListener(pSequence);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnSequenceRemoved(CTrackViewSequence *pSequence)
{
	GetIEditor()->GetUndoManager()->RemoveListener(pSequence);

	for (auto iter = m_listeners.begin(); iter != m_listeners.end(); ++iter)
	{
		(*iter)->OnSequenceRemoved(pSequence);
	}		
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnDataBaseItemEvent(IDataBaseItem *pItem, EDataBaseItemEvent event)
{
	const uint numSequences = m_sequences.size();

	for (uint i = 0; i < numSequences; ++i)
	{
		m_sequences[i]->UpdateDynamicParams();
	}
}

////////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewSequenceManager::GetAllRelatedAnimNodes(const CEntityObject *pEntityObject) const
{
	CTrackViewAnimNodeBundle nodeBundle;

	const uint sequenceCount = GetCount();

	for (uint sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
	{
		CTrackViewSequence *pSequence = GetSequenceByIndex(sequenceIndex);
		nodeBundle.AppendAnimNodeBundle(pSequence->GetAllOwnedNodes(pEntityObject));
	}

	return nodeBundle;
}

////////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode *CTrackViewSequenceManager::GetActiveAnimNode(const CEntityObject *pEntityObject) const
{
	CTrackViewAnimNodeBundle nodeBundle = GetAllRelatedAnimNodes(pEntityObject);

	const uint nodeCount = nodeBundle.GetCount();
	for (uint nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex)
	{
		CTrackViewAnimNode *pAnimNode = nodeBundle.GetNode(nodeIndex);
		if (pAnimNode->IsActive())
		{
			return pAnimNode;
		}
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::OnObjectEvent(CBaseObject* pObject, int event)
{
	if (event == CBaseObject::ON_PREATTACHED || event == CBaseObject::ON_PREDETACHED
		|| event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
	{
		HandleAttachmentChange(pObject, event);
	}
	else if (event == CBaseObject::ON_RENAME)
	{
		HandleObjectRename(pObject);
	}
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::HandleAttachmentChange(CBaseObject* pObject, int event)
{
	// If an object gets attached/detached from its parent we need to update all related anim nodes, otherwise
	// they will end up very near the origin or very far away from the attached object when animated	

	if (!pObject->IsKindOf(RUNTIME_CLASS(CEntityObject)))
	{
		return;
	}

	CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
	CTrackViewAnimNodeBundle bundle = GetAllRelatedAnimNodes(pEntityObject);

	const uint numAffectedAnimNodes = bundle.GetCount();
	if (numAffectedAnimNodes == 0)
	{
		return;
	}

	std::unordered_set<CTrackViewSequence*> affectedSequences;
	for (uint i = 0; i < numAffectedAnimNodes; ++i)
	{
		CTrackViewAnimNode* pAnimNode = bundle.GetNode(i);
		affectedSequences.insert(pAnimNode->GetSequence());
	}

	CAnimationContext* pAnimationContext = GetIEditor()->GetAnimation();
	CTrackViewSequence* pActiveSequence = pAnimationContext->GetSequence();
	const float time = pAnimationContext->GetTime();	

	for (auto iter = affectedSequences.begin(); iter != affectedSequences.end(); ++iter)
	{
		CTrackViewSequence* pSequence = *iter;
		pAnimationContext->SetSequence(pSequence, true, true);		

		if (pSequence == pActiveSequence)
		{
			pAnimationContext->SetTime(time);
		}

		for (uint i = 0; i < numAffectedAnimNodes; ++i)
		{
			CTrackViewAnimNode* pNode = bundle.GetNode(i);
			if (pNode->GetSequence() == pSequence)
			{
				if (event == CBaseObject::ON_PREATTACHED || event == CBaseObject::ON_PREDETACHED)
				{
					const Matrix34 transform = pNode->GetNodeEntity()->GetWorldTM();
					m_prevTransforms[pSequence] = transform;
				}
				else if (event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
				{
					const Matrix34 transform = m_prevTransforms[pSequence];
					pNode->GetNodeEntity()->SetWorldTM(transform);
				}				
			}
		}
	}

	if (event == CBaseObject::ON_ATTACHED || event == CBaseObject::ON_DETACHED)
	{
		m_prevTransforms.clear();
	}

	pAnimationContext->SetSequence(pActiveSequence, true, true);
	pAnimationContext->SetTime(time);
}

////////////////////////////////////////////////////////////////////////////
void CTrackViewSequenceManager::HandleObjectRename(CBaseObject* pObject)
{
	if (!pObject->IsKindOf(RUNTIME_CLASS(CEntityObject)))
	{
		return;
	}

	CEntityObject* pEntityObject = static_cast<CEntityObject*>(pObject);
	CTrackViewAnimNodeBundle bundle = GetAllRelatedAnimNodes(pEntityObject);

	const uint numAffectedAnimNodes = bundle.GetCount();
	for (uint i = 0; i < numAffectedAnimNodes; ++i)
	{
		CTrackViewAnimNode* pAnimNode = bundle.GetNode(i);
		pAnimNode->SetName(pObject->GetName());
	}

	GetIEditor()->Notify(eNotify_OnReloadTrackView);
}
