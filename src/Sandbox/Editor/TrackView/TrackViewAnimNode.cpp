//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2012.
//
//  Created: 6/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "TrackViewAnimNode.h"
#include "TrackViewTrack.h"
#include "TrackViewSequence.h"
#include "TrackViewUndo.h"
#include "TrackViewNodeFactories.h"
#include "AnimationContext.h"
#include "CommentNodeAnimator.h"
#include "LayerNodeAnimator.h"
#include "DirectorNodeAnimator.h"
#include "Objects/EntityObject.h"
#include "Objects/CameraObject.h"
#include "Objects/SequenceObject.h"
#include "Objects/ObjectLayerManager.h"
#include "Objects/MiscEntities.h"
#include "Objects/GizmoManager.h"
#include "Objects/ObjectManager.h"
#include "ViewManager.h"
#include "RenderViewport.h"
#include "Clipboard.h"

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::AppendAnimNode(CTrackViewAnimNode *pNode)
{
	stl::push_back_unique(m_animNodes, pNode);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::AppendAnimNodeBundle(const CTrackViewAnimNodeBundle &bundle)
{
	for (auto iter = bundle.m_animNodes.begin(); iter != bundle.m_animNodes.end(); ++iter)
	{
		AppendAnimNode(*iter);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::ExpandAll(bool bAlsoExpandParentNodes)
{
	std::set<CTrackViewNode*> nodesToExpand;
	std::copy(m_animNodes.begin(), m_animNodes.end(), std::inserter(nodesToExpand, nodesToExpand.end()));

	if (bAlsoExpandParentNodes)
	{
		for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
		{
			CTrackViewNode *pNode = *iter;

			for (CTrackViewNode *pParent = pNode->GetParentNode(); pParent; pParent = pParent->GetParentNode())
			{
				nodesToExpand.insert(pParent);
			}
		}		
	}

	for (auto iter = nodesToExpand.begin(); iter != nodesToExpand.end(); ++iter)
	{
		(*iter)->SetExpanded(true);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::CollapseAll()
{
	for (auto iter = m_animNodes.begin(); iter != m_animNodes.end(); ++iter)
	{
		(*iter)->SetExpanded(false);
	}
}

//////////////////////////////////////////////////////////////////////////
const bool CTrackViewAnimNodeBundle::DoesContain(const CTrackViewNode *pTargetNode)
{
	return stl::find(m_animNodes, pTargetNode);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNodeBundle::Clear()
{
	m_animNodes.clear();
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode::CTrackViewAnimNode(IAnimSequence *pSequence, IAnimNode *pAnimNode, CTrackViewNode *pParentNode)
	: CTrackViewNode(pParentNode), m_pAnimSequence(pSequence), m_pAnimNode(pAnimNode), m_pNodeEntity(nullptr),
	m_pNodeAnimator(nullptr), m_trackGizmo(nullptr)
{
	if (pAnimNode)
	{
		// Search for child nodes
		const int nodeCount = pSequence->GetNodeCount();
		for (int i = 0; i < nodeCount; ++i)
		{
			IAnimNode *pNode = pSequence->GetNode(i);
			IAnimNode *pParentNode = pNode->GetParent();

			// If our node is the parent, then the current node is a child of it
			if (pAnimNode == pParentNode)
			{
				CTrackViewAnimNodeFactory animNodeFactory;
				CTrackViewAnimNode *pNewTVAnimNode = animNodeFactory.BuildAnimNode(pSequence, pNode, this);
				m_childNodes.push_back(std::unique_ptr<CTrackViewNode>(pNewTVAnimNode));
			}
		}

		// Search for tracks
		const int trackCount = pAnimNode->GetTrackCount();
		for (int i = 0; i < trackCount; ++i)
		{
			IAnimTrack *pTrack = pAnimNode->GetTrackByIndex(i);

			CTrackViewTrackFactory trackFactory;
			CTrackViewTrack *pNewTVTrack = trackFactory.BuildTrack(pTrack, this, this);
			m_childNodes.push_back(std::unique_ptr<CTrackViewNode>(pNewTVTrack));
		}

		// Set owner to update entity CryMovie entity IDs and remove it again
		SetNodeEntity(GetNodeEntity());		
		m_pAnimNode->SetNodeOwner(nullptr);
	}

	SortNodes();

	m_bExpanded = IsGroupNode();	

	switch(GetType())
	{
	case eAnimNodeType_Comment:
		m_pNodeAnimator.reset(new CCommentNodeAnimator(this));
		break;
	case eAnimNodeType_Layer:
		m_pNodeAnimator.reset(new CLayerNodeAnimator());
		break;
	case eAnimNodeType_Director:
		m_pNodeAnimator.reset(new CDirectorNodeAnimator(this));
		break;
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::BindToEditorObjects()
{
	if (!IsActive())
	{
		return;
	}

	CTrackViewSequenceNotificationContext context(GetSequence());

	CTrackViewAnimNode *pDirector = GetDirector();
	const bool bBelongsToActiveDirector = pDirector ? pDirector->IsActiveDirector() : true;

	if(bBelongsToActiveDirector)
	{
		IObjectManager *pObjectManager = GetIEditor()->GetObjectManager();
		CEntityObject *pEntity = (CEntityObject*)pObjectManager->FindAnimNodeOwner(this);

		if (m_pNodeAnimator)
		{
			m_pNodeAnimator->Bind(this);
		}

		if (pEntity)
		{
			pEntity->SetTransformDelegate(this);
			pEntity->RegisterListener(this);
			SetNodeEntity(pEntity);			
		}

		for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
		{
			CTrackViewNode *pChildNode = (*iter).get();
			if (pChildNode->GetNodeType() == eTVNT_AnimNode)
			{
				CTrackViewAnimNode *pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
				pChildAnimNode->BindToEditorObjects();
			}
		}
	}

	UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UnBindFromEditorObjects()
{
	CTrackViewSequenceNotificationContext context(GetSequence());

	IObjectManager *pObjectManager = GetIEditor()->GetObjectManager();
	CEntityObject *pEntity = (CEntityObject*)pObjectManager->FindAnimNodeOwner(this);
	
	if (m_pAnimNode)
	{
		m_pAnimNode->SetNodeOwner(nullptr);
	}	

	if (pEntity)
	{
		pEntity->SetTransformDelegate(nullptr);		
		pEntity->UnregisterListener(this);
	}

	if (m_pNodeAnimator)
	{
		m_pNodeAnimator->UnBind(this);
	}

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
			pChildAnimNode->UnBindFromEditorObjects();
		}
	}

	GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_trackGizmo);
	m_trackGizmo = nullptr;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsBoundToEditorObjects() const
{
	return m_pAnimNode ? (m_pAnimNode->GetNodeOwner() != NULL) : false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SyncToConsole(SAnimContext &animContext)
{
	switch(GetType())
	{
	case eAnimNodeType_Camera:
		{
			IEntity *pEntity = GetEntity();
			if(pEntity)
			{
				CBaseObject *pCameraObject = GetIEditor()->GetObjectManager()->FindObject(GetIEditor()->GetViewManager()->GetCameraObjectId());
				IEntity *pCameraEntity = pCameraObject ? ((CEntityObject*)pCameraObject)->GetIEntity() : NULL;
				if(pCameraEntity && pEntity->GetId() == pCameraEntity->GetId())
					// If this camera is currently active,
				{
					Matrix34 viewTM = pEntity->GetWorldTM();
					Vec3 oPosition(viewTM.GetTranslation());
					Vec3 oDirection(viewTM.TransformVector(FORWARD_DIRECTION));
				}
			}
		}
		break;
	}

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = (CTrackViewAnimNode*)pChildNode;
			pChildAnimNode->SyncToConsole(animContext);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNode *CTrackViewAnimNode::CreateSubNode(const CString &name, const EAnimNodeType animNodeType, CEntityObject *pOwner)
{
	assert(CUndo::IsRecording());

	const bool bIsGroupNode = IsGroupNode();
	assert(bIsGroupNode);
	if (!bIsGroupNode)
	{
		return nullptr;
	}

	// Check if the node's director or sequence already contains a node with this name
	CTrackViewAnimNode *pDirector = (GetType() == eAnimNodeType_Director) ? this : GetDirector();	
	pDirector = pDirector ? pDirector : GetSequence();
	if (pDirector->GetAnimNodesByName(name).GetCount() > 0)
	{
		return nullptr;
	}

	// Create CryMovie and TrackView node
	IAnimNode *pNewAnimNode = m_pAnimSequence->CreateNode(animNodeType);
	if (!pNewAnimNode)
	{
		return nullptr;
	}

	pNewAnimNode->SetName(name);
	pNewAnimNode->CreateDefaultTracks();
	pNewAnimNode->SetParent(m_pAnimNode);

	CTrackViewAnimNodeFactory animNodeFactory;
	CTrackViewAnimNode *pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);
	pNewNode->m_bExpanded = true;

	// Make sure that camera and entity nodes get created with an owner
	assert((animNodeType != eAnimNodeType_Camera && animNodeType != eAnimNodeType_Entity) || pOwner);

	pNewNode->SetNodeEntity(pOwner);
	pNewAnimNode->SetNodeOwner(pNewNode);

	pNewNode->BindToEditorObjects();

	AddNode(pNewNode);	
	CUndo::Record(new CUndoAnimNodeAdd(pNewNode));		

	return pNewNode;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::RemoveSubNode(CTrackViewAnimNode *pSubNode)
{
	assert(CUndo::IsRecording());

	const bool bIsGroupNode = IsGroupNode();
	assert(bIsGroupNode);
	if (!bIsGroupNode)
	{
		return;
	}

	CUndo::Record(new CUndoAnimNodeRemove(pSubNode));
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack *CTrackViewAnimNode::CreateTrack(const CAnimParamType &paramType)
{
	assert(CUndo::IsRecording());

	if (GetTrackForParameter(paramType) && !(GetParamFlags(paramType) & IAnimNode::eSupportedParamFlags_MultipleTracks))
	{
		return nullptr;
	}
	
	// Create CryMovie and TrackView track
	IAnimTrack *pNewAnimTrack = m_pAnimNode->CreateTrack(paramType);
	if (!pNewAnimTrack)
	{
		return nullptr;
	}

	CTrackViewTrackFactory trackFactory;
	CTrackViewTrack *pNewTrack = trackFactory.BuildTrack(pNewAnimTrack, this, this);

	AddNode(pNewTrack);	
	CUndo::Record(new CUndoTrackAdd(pNewTrack));	

	SetPosRotScaleTracksDefaultValues();

	return pNewTrack;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::RemoveTrack(CTrackViewTrack *pTrack)
{
	assert(CUndo::IsRecording());
	assert(!pTrack->IsSubTrack());
	
	if (!pTrack->IsSubTrack())
	{
		CUndo::Record(new CUndoTrackRemove(pTrack));
	}	
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SnapTimeToPrevKey(float &time) const
{
	const float startTime = time;
	float closestTrackTime = std::numeric_limits<float>::min();
	bool bFoundPrevKey = false;	

	for (size_t i = 0; i < m_childNodes.size(); ++i)
	{
		const CTrackViewNode *pNode = m_childNodes[i].get();

		float closestNodeTime = startTime;
		if (pNode->SnapTimeToPrevKey(closestNodeTime))
		{
			closestTrackTime = std::max(closestNodeTime, closestTrackTime);
			bFoundPrevKey = true;
		}		
	}

	if (bFoundPrevKey)
	{
		time = closestTrackTime;
	}	

	return bFoundPrevKey;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SnapTimeToNextKey(float &time) const
{
	const float startTime = time;
	float closestTrackTime = std::numeric_limits<float>::max();
	bool bFoundNextKey = false;	

	for (size_t i = 0; i < m_childNodes.size(); ++i)
	{
		const CTrackViewNode *pNode = m_childNodes[i].get();

		float closestNodeTime = startTime;
		if (pNode->SnapTimeToNextKey(closestNodeTime))
		{
			closestTrackTime = std::min(closestNodeTime, closestTrackTime);
			bFoundNextKey = true;
		}		
	}

	if (bFoundNextKey)
	{
		time = closestTrackTime;
	}	

	return bFoundNextKey;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetSelectedKeys()
{
	CTrackViewKeyBundle bundle;

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		bundle.AppendKeyBundle((*iter)->GetSelectedKeys());
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetAllKeys()
{
	CTrackViewKeyBundle bundle;

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		bundle.AppendKeyBundle((*iter)->GetAllKeys());
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewKeyBundle CTrackViewAnimNode::GetKeysInTimeRange(const float t0, const float t1)
{
	CTrackViewKeyBundle bundle;

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		bundle.AppendKeyBundle((*iter)->GetKeysInTimeRange(t0, t1));
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetAllTracks()
{
	return GetTracks(false, CAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetSelectedTracks()
{
	return GetTracks(true, CAnimParamType());
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetTracksByParam(const CAnimParamType &paramType)
{
	return GetTracks(false, paramType);
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrackBundle CTrackViewAnimNode::GetTracks(const bool bOnlySelected, const CAnimParamType &paramType)
{
	CTrackViewTrackBundle bundle;

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pNode = (*iter).get();
		
		if (pNode->GetNodeType() == eTVNT_Track)
		{
			CTrackViewTrack *pTrack = static_cast<CTrackViewTrack*>(pNode);

			if (paramType != eAnimParamType_Invalid && pTrack->GetParameterType() != paramType)
			{
				continue;
			}

			if (!bOnlySelected || pTrack->IsSelected())
			{
				bundle.AppendTrack(pTrack);
			}			

			const unsigned int subTrackCount = pTrack->GetChildCount();
			for (unsigned int subTrackIndex = 0; subTrackIndex < subTrackCount; ++subTrackIndex)
			{
				CTrackViewTrack *pSubTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(subTrackIndex));
				if (!bOnlySelected || pSubTrack->IsSelected())
				{
					bundle.AppendTrack(pSubTrack);
				}				
			}
		}
		else if (pNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pAnimNode = static_cast<CTrackViewAnimNode*>(pNode);
			bundle.AppendTrackBundle(pAnimNode->GetTracks(bOnlySelected, paramType));
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
EAnimNodeType CTrackViewAnimNode::GetType() const
{
	return m_pAnimNode ? m_pAnimNode->GetType() : eAnimNodeType_Invalid;
}

//////////////////////////////////////////////////////////////////////////
EAnimNodeFlags CTrackViewAnimNode::GetFlags() const
{
	return m_pAnimNode ? (EAnimNodeFlags)m_pAnimNode->GetFlags() : (EAnimNodeFlags)0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetAsActiveDirector()
{
	if (GetType() == eAnimNodeType_Director)
	{
		m_pAnimSequence->SetActiveDirector(m_pAnimNode);

		GetSequence()->UnBindFromEditorObjects();
		GetSequence()->BindToEditorObjects();

		GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_SetAsActiveDirector);
	}		
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsActiveDirector() const
{
	return m_pAnimNode == m_pAnimSequence->GetActiveDirector();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsParamValid(const CAnimParamType &param) const
{
	return m_pAnimNode ? m_pAnimNode->IsParamValid(param) : false;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewTrack *CTrackViewAnimNode::GetTrackForParameter(const CAnimParamType &paramType, uint32 index) const
{
	uint32 currentIndex = 0;

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pNode = (*iter).get();

		if (pNode->GetNodeType() == eTVNT_Track)
		{
			CTrackViewTrack *pTrack = static_cast<CTrackViewTrack*>(pNode);
			
			if (pTrack->GetParameterType() == paramType)
			{
				if(currentIndex == index)
				{
					return pTrack;
				}
				else
				{
					++currentIndex;
				}
			}

			if (pTrack->IsCompoundTrack())
			{
				unsigned int numChildTracks = pTrack->GetChildCount();
				for (unsigned int i = 0; i < numChildTracks; ++i)
				{
					CTrackViewTrack *pChildTrack = static_cast<CTrackViewTrack*>(pTrack->GetChild(i));
					if (pChildTrack->GetParameterType() == paramType)
					{
						if(currentIndex == index)
						{
							return pChildTrack;
						}
						else
						{
							++currentIndex;
						}
					}
				}
			}
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::Render(const SAnimContext &ac)
{
	if (m_pNodeAnimator && IsActive())
	{
		m_pNodeAnimator->Render(this, ac);
	}

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
			pChildAnimNode->Render(ac);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::Animate(const SAnimContext &animContext)
{
	if (m_pNodeAnimator && IsActive())
	{
		m_pNodeAnimator->Animate(this, animContext);
	}

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
			pChildAnimNode->Animate(animContext);
		}
	}

	UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::SetName(const char *pName)
{
	// Check if the node's director already contains a node with this name
	CTrackViewAnimNode *pDirector = GetDirector();	
	pDirector = pDirector ? pDirector : GetSequence();

	CTrackViewAnimNodeBundle nodes = pDirector->GetAnimNodesByName(pName);
	const uint numNodes = nodes.GetCount();
	for (uint i = 0; i < numNodes; ++i)
	{
		if (nodes.GetNode(i) != this)
		{
			return false;
		}
	}

	string oldName = GetName();
	m_pAnimNode->SetName(pName);
	
	if (CUndo::IsRecording())
	{
		CUndo::Record(new CUndoAnimNodeRename(this, oldName));
	}
	
	GetSequence()->OnNodeRenamed(this, oldName);

	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::CanBeRenamed() const
{
	return (GetFlags() & eAnimNodeFlags_CanChangeName) != 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetNodeEntity(CEntityObject *pEntity)
{
	m_pNodeEntity = pEntity;

	if (m_pAnimNode)
	{
		m_pAnimNode->SetNodeOwner(this);	
	}	

	if (pEntity)
	{		
		const EntityGUID guid = ToEntityGuid(pEntity->GetId());
		SetEntityGuid(guid);

		if(pEntity->GetLookAt() && pEntity->GetLookAt()->IsKindOf(RUNTIME_CLASS(CEntityObject)))
		{
			CEntityObject *target = static_cast<CEntityObject*>(pEntity->GetLookAt());
			SetEntityGuidTarget(ToEntityGuid(target->GetId()));
		}
		if(pEntity->GetLookAtSource() && pEntity->GetLookAtSource()->IsKindOf(RUNTIME_CLASS(CEntityObject)))
		{
			CEntityObject *source = static_cast<CEntityObject*>(pEntity->GetLookAtSource());
			SetEntityGuidSource(ToEntityGuid(source->GetId()));
		}

		SetPosRotScaleTracksDefaultValues();

		OnSelectionChanged(pEntity->IsSelected());
	}

	GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_NodeOwnerChanged);
}

//////////////////////////////////////////////////////////////////////////
CEntityObject *CTrackViewAnimNode::GetNodeEntity(const bool bSearch)
{
	if (m_pAnimNode)
	{		
		if (m_pNodeEntity)
		{
			return m_pNodeEntity;
		}

		if (bSearch)
		{
			// Search with object manager
			EntityGUID *pGuid = GetEntityGuid();
			return static_cast<CEntityObject*>(GetIEditor()->GetObjectManager()->FindAnimNodeOwner(this));
		}
	}

	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllAnimNodes()
{
	CTrackViewAnimNodeBundle bundle;

	if (GetNodeType() == eTVNT_AnimNode)
	{
		bundle.AppendAnimNode(this);
	}	
	
	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			
			bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllAnimNodes());
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetSelectedAnimNodes()
{
	CTrackViewAnimNodeBundle bundle;

	if ((GetNodeType() == eTVNT_AnimNode || GetNodeType() == eTVNT_Sequence) && IsSelected())
	{
		bundle.AppendAnimNode(this);
	}	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			
			bundle.AppendAnimNodeBundle(pChildAnimNode->GetSelectedAnimNodes());
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAllOwnedNodes(const CEntityObject *pOwner)
{
	CTrackViewAnimNodeBundle bundle;

	if (GetNodeType() == eTVNT_AnimNode && GetNodeEntity() == pOwner)
	{
		bundle.AppendAnimNode(this);
	}	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			
			bundle.AppendAnimNodeBundle(pChildAnimNode->GetAllOwnedNodes(pOwner));
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByType(EAnimNodeType animNodeType)
{
	CTrackViewAnimNodeBundle bundle;

	if (GetNodeType() == eTVNT_AnimNode && GetType() == animNodeType)
	{
		bundle.AppendAnimNode(this);
	}	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			
			bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByType(animNodeType));
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::GetAnimNodesByName(const char *pName)
{
	CTrackViewAnimNodeBundle bundle;

	CString nodeName = GetName();
	if (GetNodeType() == eTVNT_AnimNode && stricmp(pName, nodeName) == 0)
	{		
		bundle.AppendAnimNode(this);
	}	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);
			bundle.AppendAnimNodeBundle(pChildAnimNode->GetAnimNodesByName(pName));
		}
	}

	return bundle;
}

//////////////////////////////////////////////////////////////////////////
char *CTrackViewAnimNode::GetParamName(const CAnimParamType &paramType) const
{
	const char *pName = m_pAnimNode->GetParamName(paramType);
	return pName ? pName : "";
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsGroupNode() const
{
	return GetType() == eAnimNodeType_Director || GetType() == eAnimNodeType_Group;
}

//////////////////////////////////////////////////////////////////////////
CString CTrackViewAnimNode::GetAvailableNodeNameStartingWith(const CString &name) const
{
	CString newName = name;
	unsigned int index = 2;

	while (const_cast<CTrackViewAnimNode*>(this)->GetAnimNodesByName(newName).GetCount() > 0) 
	{
		newName.Format("%s%d", name, index);
		++index;
	}

	return newName;
}

//////////////////////////////////////////////////////////////////////////
CTrackViewAnimNodeBundle CTrackViewAnimNode::AddSelectedEntities()
{
	assert(IsGroupNode());
	assert(CUndo::IsRecording());

	CTrackViewAnimNodeBundle addedNodes;

	// Add selected nodes.
	CSelectionGroup *pSelection = GetIEditor()->GetSelection();
	for (int i = 0; i < pSelection->GetCount(); i++)
	{
		CBaseObject *pObject = pSelection->GetObject(i);
		if (!pObject)
		{
			continue;
		}

		// Check if object already assigned to some AnimNode.
		CTrackViewAnimNode *pExistingNode = GetIEditor()->GetSequenceManager()->GetActiveAnimNode(static_cast<const CEntityObject*>(pObject));		
		if (pExistingNode)
		{
			// If it has the same director than the current node, reject it
			if (pExistingNode->GetDirector() == GetDirector())
			{
				continue;
			}
		}

		// Get node type (either entity or camera)
		EAnimNodeType nodeType = eAnimNodeType_Invalid;
		CTrackViewAnimNode *pAnimNode = nullptr;
		if (pObject->IsKindOf(RUNTIME_CLASS(CCameraObject)))
		{
			pAnimNode = CreateSubNode(pObject->GetName(), eAnimNodeType_Camera, static_cast<CEntityObject*>(pObject));
		}
#if defined(USE_GEOM_CACHES)
		else if (pObject->IsKindOf(RUNTIME_CLASS(CGeomCacheEntity)))
		{
			pAnimNode = CreateSubNode(pObject->GetName(), eAnimNodeType_GeomCache, static_cast<CEntityObject*>(pObject));
		}
#endif
		else if (pObject->IsKindOf(RUNTIME_CLASS(CEntityObject)))
		{
			pAnimNode = CreateSubNode(pObject->GetName(), eAnimNodeType_Entity, static_cast<CEntityObject*>(pObject));
		}

		if (pAnimNode)
		{
			addedNodes.AppendAnimNode(pAnimNode);
		}
	}

	return addedNodes;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::AddCurrentLayer()
{
	assert(IsGroupNode());

	CObjectLayerManager *pLayerManager = GetIEditor()->GetObjectManager()->GetLayersManager();
	CObjectLayer *pLayer = pLayerManager->GetCurrentLayer();
	const CString name = pLayer->GetName();

	CreateSubNode(name, eAnimNodeType_Entity);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuidTarget( const EntityGUID& guid )
{
	if (m_pAnimNode) 
	{ 
		m_pAnimNode->SetEntityGuidTarget(guid); 
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuid( const EntityGUID& guid )
{
	if (m_pAnimNode) 
	{ 
		m_pAnimNode->SetEntityGuid(guid); 
	}
}

//////////////////////////////////////////////////////////////////////////
EntityGUID *CTrackViewAnimNode::GetEntityGuid() const
{
	return m_pAnimNode ? m_pAnimNode->GetEntityGuid() : nullptr;
}

//////////////////////////////////////////////////////////////////////////
IEntity *CTrackViewAnimNode::GetEntity() const
{
	return m_pAnimNode ? m_pAnimNode->GetEntity() : nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetEntityGuidSource( const EntityGUID& guid )
{
	if (m_pAnimNode) 
	{ 
		m_pAnimNode->SetEntityGuidSource(guid); 
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetAsViewCamera()
{
	assert (GetType() == eAnimNodeType_Camera);

	if (GetType() == eAnimNodeType_Camera)
	{
		CEntityObject *pCameraEntity = GetNodeEntity();
		CRenderViewport *pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());
		pRenderViewport->SetCameraObject(pCameraEntity);
	}
}

//////////////////////////////////////////////////////////////////////////
unsigned int CTrackViewAnimNode::GetParamCount() const
{
	return m_pAnimNode ? m_pAnimNode->GetParamCount() : 0;
}

//////////////////////////////////////////////////////////////////////////
CAnimParamType CTrackViewAnimNode::GetParamType(unsigned int index) const
{
	unsigned int paramCount = GetParamCount();
	if (!m_pAnimNode || index >= paramCount)
	{
		return eAnimParamType_Invalid;
	}

	return m_pAnimNode->GetParamType(index);
}

//////////////////////////////////////////////////////////////////////////
IAnimNode::ESupportedParamFlags CTrackViewAnimNode::GetParamFlags(const CAnimParamType &paramType) const
{
	if (m_pAnimNode)
	{
		return m_pAnimNode->GetParamFlags(paramType);
	}

	return IAnimNode::ESupportedParamFlags(0);
}

//////////////////////////////////////////////////////////////////////////
EAnimValue CTrackViewAnimNode::GetParamValueType(const CAnimParamType &paramType) const
{
	if (m_pAnimNode)
	{
		return m_pAnimNode->GetParamValueType(paramType);
	}

	return eAnimValue_Unknown;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UpdateDynamicParams()
{
	if (m_pAnimNode)
	{
		m_pAnimNode->UpdateDynamicParams();
	}	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			
			pChildAnimNode->UpdateDynamicParams();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyKeysToClipboard(XmlNodeRef &xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks)
{
	XmlNodeRef childNode = xmlNode->createNode("Node");	
	childNode->setAttr("name", GetName());
	childNode->setAttr("type", GetType());	

	for (auto iter = m_childNodes.begin(); iter != m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		pChildNode->CopyKeysToClipboard(childNode, bOnlySelectedKeys, bOnlyFromSelectedTracks);
	}

	if (childNode->getChildCount() > 0)
	{
		xmlNode->addChild(childNode);
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyNodesToClipboard(const bool bOnlySelected)
{
	XmlNodeRef animNodesRoot = XmlHelpers::CreateXmlNode("CopyAnimNodesRoot");

	CopyNodesToClipboardRec(this, animNodesRoot, bOnlySelected);

	CClipboard clipboard;
	clipboard.Put(animNodesRoot, "Track view entity nodes");
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::CopyNodesToClipboardRec(CTrackViewAnimNode *pCurrentAnimNode, XmlNodeRef &xmlNode, const bool bOnlySelected)
{
	if (!pCurrentAnimNode->IsGroupNode() && (!bOnlySelected || pCurrentAnimNode->IsSelected()))
	{
		XmlNodeRef childXmlNode = xmlNode->newChild("Node");
		pCurrentAnimNode->m_pAnimNode->Serialize(childXmlNode, false, true);
	}

	for (auto iter = pCurrentAnimNode->m_childNodes.begin(); iter != pCurrentAnimNode->m_childNodes.end(); ++iter)
	{
		CTrackViewNode *pChildNode = (*iter).get();
		if (pChildNode->GetNodeType() == eTVNT_AnimNode)
		{
			CTrackViewAnimNode *pChildAnimNode = static_cast<CTrackViewAnimNode*>(pChildNode);			

			// If selected and group node, force copying of children
			const bool bSelectedAndGroupNode = pCurrentAnimNode->IsSelected() && pCurrentAnimNode->IsGroupNode();
			CopyNodesToClipboardRec(pChildAnimNode, xmlNode, !bSelectedAndGroupNode && bOnlySelected);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::PasteNodesFromClipboard()
{
	assert(CUndo::IsRecording());

	CClipboard clipboard;
	if (clipboard.IsEmpty())
	{
		return false;
	}

	XmlNodeRef animNodesRoot = clipboard.Get();
	if (animNodesRoot == NULL || strcmp(animNodesRoot->getTag(), "CopyAnimNodesRoot") != 0)
	{
		return false;
	}

	const bool bLightAnimationSetActive = GetSequence()->GetFlags() & IAnimSequence::eSeqFlags_LightAnimationSet;

	const unsigned int numNodes = animNodesRoot->getChildCount();
	for (int i = 0; i < numNodes; ++i)
	{
		XmlNodeRef xmlNode = animNodesRoot->getChild(i);

		int type;
		if (!xmlNode->getAttr("Type", type))
		{
			continue;
		}		

		if (bLightAnimationSetActive && (EAnimNodeType)type != eAnimNodeType_Light)
		{
			// Ignore non light nodes in light animation set
			continue;
		}

		PasteNodeFromClipboard(xmlNode);
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::PasteNodeFromClipboard(XmlNodeRef xmlNode)
{
	CString name;

	if (!xmlNode->getAttr("Name", name))
	{
		return;
	}		

	const bool bIsGroupNode = IsGroupNode();
	assert(bIsGroupNode);
	if (!bIsGroupNode)
	{
		return;
	}

	// Check if the node's director or sequence already contains a node with this name
	CTrackViewAnimNode *pDirector = GetDirector();	
	pDirector = pDirector ? pDirector : GetSequence();
	if (pDirector->GetAnimNodesByName(name).GetCount() > 0)
	{
		return;
	}

	// Create CryMovie and TrackView node
	IAnimNode *pNewAnimNode = m_pAnimSequence->CreateNode(xmlNode);
	if (!pNewAnimNode)
	{
		return;
	}

	pNewAnimNode->SetParent(m_pAnimNode);

	CTrackViewAnimNodeFactory animNodeFactory;
	CTrackViewAnimNode *pNewNode = animNodeFactory.BuildAnimNode(m_pAnimSequence, pNewAnimNode, this);
	pNewNode->m_bExpanded = true;

	AddNode(pNewNode);
	CUndo::Record(new CUndoAnimNodeAdd(pNewNode));
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsValidReparentingTo(CTrackViewAnimNode *pNewParent)
{	
	if (pNewParent == GetParentNode() || !pNewParent->IsGroupNode())
	{
		return false;
	}

	// Check if the new parent already contains a node with this name
	CTrackViewAnimNodeBundle foundNodes = pNewParent->GetAnimNodesByName(GetName());
	if (foundNodes.GetCount() > 1 || (foundNodes.GetCount() == 1 && foundNodes.GetNode(0) != this))
	{
		return false;
	}

	// Check if another node already owns this entity in the new parent's tree
	CEntityObject *pOwner = GetNodeEntity();
	if (pOwner)
	{
		CTrackViewAnimNodeBundle ownedNodes = pNewParent->GetAllOwnedNodes(pOwner);
		if (ownedNodes.GetCount() > 0 && ownedNodes.GetNode(0) != this)
		{			
			return false;
		}
	}	

	return true;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetNewParent(CTrackViewAnimNode *pNewParent)
{
	if (pNewParent == GetParentNode())
	{
		return;
	}

	assert(CUndo::IsRecording());
	assert(IsValidReparentingTo(pNewParent));	
	
	CUndo::Record(new CUndoAnimNodeReparent(this, pNewParent));	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetDisabled(bool bDisabled)
{
	if (m_pAnimNode)
	{
		if (bDisabled)
		{
			m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() | eAnimNodeFlags_Disabled);	
			GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Disabled);
		}
		else
		{
			m_pAnimNode->SetFlags(m_pAnimNode->GetFlags() & ~eAnimNodeFlags_Disabled);
			GetSequence()->OnNodeChanged(this, ITrackViewSequenceListener::eNodeChangeType_Enabled);
		}	
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsDisabled() const
{
	return m_pAnimNode ? m_pAnimNode->GetFlags() & eAnimNodeFlags_Disabled : false;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetPos(const Vec3& position)
{
	const float time = GetSequence()->GetTime();
	CTrackViewTrack *pTrack = GetTrackForParameter(eAnimParamType_Position);
	CRenderViewport *pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

	if (pTrack)
	{
		if (!GetIEditor()->GetAnimation()->IsRecording())
		{
			// Offset all keys by move amount.
			Vec3 posPrev;
			pTrack->GetValue(time, posPrev);
			Vec3 offset = position - posPrev;
			pTrack->OffsetKeyPosition(offset);

			GetSequence()->OnKeysChanged();
		}
		else if (m_pNodeEntity->IsSelected() || pRenderViewport->GetCameraObject() == m_pNodeEntity)
		{
			CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
			const int flags = m_pAnimNode->GetFlags();
			// Set the selected flag to enable record when unselected camera is moved through viewport
			m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
			m_pAnimNode->SetPos(GetSequence()->GetTime(), position);
			m_pAnimNode->SetFlags(flags);
			GetSequence()->OnKeysChanged();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetScale(const Vec3& scale)
{
	CTrackViewTrack *pTrack = GetTrackForParameter(eAnimParamType_Scale);

	if (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && pTrack)
	{
		CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
		m_pAnimNode->SetScale(GetSequence()->GetTime(), scale);
		GetSequence()->OnKeysChanged();
	}	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetRotation(const Quat &rotation)
{
	CTrackViewTrack *pTrack = GetTrackForParameter(eAnimParamType_Rotation);
	CRenderViewport *pRenderViewport = static_cast<CRenderViewport*>(GetIEditor()->GetViewManager()->GetGameViewport());

	if (GetIEditor()->GetAnimation()->IsRecording() && (m_pNodeEntity->IsSelected() || pRenderViewport->GetCameraObject() == m_pNodeEntity) && pTrack)
	{
		CUndo::Record(new CUndoTrackObject(pTrack, GetSequence()));
		const int flags = m_pAnimNode->GetFlags();
		// Set the selected flag to enable record when unselected camera is moved through viewport
		m_pAnimNode->SetFlags(flags | eAnimNodeFlags_EntitySelected);
		m_pAnimNode->SetRotate(GetSequence()->GetTime(), rotation);
		m_pAnimNode->SetFlags(flags);
		GetSequence()->OnKeysChanged();
	}	
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsActive()
{
	CTrackViewSequence *pSequence = GetSequence();
	const bool bInActiveSequence = pSequence ? GetSequence()->IsBoundToEditorObjects() : false;
	
	CTrackViewAnimNode *pDirector = GetDirector();
	const bool bMemberOfActiveDirector = pDirector ? GetDirector()->IsActiveDirector() : true;

	return bInActiveSequence && bMemberOfActiveDirector;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnSelectionChanged(const bool bSelected)
{
	if (m_pAnimNode)
	{
		const EAnimNodeType animNodeType = GetType();
		assert(animNodeType == eAnimNodeType_Camera || animNodeType == eAnimNodeType_Entity || animNodeType == eAnimNodeType_GeomCache);

		const EAnimNodeFlags flags = (EAnimNodeFlags)m_pAnimNode->GetFlags();
		m_pAnimNode->SetFlags(bSelected ? (flags | eAnimNodeFlags_EntitySelected) : (flags & ~eAnimNodeFlags_EntitySelected));
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetPosRotScaleTracksDefaultValues()
{
	const CEntityObject *pOwner = GetNodeEntity(false);

	if (pOwner && IsBoundToEditorObjects())
	{
		const float time = GetSequence()->GetTime();
		m_pAnimNode->SetPos(time, pOwner->GetPos());
		m_pAnimNode->SetRotate(time, pOwner->GetRotation());
		m_pAnimNode->SetScale(time, pOwner->GetScale());
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::UpdateTrackGizmo()
{
	if (IsActive() && m_pNodeEntity && !m_pNodeEntity->IsHidden())
	{
		if (!m_trackGizmo)
		{
			CTrackGizmo *pTrackGizmo = new CTrackGizmo;
			pTrackGizmo->SetAnimNode(this);
			m_trackGizmo = pTrackGizmo;
			GetIEditor()->GetObjectManager()->GetGizmoManager()->AddGizmo(m_trackGizmo);
		}
	}
	else
	{
		GetIEditor()->GetObjectManager()->GetGizmoManager()->RemoveGizmo(m_trackGizmo);
		m_trackGizmo = nullptr;
	}

	if (m_pNodeEntity && m_trackGizmo)
	{
		const Matrix34 gizmoMatrix = m_pNodeEntity->GetParentAttachPointWorldTM();
		m_trackGizmo->SetMatrix(gizmoMatrix);
	}
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::CheckTrackAnimated(const CAnimParamType &paramType) const
{
	if (!m_pAnimNode)
	{
		return false;
	}

	CTrackViewTrack *pTrack = GetTrackForParameter(paramType);
	return pTrack && pTrack->GetKeyCount() > 0;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeAnimated(IAnimNode *pNode)
{
	if (m_pNodeEntity)
	{		
		m_pNodeEntity->InvalidateTM( 0 );		
	}	
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeVisibilityChanged(IAnimNode *pNode, const bool bHidden)
{
	if (m_pNodeEntity)
	{
		m_pNodeEntity->SetHidden(bHidden);

		// Need to do this to force recreation of gizmos
		bool bUnhideSelected = !m_pNodeEntity->IsHidden() && m_pNodeEntity->IsSelected();
		if (bUnhideSelected)
		{
			GetIEditor()->GetObjectManager()->UnselectObject( m_pNodeEntity );
			GetIEditor()->GetObjectManager()->SelectObject( m_pNodeEntity );
		}

		UpdateTrackGizmo();
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnNodeReset(IAnimNode *pNode)
{
	if ( gEnv->IsEditing() && m_pNodeEntity )
	{
		// If the node has an event track, one should also reload the script when the node is reset.
		CTrackViewTrack *pAnimTrack = GetTrackForParameter( eAnimParamType_Event );
		if ( pAnimTrack && pAnimTrack->GetKeyCount() )
		{
			CEntityScript *script = m_pNodeEntity->GetScript();
			script->Reload();
			m_pNodeEntity->Reload( true );
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::MatrixInvalidated()
{
	UpdateTrackGizmo();
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTrackViewAnimNode::GetTransformDelegatePos(const Vec3 &basePos) const
{	
	const Vec3 position = GetPos();

	return Vec3(CheckTrackAnimated(eAnimParamType_PositionX) ? position.x : basePos.x,
		CheckTrackAnimated(eAnimParamType_PositionY) ? position.y : basePos.y,
		CheckTrackAnimated(eAnimParamType_PositionZ) ? position.z : basePos.z);
}

//////////////////////////////////////////////////////////////////////////
Quat CTrackViewAnimNode::GetTransformDelegateRotation(const Quat &baseRotation) const
{
	if (!CheckTrackAnimated(eAnimParamType_Rotation))
	{
		return baseRotation;
	}

	const Ang3 angBaseRotation(baseRotation);
	const Ang3 angNodeRotation(GetRotation());

	return Quat(Ang3(CheckTrackAnimated(eAnimParamType_RotationX) ? angNodeRotation.x : angBaseRotation.x,
		CheckTrackAnimated(eAnimParamType_RotationY) ? angNodeRotation.y : angBaseRotation.y,
		CheckTrackAnimated(eAnimParamType_RotationZ) ? angNodeRotation.z : angBaseRotation.z));
}

//////////////////////////////////////////////////////////////////////////
Vec3 CTrackViewAnimNode::GetTransformDelegateScale(const Vec3 &baseScale) const
{
	const Vec3 scale = GetScale();

	return Vec3(CheckTrackAnimated(eAnimParamType_ScaleX) ? scale.x : baseScale.x,
		CheckTrackAnimated(eAnimParamType_ScaleY) ? scale.y : baseScale.y,
		CheckTrackAnimated(eAnimParamType_ScaleZ) ? scale.z : baseScale.z);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegatePos(const Vec3 &position)
{
	SetPos(position);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegateRotation(const Quat &rotation)
{
	SetRotation(rotation);
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::SetTransformDelegateScale(const Vec3 &scale)
{
	SetScale(scale);
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsPositionDelegated() const
{		
	const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Position)) || CheckTrackAnimated(eAnimParamType_Position);
	return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsRotationDelegated() const
{	
	const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Rotation)) || CheckTrackAnimated(eAnimParamType_Rotation);
	return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
bool CTrackViewAnimNode::IsScaleDelegated() const
{	
	const bool bDelegated = (GetIEditor()->GetAnimation()->IsRecording() && m_pNodeEntity->IsSelected() && GetTrackForParameter(eAnimParamType_Scale)) || CheckTrackAnimated(eAnimParamType_Scale);
	return bDelegated;
}

//////////////////////////////////////////////////////////////////////////
void CTrackViewAnimNode::OnDone()
{
	SetNodeEntity(nullptr);
	UpdateTrackGizmo();
}
