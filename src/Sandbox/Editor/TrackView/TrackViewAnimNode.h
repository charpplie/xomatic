//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2012.
//
//  Created: 6/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TrackViewNode.h"
#include "TrackViewTrack.h"
#include "Objects/EntityObject.h"
#include "Objects/TrackGizmo.h"

class CTrackViewAnimNode;
class CEntityObject;

// Represents a bundle of anim nodes
class CTrackViewAnimNodeBundle
{
public:
	unsigned int GetCount() const { return m_animNodes.size(); }
	CTrackViewAnimNode *GetNode(const unsigned int index) { return m_animNodes[index]; }
	const CTrackViewAnimNode *GetNode(const unsigned int index) const { return m_animNodes[index]; }

	void Clear();
	const bool DoesContain(const CTrackViewNode *pTargetNode);

	void AppendAnimNode(CTrackViewAnimNode *pNode);
	void AppendAnimNodeBundle(const CTrackViewAnimNodeBundle &bundle);

	void ExpandAll(bool bAlsoExpandParentNodes = true);
	void CollapseAll();

private:
	std::vector<CTrackViewAnimNode*> m_animNodes;
};

// Callback called by animation node when its animated.
class IAnimNodeAnimator
{
public:
	virtual ~IAnimNodeAnimator() {}

	virtual void Animate(CTrackViewAnimNode *pNode, const SAnimContext& ac) = 0;
	virtual void Render(CTrackViewAnimNode *pNode, const SAnimContext& ac) {}

