// TrackEventKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "TVEventsDialog.h"

//////////////////////////////////////////////////////////////////////////
class CTrackEventKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CTrackEventKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariableEnum<CString> mv_event;
	CSmartVariable<CString> mv_value;
	CSmartVariable<bool> mv_editEvents;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_event,"Track Event" );
		AddVariable( mv_table,mv_value,"Value" );
		AddVariable( mv_table,mv_editEvents,"Edit Track Events..." );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_TRACKEVENT)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
	void OnClickedEventEdit();
};
IMPLEMENT_DYNCREATE(CTrackEventKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CTrackEventKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_TRACKEVENT)
		{
			mv_event.SetEnumList(NULL);
			// Add track events
			if (CAnimationContext *pContext = GetIEditor()->GetAnimation())
			{
				if (IAnimSequence *pSequence = pContext->GetSequence())
				{
					const int iCount = pSequence->GetTrackEventsCount();
					for (int i = 0; i < iCount; ++i)
					{
						mv_event->AddEnumItem(pSequence->GetTrackEvent(i),
							pSequence->GetTrackEvent(i));
					}
				}
			}

			IEventKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_event = key.event;
			mv_value = key.eventValue;
			mv_editEvents = false;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CTrackEventKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	if(mv_editEvents == true)
	{
		mv_editEvents = false;
		OnClickedEventEdit();
		return;
	}

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_TRACKEVENT)
		{
			IEventKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			CString event,value,animation;
			event = mv_event;
			value = mv_value;

			if (pVar == mv_event.GetVar())
			{
				strncpy( key.event,event,sizeof(key.event) );
				key.event[sizeof(key.event)-1] = '\0';
			}
			if (pVar == mv_value.GetVar())
			{
				strncpy( key.eventValue,value,sizeof(key.eventValue) );
				key.eventValue[sizeof(key.eventValue)-1] = '\0';
			}
			strncpy( key.animation,animation,sizeof(key.animation) );
			key.animation[sizeof(key.animation)-1] = '\0';
			key.duration = 0;

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	RefreshTrackViewKeys();
}

void CTrackEventKeyUIControls::OnClickedEventEdit()
{
	if (CAnimationContext *pContext = GetIEditor()->GetAnimation())
	{
		if (IAnimSequence *pSequence = pContext->GetSequence())
		{
			// Create dialog
			CTVEventsDialog dlg(pSequence);
			if (dlg.DoModal() == IDOK)
			{
				mv_event.SetEnumList(NULL);
				pSequence->ClearTrackEvents();

				// Copy over track events and repopulate list
				TrackEvents::iterator itEvent = dlg.m_events.begin();
				TrackEvents::iterator itEventEnd = dlg.m_events.end();
				for (; itEvent != itEventEnd; ++itEvent)
				{
					const char* szEvent = itEvent->c_str();
					pSequence->AddTrackEvent(szEvent);
					mv_event->AddEnumItem(szEvent, szEvent);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
class CTrackEventKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {F7D002EB-1FEA-46fa-B857-FC2B1B990B7F}
		static const GUID guid = 
		{ 0xf7d002eb, 0x1fea, 0x46fa, { 0xb8, 0x57, 0xfc, 0x2b, 0x1b, 0x99, 0xb, 0x7f } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.TrackEvent"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CTrackEventKeyUIControls); };
};

REGISTER_CLASS_DESC(CTrackEventKeyUIControls_Class);