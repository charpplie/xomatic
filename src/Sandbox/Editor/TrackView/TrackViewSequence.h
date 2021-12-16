//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2012.
//
//  Created: 6/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "IMovieSystem.h"
#include "TrackViewAnimNode.h"
#include "Objects/SequenceObject.h"
#include "Undo/Undo.h"

struct ITrackViewSequenceListener
{
	// Called when sequence settings (time range, flags) have changed
	virtual void OnSequenceSettingsChanged(CTrackViewSequence *pSequence) {}

	enum ENodeChangeType
	{
		eNodeChangeType_Added,
		eNodeChangeType_Removed,
		eNodeChangeType_Expanded,
		eNodeChangeType_Collapsed,
		eNodeChangeType_Hidden,
		eNodeChangeType_Unhidden,
		eNodeChangeType_Enabled,
		eNodeChangeType_Disabled,
		eNodeChangeType_Muted,
		eNodeChangeType_Unmuted,
		eNodeChangeType_Selected,
		eNodeChangeType_Deselected,
		eNodeChangeType_SetAsActiveDirector,
		eNodeChangeType_NodeOwnerChanged
	};

	// Called when a node is changed
	virtual void OnNodeChanged(CTrackViewNode *pNode, ENodeChangeType type) {}

	// Called when a node is added
	virtual void OnNodeRenamed(CTrackViewNode *pNode, const char *pOldName) {}

	// Called when selection of nodes changed
	virtual void OnNodeSelectionChanged(CTrackViewSequence *pSequence) {}

	// Called when selection of keys changed.
	virtual void OnKeySelectionChanged(CTrackViewSequence *pSequence) {}	

