//  Crytek Engine Source File.
//  Copyright (C), Crytek GmbH, 2012.
//
//  Created: 21/5/2012 by Axel Gneiting
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "TrackViewSequence.h"
#include "IDataBaseManager.h"

struct ITrackViewSequenceManagerListener
{
	virtual void OnSequenceAdded(CTrackViewSequence *pSequence) {}
	virtual void OnSequenceRemoved(CTrackViewSequence *pSequence) {}
};

class CTrackViewSequenceManager : public IEditorNotifyListener, public IDataBaseManagerListener
{	
	friend class CAbstractUndoSequenceTransaction;
	friend class CSequenceObject;

public:
	CTrackViewSequenceManager();
	~CTrackViewSequenceManager();

	virtual void OnEditorNotifyEvent(EEditorNotifyEvent event);

	unsigned int GetCount() const { return m_sequences.size(); }

	void CreateSequence(CString name);
	void DeleteSequence(CTrackViewSequence *pSequence);

	CTrackViewSequence *GetSequenceByName(CString name) const;
	CTrackViewSequence *GetSequenceByIndex(unsigned int index) const;
	CTrackViewSequence *GetSequenceByAnimSequence(IAnimSequence *pAnimSequence) const;

	CTrackViewAnimNodeBundle GetAllRelatedAnimNodes(const CEntityObject *pEntityObject) const;
	CTrackViewAnimNode *GetActiveAnimNode(const CEntityObject *pEntityObject) const;

	void AddListener(ITrackViewSequenceManagerListener *pListener) { stl::push_back_unique(m_listeners, pListener); }
	void RemoveListener(ITrackViewSequenceManagerListener *pListener) { stl::find_and_erase(m_listeners, pListener); }

private:	
	void SortSequences();

	void OnSequenceAdded(CTrackViewSequence *pSequence);
	void OnSequenceRemoved(CTrackViewSequence *pSequence);

	virtual void OnDataBaseItemEvent(IDataBaseItem *pItem, EDataBaseItemEvent event);

	// Callback from SequenceObject
	IAnimSequence *OnCreateSequenceObject(CString name);
	void OnDeleteSequenceObject(CString name);

	void OnObjectEvent(CBaseObject* pObject, int event);
	void HandleObjectRename(CBaseObject* pObject);
	void HandleAttachmentChange(CBaseObject* pObject, int event);

	std::vector<ITrackViewSequenceManagerListener*> m_listeners;
	std::vector<std::unique_ptr<CTrackViewSequence>> m_sequences;

	// Set to hold sequences that existed when undo transaction began
	std::set<CTrackViewSequence*> m_transactionSequences;

	uint32 m_nextSequenceId;
	bool m_bUnloadingLevel;

	// Used to handle object attach/detach	
	std::unordered_map<CTrackViewSequence*, Matrix34> m_prevTransforms;
};