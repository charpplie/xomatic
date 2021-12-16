// SequenceKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CSequenceKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CSequenceKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_name;
	CSmartVariable<bool> mv_overrideTimes;
	CSmartVariable<float> mv_startTime;
	CSmartVariable<float> mv_endTime;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_name,"Sequence" );
		AddVariable( mv_table,mv_overrideTimes,"Override Start/End Times" );
		AddVariable( mv_table,mv_startTime,"Start Time" );
		AddVariable( mv_table,mv_endTime,"End Time" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_SEQUENCE)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
};
IMPLEMENT_DYNCREATE(CSequenceKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CSequenceKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_SEQUENCE)
		{
			std::vector<CBaseObject*> objects;
			IMovieSystem *pMovieSystem;
			ISequenceIt *pSeqIt;
			IAnimSequence *pSeq;

			// fill list with available items
			mv_name.SetEnumList(NULL);
			pMovieSystem = GetIEditor()->GetSystem()->GetIMovieSystem();
			pSeqIt = pMovieSystem->GetSequences();
			pSeq = pSeqIt->first();
			while (pSeq)
			{
				if (pSeq != GetIEditor()->GetAnimation()->GetSequence())
					mv_name->AddEnumItem(pSeq->GetName(), pSeq->GetName());
				pSeq = pSeqIt->next();
			}
			pSeqIt->Release();

			ISequenceKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_name = key.szSelection;
			mv_overrideTimes = key.bOverrideTimes;
			if (!key.bOverrideTimes)
			{
				IAnimSequence *pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(key.szSelection);
				if (pSequence)
				{
					key.fStartTime = pSequence->GetTimeRange().start;
					key.fEndTime = pSequence->GetTimeRange().end;
				}
				else
				{
					key.fStartTime = 0.0f;
					key.fEndTime = 0.0f;
				}
			}

			mv_startTime = key.fStartTime;
			mv_endTime = key.fEndTime;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CSequenceKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_SEQUENCE)
		{
			ISequenceKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_name.GetVar())
			{
				CString sName;
				sName = mv_name;
				strncpy(key.szSelection,sName,sizeof(key.szSelection));
				key.szSelection[sizeof(key.szSelection)-1] = '\0';
			}

			SyncValue( mv_overrideTimes,key.bOverrideTimes,false,pVar );

			IAnimSequence *pSequence = GetIEditor()->GetSystem()->GetIMovieSystem()->FindSequence(key.szSelection);

			if (!key.bOverrideTimes)
			{
				if (pSequence)
				{
					key.fStartTime = pSequence->GetTimeRange().start;
					key.fEndTime = pSequence->GetTimeRange().end;
				}
				else
				{
					key.fStartTime = 0.0f;
					key.fEndTime = 0.0f;
				}
			}
			else
			{
				SyncValue( mv_startTime,key.fStartTime,false,pVar );
				SyncValue( mv_endTime,key.fEndTime,false,pVar );
			}

			key.fDuration = key.fEndTime-key.fStartTime > 0 ? key.fEndTime-key.fStartTime : 0.0f;

			IMovieSystem *pMovieSystem = GetIEditor()->GetSystem()->GetIMovieSystem();

			if (pMovieSystem != NULL)
			{
				pMovieSystem->SetStartEndTime(pSequence,key.fStartTime,key.fEndTime);
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CSequenceKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {68030C46-1402-45d1-91B3-8EC6F29C0FED}
		static const GUID guid = 
		{ 0x68030c46, 0x1402, 0x45d1, { 0x91, 0xb3, 0x8e, 0xc6, 0xf2, 0x9c, 0xf, 0xed } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Sequence"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSequenceKeyUIControls); };
};

REGISTER_CLASS_DESC(CSequenceKeyUIControls_Class);