	// Called when keys in a track changed
	virtual void OnKeysChanged(CTrackViewSequence *pSequence) {}	
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimSequence in TrackView and contains
// the editor side code for changing it
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewSequence : public CTrackViewAnimNode, public IUndoManagerListener
{
	friend class CTrackViewSequenceManager;
	friend class CTrackViewNode;
	friend class CTrackViewAnimNode;
	friend class CTrackViewTrack;
	friend class CTrackViewSequenceNotificationContext;
	friend class CTrackViewSequenceNoNotificationContext;

	// Undo friends
	friend class CAbstractUndoTrackTransaction;
	friend class CAbstractUndoAnimNodeTransaction;
	friend class CUndoAnimNodeReparent;
	friend class CUndoTrackObject;
	friend class CAbstractUndoSequenceTransaction;	

public:
	CTrackViewSequence(IAnimSequence *pSequence);
	~CTrackViewSequence();

	// Called after de-serialization of IAnimSequence
	void Load();

	// ITrackViewNode
	virtual ETrackViewNodeType GetNodeType() const override { return eTVNT_Sequence; }	
	
	virtual const char *GetName() const override { return m_pAnimSequence->GetName(); }
	virtual bool SetName(const char *pName) override;
	virtual bool CanBeRenamed() const override { return true; }

	// Binding/Unbinding
	virtual void BindToEditorObjects() override;
	virtual void UnBindFromEditorObjects() override;
	virtual bool IsBoundToEditorObjects() const override;

	// Time range
	void SetTimeRange(Range timeRange);
	Range GetTimeRange() const;

	// Current time in sequence. Note that this can be different from the time 
	// of the animation context, if this sequence is used as a sub sequence
	const float GetTime() const;

	// CryMovie Flags
	void SetFlags(IAnimSequence::EAnimSequenceFlags flags);
	IAnimSequence::EAnimSequenceFlags GetFlags() const;

	// Get sequence object in scene
	CSequenceObject *GetSequenceObject() const { return static_cast<CSequenceObject*>(m_pAnimSequence->GetOwner()); }

	// Check if this node belongs to a sequence
	bool IsAncestorOf(CTrackViewSequence *pSequence) const;

	// Get single selected key if only one key is selected
	CTrackViewKeyHandle FindSingleSelectedKey();

	// Get CryMovie sequence ID
	uint32 GetCryMovieId() const { return m_pAnimSequence->GetId(); }

	// Rendering
	virtual void Render(const SAnimContext &animContext) override;

	// Playback control	
	virtual void Animate(const SAnimContext &animContext) override;
	void Resume() { m_pAnimSequence->Resume(); }
	void Pause() { m_pAnimSequence->Pause(); }
	void StillUpdate() { m_pAnimSequence->StillUpdate(); }
	
	// Active & deactivate
	void Activate() { m_pAnimSequence->Activate(); }
	void Deactivate() { m_pAnimSequence->Deactivate(); }
	void PrecacheData(const float time) { m_pAnimSequence->PrecacheData(time); }

	// Begin & end cut scene
	void BeginCutScene(const bool bResetFx) const;
	void EndCutScene() const;

	// Time step
	float GetFixedTimeStep() const { return m_pAnimSequence->GetFixedTimeStep(); }
	void SetFixedTimeStep(const float dt) { m_pAnimSequence->SetFixedTimeStep(dt); }

	// Reset
	void Reset(const bool bSeekToStart) { m_pAnimSequence->Reset(bSeekToStart); }
	void ResetHard() { m_pAnimSequence->ResetHard(); }

	// Check if the sequence's layer is locked
	bool IsLayerLocked() const;

	// Check if it's a group node
	virtual bool IsGroupNode() const override { return true; }

	// Track Events (TODO: Undo?)
	int GetTrackEventsCount() const { return m_pAnimSequence->GetTrackEventsCount(); }
	const char *GetTrackEvent(int index) { return m_pAnimSequence->GetTrackEvent(index); }
	bool AddTrackEvent(const char* szEvent) { return m_pAnimSequence->AddTrackEvent(szEvent); }
	bool RemoveTrackEvent(const char* szEvent) { return m_pAnimSequence->RemoveTrackEvent(szEvent); }
	bool RenameTrackEvent(const char* szEvent, const char* szNewEvent) { return m_pAnimSequence->RenameTrackEvent(szEvent, szNewEvent); }
	bool MoveUpTrackEvent(const char* szEvent) { return m_pAnimSequence->MoveUpTrackEvent(szEvent); }
	bool MoveDownTrackEvent(const char* szEvent) { return m_pAnimSequence->MoveDownTrackEvent(szEvent); }
	void ClearTrackEvents() { m_pAnimSequence->ClearTrackEvents(); }

	// Deletes all selected nodes (re-parents childs if group node gets deleted)
	void DeleteSelectedNodes();
	
	// Select selected nodes in viewport
	void SelectSelectedNodesInViewport();

	// Deletes all selected keys
	void DeleteSelectedKeys();

	// Sync from/to base
	void SyncSelectedTracksToBase();
	void SyncSelectedTracksFromBase();

	// Listeners
	void AddListener(ITrackViewSequenceListener *pListener);
	void RemoveListener(ITrackViewSequenceListener *pListener);

	// Checks if this is the active sequence in TV
	bool IsActiveSequence() const;

	// The root sequence node is always an active director
	virtual bool IsActiveDirector() const override { return true; }

	// Stores track undo objects for tracks with selected keys
	void StoreUndoForTracksWithSelectedKeys();

	// Copy keys to clipboard (in XML form)
	void CopyKeysToClipboard(const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks);	

	// Paste keys from clipboard. Tries to match the given data to the target track first, 
	// then the target anim node and finally the whole sequence. If it doesn't find any 
	// matching location, nothing will be pasted. Before pasting the given time offset is
	// applied to the keys.
	void PasteKeysFromClipboard(CTrackViewAnimNode *pTargetNode, CTrackViewTrack *pTargetTrack, const float timeOffset = 0.0f);

	// Returns a vector of pairs that match the XML track nodes in the clipboard to the tracks in the sequence for pasting.
	// It is used by PasteKeysFromClipboard directly and to preview the locations of the to be pasted keys.
	typedef std::pair<CTrackViewTrack*, XmlNodeRef> TMatchedTrackLocation;
	std::vector<TMatchedTrackLocation> GetMatchedPasteLocations(XmlNodeRef clipboardContent, CTrackViewAnimNode *pTargetNode, CTrackViewTrack *pTargetTrack);

	// Adjust the time range
	void AdjustKeysToTimeRange(Range newTimeRange);

	// Clear all key selection
	void DeselectAllKeys();

	// Offset all key selection
	void OffsetSelectedKeys(const float timeOffset);
	// Scale all selected keys by this offset.
	void ScaleSelectedKeys(const float timeOffset);
	//! Push all the keys which come after the first key in time among selected ones by this offset.
	void SlideKeys(const float timeOffset);
	//! Clone all selected keys
	void CloneSelectedKeys();

	// Limit the time offset so as to keep all involved keys in range when offsetting.
	float ClipTimeOffsetForOffsetting(const float timeOffset);
	// Limit the time offset so as to keep all involved keys in range when scaling.
	float ClipTimeOffsetForScaling(const float timeOffset);
	// Limit the time offset so as to keep all involved keys in range when sliding.
	float ClipTimeOffsetForSliding(const float timeOffset);

	// Notifications
	void OnSequenceSettingsChanged();
	void OnKeySelectionChanged();
	void OnKeysChanged();
	void OnNodeSelectionChanged();		
	void OnNodeChanged(CTrackViewNode *pNode, ITrackViewSequenceListener::ENodeChangeType type);
	void OnNodeRenamed(CTrackViewNode *pNode, const char *pOldName);	

private:
	// These are used to avoid listener notification spam via CTrackViewSequenceNotificationContext.
	// For recursion there is a counter that increases on QueueListenerNotifications 
	// and decreases on SubmitPendingListenerNotifcations
	// Only when the counter reaches 0 again SubmitPendingListenerNotifcations 
	// will submit the notifications
	void QueueNotifications();
	void SubmitPendingNotifcations();

	// Called when an animation updates needs to be schedules
	void ForceAnimation();	

	virtual void CopyKeysToClipboard(XmlNodeRef &xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;

	void UpdateLightAnimationRefs(const char *pOldName, const char *pNewName);
	
	std::deque<CTrackViewTrack*> GetMatchingTracks(CTrackViewAnimNode *pAnimNode, XmlNodeRef trackNode);
	void GetMatchedPasteLocationsRec(std::vector<TMatchedTrackLocation> &locations, CTrackViewNode *pCurrentNode, XmlNodeRef clipboardNode);	

	virtual void BeginUndoTransaction();
	virtual void EndUndoTransaction();
	virtual void BeginRestoreTransaction();
	virtual void EndRestoreTransaction();

	// Current time when animated
	float m_time;

	// Stores if sequence is bound
	bool m_bBoundToEditorObjects;

	_smart_ptr<IAnimSequence> m_pAnimSequence;	
	std::vector<ITrackViewSequenceListener*> m_sequenceListeners;

	// Notification queuing
	unsigned int m_selectionRecursionLevel;
	bool m_bNoNotifications;
	bool m_bQueueNotifications;
	bool m_bNodeSelectionChanged;
	bool m_bForceAnimation;
	bool m_bKeySelectionChanged;
	bool m_bKeysChanged;
};

////////////////////////////////////////////////////////////////////////////
class CTrackViewSequenceNotificationContext
{
public:
	CTrackViewSequenceNotificationContext(CTrackViewSequence *pSequence) : m_pSequence(pSequence)
	{
		if (m_pSequence)
		{
			m_pSequence->QueueNotifications();
		}		
	}

	~CTrackViewSequenceNotificationContext()
	{
		if (m_pSequence)
		{
			m_pSequence->SubmitPendingNotifcations();
		}
	}

private:
	CTrackViewSequence *m_pSequence;
};

////////////////////////////////////////////////////////////////////////////
class CTrackViewSequenceNoNotificationContext
{
public:
	CTrackViewSequenceNoNotificationContext(CTrackViewSequence *pSequence) : m_pSequence(pSequence)
	{
		if (m_pSequence)
		{
			m_pSequence->m_bNoNotifications = true;
		}		
	}

	~CTrackViewSequenceNoNotificationContext()
	{
		if (m_pSequence)
		{
			m_pSequence->m_bNoNotifications = false;
		}
	}

private:
	CTrackViewSequence *m_pSequence;
};