	// Called when binding/unbinding the owning node
	virtual void Bind(CTrackViewAnimNode *pNode) {}
	virtual void UnBind(CTrackViewAnimNode *pNode) {}
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimNode in TrackView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IAnimNode, therefore deleting it 
// will not destroy the CryMovie track
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewAnimNode : public CTrackViewNode, public IAnimNodeOwner, 
	public ITransformDelegate, public IEntityObjectListener
{	
	friend class CAbstractUndoAnimNodeTransaction;
	friend class CAbstractUndoTrackTransaction;
	friend class CUndoAnimNodeReparent;

public:
	CTrackViewAnimNode(IAnimSequence *pSequence, IAnimNode *pAnimNode, CTrackViewNode *pParentNode);

	// Rendering
	virtual void Render(const SAnimContext &ac);
	
	// Playback
	virtual void Animate(const SAnimContext &animContext);

	// Binding/Unbinding
	virtual void BindToEditorObjects();
	virtual void UnBindFromEditorObjects();
	virtual bool IsBoundToEditorObjects() const;

	// Console sync
	virtual void SyncToConsole(SAnimContext &animContext);

	// CTrackViewAnimNode
	virtual ETrackViewNodeType GetNodeType() const override { return eTVNT_AnimNode; }
	
	// Create & remove sub anim nodes
	virtual CTrackViewAnimNode *CreateSubNode(const CString &name, const EAnimNodeType animNodeType, CEntityObject *pOwner = nullptr);
	virtual void RemoveSubNode(CTrackViewAnimNode *pSubNode);

	// Create & remove sub tracks
	virtual CTrackViewTrack *CreateTrack(const CAnimParamType &paramType);
	virtual void RemoveTrack(CTrackViewTrack *pTrack);

	// Add selected entities from scene to group node
	virtual CTrackViewAnimNodeBundle AddSelectedEntities();

	// Add current layer to group node
	virtual void AddCurrentLayer();

	// Director related	
	virtual void SetAsActiveDirector();
	virtual bool IsActiveDirector() const;

	// Checks if anim node is part of active sequence and of an active director
	virtual bool IsActive();

	// Set as view camera
	virtual void SetAsViewCamera();

	// Name setter/getter
	virtual const char *GetName() const override { return m_pAnimNode->GetName(); }
	virtual bool SetName(const char *pName) override;
	virtual bool CanBeRenamed() const override;

	// Node owner setter/getter
	virtual void SetNodeEntity(CEntityObject *pEntity);
	virtual CEntityObject *GetNodeEntity(const bool bSearch = true);

	// Entity setters/getters
	void SetEntityGuid(const EntityGUID& guid);
	EntityGUID *GetEntityGuid() const;	
	IEntity *GetEntity() const;	

	// Set/get source/target GUIDs
	void SetEntityGuidSource(const EntityGUID& guid);
	void SetEntityGuidTarget(const EntityGUID& guid);

	// Snap time value to prev/next key in sequence
	virtual bool SnapTimeToPrevKey(float &time) const override;
	virtual bool SnapTimeToNextKey(float &time) const override;

	// Node getters
	CTrackViewAnimNodeBundle GetAllAnimNodes();
	CTrackViewAnimNodeBundle GetSelectedAnimNodes();
	CTrackViewAnimNodeBundle GetAllOwnedNodes(const CEntityObject *pOwner);
	CTrackViewAnimNodeBundle GetAnimNodesByType(EAnimNodeType animNodeType);
	CTrackViewAnimNodeBundle GetAnimNodesByName(const char *pName);

	// Track getters
	virtual CTrackViewTrackBundle GetAllTracks();
	virtual CTrackViewTrackBundle GetSelectedTracks();	
	virtual CTrackViewTrackBundle GetTracksByParam(const CAnimParamType &paramType);

	// Key getters
	virtual CTrackViewKeyBundle GetAllKeys() override;
	virtual CTrackViewKeyBundle GetSelectedKeys() override;	
	virtual CTrackViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

	// Type getters
	EAnimNodeType GetType() const;

	// Flags
	EAnimNodeFlags GetFlags() const;

	// Disabled state
	virtual void SetDisabled(bool bDisabled) override;
	virtual bool IsDisabled() const override;

	// Return track assigned to the specified parameter.
	CTrackViewTrack* GetTrackForParameter(const CAnimParamType &paramType, uint32 index = 0) const;

	// Rotation/Position & Scale
	void SetPos(const Vec3& position);
	Vec3 GetPos() const { return m_pAnimNode->GetPos(); }	
	void SetScale(const Vec3& scale);
	Vec3 GetScale() const { return m_pAnimNode->GetScale(); }
	void SetRotation(const Quat &rotation);
	Quat GetRotation() const { return m_pAnimNode->GetRotate(); }

	// Param
	unsigned int GetParamCount() const;
	CAnimParamType GetParamType(unsigned int index) const;
	char *GetParamName(const CAnimParamType &paramType) const;	
	bool IsParamValid(const CAnimParamType &param) const;
	IAnimNode::ESupportedParamFlags GetParamFlags(const CAnimParamType &paramType) const;	
	EAnimValue GetParamValueType(const CAnimParamType &paramType) const;	
	void UpdateDynamicParams();

	// Parameter getters/setters
	template <class Type> bool SetParamValue(const float time, const CAnimParamType &param, const Type &value)
	{
		assert(m_pAnimNode);
		return m_pAnimNode->SetParamValue(time, param, value);
	}

	template <class Type> bool GetParamValue(const float time, const CAnimParamType &param, Type &value)
	{
		assert(m_pAnimNode);
		return m_pAnimNode->GetParamValue(time, param, value);		
	}

	// Check if it's a group node
	virtual bool IsGroupNode() const override;

	// Generate a new node name
	virtual CString GetAvailableNodeNameStartingWith(const CString &name) const;

	// Copy/Paste nodes
	virtual void CopyNodesToClipboard(const bool bOnlySelected);
	virtual bool PasteNodesFromClipboard();

	// Set new parent
	virtual void SetNewParent(CTrackViewAnimNode *pNewParent);

	// Check if this node may be moved to new parent
	virtual bool IsValidReparentingTo(CTrackViewAnimNode *pNewParent);

protected:
	// IAnimNodeOwner
	virtual void OnNodeAnimated(IAnimNode *pNode) override;
	// ~IAnimNodeOwner	
	
	IAnimNode *GetAnimNode() { return m_pAnimNode; }

private:
	// Copy selected keys to XML representation for clipboard
	virtual void CopyKeysToClipboard(XmlNodeRef &xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

	void CopyNodesToClipboardRec(CTrackViewAnimNode *pCurrentAnimNode, XmlNodeRef &xmlNode, const bool bOnlySelected);

	bool HasObsoleteTrackRec(CTrackViewNode *pCurrentNode) const;
	CTrackViewTrackBundle GetTracks(const bool bOnlySelected, const CAnimParamType &paramType);			

	void PasteNodeFromClipboard(XmlNodeRef xmlNode);

	void SetPosRotScaleTracksDefaultValues();

	void UpdateTrackGizmo();

	bool CheckTrackAnimated(const CAnimParamType &paramType) const;

	// IAnimNodeOwner
	virtual void OnNodeVisibilityChanged(IAnimNode *pNode, const bool bHidden) override;
	virtual void OnNodeReset(IAnimNode *pNode) override;
	// ~IAnimNodeOwner

	// ITransformDelegate
	virtual void MatrixInvalidated() override;

	virtual Vec3 GetTransformDelegatePos(const Vec3 &realPos) const override;
	virtual Quat GetTransformDelegateRotation(const Quat &realRotation) const override;
	virtual Vec3 GetTransformDelegateScale(const Vec3 &realScale) const override;
	
	virtual void SetTransformDelegatePos(const Vec3 &position) override;
	virtual void SetTransformDelegateRotation(const Quat &rotation) override;
	virtual void SetTransformDelegateScale(const Vec3 &scale) override;

	// If those return true the base object uses its own transform instead
	virtual bool IsPositionDelegated() const override;
	virtual bool IsRotationDelegated() const override;
	virtual bool IsScaleDelegated() const override;
	// ~ITransformDelegate

	// IEntityObjectListener
	virtual void OnNameChanged(const char *pName) override {}
	virtual void OnSelectionChanged(const bool bSelected) override;
	virtual void OnDone() override;
	// ~IEntityObjectListener	

	IAnimSequence *m_pAnimSequence;	
	_smart_ptr<IAnimNode> m_pAnimNode;
	CEntityObject *m_pNodeEntity;
	std::unique_ptr<IAnimNodeAnimator> m_pNodeAnimator;
	_smart_ptr<CGizmo> m_trackGizmo;
};
