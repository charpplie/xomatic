// MusicKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CMusicKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CMusicKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariable<CString> mv_mood;
	CSmartVariable<float> mv_time;
	CSmartVariable<bool> mv_type;

	virtual void OnCreateVars()
	{
		mv_time.GetVar()->SetLimits(0, 60.0f);

		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_type,"Mood(T) or Volume(F)?" );
		AddVariable( mv_table,mv_mood,"Mood (if Mood)" );
		AddVariable( mv_table,mv_time,"Time (if Volume)" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_MUSIC)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CMusicKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CMusicKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_MUSIC)
		{
			IMusicKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_type = key.eType==eMusicKeyType_SetMood?true:false;
			mv_mood = key.szMood;
			mv_time = key.fTime;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CMusicKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_MUSIC)
		{
			IMusicKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_type.GetVar())
				key.eType = mv_type ? eMusicKeyType_SetMood : eMusicKeyType_VolumeRamp;
			if (pVar == mv_mood.GetVar())
			{
				CString sMood;
				sMood = mv_mood;
				strncpy(key.szMood, sMood, sizeof(key.szMood));
				key.szMood[sizeof(key.szMood)-1] = 0;
			}
			SyncValue( mv_time,key.fTime,false,pVar );

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CMusicKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {351BEE0D-9D29-4fca-ADE7-6C16B607D666}
		static const GUID guid = 
		{ 0x351bee0d, 0x9d29, 0x4fca, { 0xad, 0xe7, 0x6c, 0x16, 0xb6, 0x7, 0xd6, 0x66 } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Music"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CMusicKeyUIControls); };
};

REGISTER_CLASS_DESC(CMusicKeyUIControls_Class);