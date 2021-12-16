//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2012.
//
//  Created: 6/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "IMovieSystem.h"
#include "TrackViewNode.h"

class CTrackViewAnimNode;

// Represents a bundle of tracks
class CTrackViewTrackBundle
{
public:
	CTrackViewTrackBundle() : m_bAllOfSameType(true), m_bHasRotationTrack(false) {}

	unsigned int GetCount() const { return m_tracks.size(); }
	CTrackViewTrack *GetTrack(const unsigned int index) { return m_tracks[index]; }
	const CTrackViewTrack *GetTrack(const unsigned int index) const { return m_tracks[index]; }

	void AppendTrack(CTrackViewTrack *pTrack);
	void AppendTrackBundle(const CTrackViewTrackBundle &bundle);

	bool IsOneTrack() const;
	bool AreAllOfSameType() const { return m_bAllOfSameType; }
	bool HasRotationTrack() const { return m_bHasRotationTrack; }

private:
	bool m_bAllOfSameType;
	bool m_bHasRotationTrack;
	std::vector<CTrackViewTrack*> m_tracks;
};

// Track Memento for Undo/Redo
class CTrackViewTrackMemento
{
private:
	friend class CTrackViewTrack;
	XmlNodeRef m_serializedTrackState;
};

////////////////////////////////////////////////////////////////////////////
//
// This class represents a IAnimTrack in TrackView and contains
// the editor side code for changing it
//
// It does *not* have ownership of the IAnimTrack, therefore deleting it 
// will not destroy the CryMovie track
//
////////////////////////////////////////////////////////////////////////////
class CTrackViewTrack : public CTrackViewNode, public ITrackViewKeyBundle
{
	friend class CTrackViewKeyHandle;
	friend class CTrackViewKeyConstHandle;
	friend class CTrackViewKeyBundle;	
	friend class CAbstractUndoTrackTransaction;

public:
	CTrackViewTrack(IAnimTrack *pTrack, CTrackViewAnimNode *pTrackAnimNode, CTrackViewNode *pParentNode, 
		bool bIsSubTrack = false, unsigned int subTrackIndex = 0);

	CTrackViewAnimNode *GetAnimNode() const;

	// Name getter
	virtual const char *GetName() const;
	
	// CTrackViewNode
	virtual ETrackViewNodeType GetNodeType() const override{ return eTVNT_Track; }

	// Check for compound/sub track
	bool IsCompoundTrack() const { return m_bIsCompoundTrack; }

	// Sub track index
	bool IsSubTrack() const { return m_bIsSubTrack; }
	unsigned int GetSubTrackIndex() const { return m_subTrackIndex; }

	// Snap time value to prev/next key in track
	virtual bool SnapTimeToPrevKey(float &time) const override;
	virtual bool SnapTimeToNextKey(float &time) const override;

	// Key getters
	virtual unsigned int GetKeyCount() const override { return m_pAnimTrack->GetNumKeys(); }		
	virtual CTrackViewKeyHandle GetKey(unsigned int index) override;
	virtual CTrackViewKeyConstHandle GetKey(unsigned int index) const;

	virtual CTrackViewKeyHandle GetKeyByTime(const float time);
	virtual CTrackViewKeyHandle GetNearestKeyByTime(const float time);
	
	virtual CTrackViewKeyBundle GetSelectedKeys() override;
	virtual CTrackViewKeyBundle GetAllKeys() override;
	virtual CTrackViewKeyBundle GetKeysInTimeRange(const float t0, const float t1) override;

	// Key modifications	
	virtual CTrackViewKeyHandle CreateKey(const float time);
	virtual void SlideKeys(const float time0, const float timeOffset);
	void OffsetKeyPosition(const Vec3 &offset);

	// Value getters
	template <class Type> void GetValue(const float time, Type &value) const
	{
		assert (m_pAnimTrack);
		return m_pAnimTrack->GetValue(time, value);
	}

	void GetKeyValueRange(float &min, float &max) const;

	// Type getters
	CAnimParamType GetParameterType() const { return m_pAnimTrack->GetParameterType(); }
	EAnimValue GetValueType() const { return m_pAnimTrack->GetValueType(); }
	EAnimCurveType GetCurveType() const { return m_pAnimTrack->GetCurveType(); }

	// Mask
	bool IsMasked(uint32 mask) const { return m_pAnimTrack->IsMasked(mask); }

	// Flag getter
	IAnimTrack::EAnimTrackFlags GetFlags() const;

	// Spline getter
	ISplineInterpolator *GetSpline() const { return m_pAnimTrack->GetSpline(); }

	// Color
	ColorB GetCustomColor() const;
	void SetCustomColor(ColorB color);
	bool HasCustomColor() const;
	void ClearCustomColor();

	// Memento
	virtual CTrackViewTrackMemento GetMemento() const;
	virtual void RestoreFromMemento(const CTrackViewTrackMemento &memento);

	// Disabled state
	virtual void SetDisabled(bool bDisabled) override;
	virtual bool IsDisabled() const override;

	// Muted state
	void SetMuted(bool bMuted);
	bool IsMuted() const;

	// Key selection
	virtual void SelectKeys(const bool bSelected) override;

	// Paste from XML representation with time offset
	void PasteKeys(XmlNodeRef xmlNode, const float timeOffset);

	// Key types
	virtual bool AreAllKeysOfSameType() const override { return true; }

	// Animation layer index
	void SetAnimationLayerIndex(const int index);
	int GetAnimationLayerIndex() const;

private:	
	CTrackViewKeyHandle GetPrevKey(const float time);
	CTrackViewKeyHandle GetNextKey(const float time);

	// Those are called from CTrackViewKeyHandle
	void SetKey(unsigned int keyIndex, IKey *pKey);
	void GetKey(unsigned int keyIndex, IKey *pKey) const;

	void SelectKey(unsigned int keyIndex, bool bSelect);
	bool IsKeySelected(unsigned int keyIndex) const;

	void SetKeyTime(const int index, const float time);
	float GetKeyTime(const int index) const;

	void RemoveKey(const int index);
	int CloneKey(const int index);

	CTrackViewKeyBundle GetKeys(bool bOnlySelected, float t0, float t1);				
	CTrackViewKeyHandle GetSubTrackKeyHandle(unsigned int index) const;	

	// Copy selected keys to XML representation for clipboard
	virtual void CopyKeysToClipboard(XmlNodeRef &xmlNode, const bool bOnlySelectedKeys, const bool bOnlyFromSelectedTracks) override;	

	bool m_bIsCompoundTrack;
	bool m_bIsSubTrack;
	unsigned int m_subTrackIndex;
	_smart_ptr<IAnimTrack> m_pAnimTrack;	
	CTrackViewAnimNode *m_pTrackAnimNode;
};