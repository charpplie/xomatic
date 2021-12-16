// SoundKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CSoundKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CSoundKeyUIControls)
public:
	CSmartVariableArray mv_table;
	CSmartVariableArray mv_options;

	CSmartVariable<CString> mv_soundFile;
	CSmartVariable<float> mv_volume;
	CSmartVariable<int> mv_pan;
	CSmartVariable<float> mv_inRadius;
	CSmartVariable<float> mv_outRadius;
	CSmartVariable<bool> mv_stream;
	CSmartVariable<bool> mv_loop;
	CSmartVariable<bool> mv_voice;
	CSmartVariable<bool> mv_lipSync;
	CSmartVariable<bool> mv_3d; 

	virtual void OnCreateVars()
	{
		mv_volume.GetVar()->SetLimits(0, 1.0f);
		mv_pan.GetVar()->SetLimits(-127.0f, 127.0f);
		mv_inRadius.GetVar()->SetLimits(0, 100000.0f);
		mv_outRadius.GetVar()->SetLimits(0, 100000.0f);

		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_soundFile,"Sound File",IVariable::DT_SOUND );
		AddVariable( mv_table,mv_volume,"Volume" );
		AddVariable( mv_table,mv_pan,"Pan" );
		AddVariable( mv_table,mv_inRadius,"In Radius" );
		AddVariable( mv_table,mv_outRadius,"Out Radius" );
		AddVariable( mv_options,"Options" );
		AddVariable( mv_options,mv_loop,"Loop" );
		AddVariable( mv_options,mv_stream,"Stream Sound" );
		AddVariable( mv_options,mv_voice,"Voice" );
		AddVariable( mv_options,mv_lipSync,"LipSync [if Voice]" );
		AddVariable( mv_options,mv_3d,"3D Sound" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_SOUND)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CSoundKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CSoundKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_SOUND)
		{
			ISoundKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_soundFile = (CString)key.pszFilename;
			mv_volume = key.fVolume;
			mv_pan = key.nPan - 127;
			mv_inRadius = key.inRadius;
			mv_outRadius = key.outRadius;
			mv_loop = key.bLoop;
			mv_voice = key.bVoice;
			mv_lipSync = key.bLipSync;
			mv_stream = key.bStream;
			mv_3d = key.b3DSound;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CSoundKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_SOUND)
		{
			ISoundKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (pVar == mv_soundFile.GetVar())
			{
				CString sFilename = mv_soundFile;
				strncpy(key.pszFilename, sFilename, sizeof(key.pszFilename));
				key.pszFilename[sizeof(key.pszFilename)-1]=0;
			}
			SyncValue( mv_volume,key.fVolume,false,pVar );
			if (pVar == mv_pan.GetVar())
				key.nPan = mv_pan+127;
			SyncValue( mv_inRadius,key.inRadius,false,pVar );
			SyncValue( mv_outRadius,key.outRadius,false,pVar );
			SyncValue( mv_3d,key.b3DSound,false,pVar );
			SyncValue( mv_stream,key.bStream,false,pVar );
			SyncValue( mv_loop,key.bLoop,false,pVar );
			SyncValue( mv_voice,key.bVoice,false,pVar );
			SyncValue( mv_lipSync,key.bLipSync,false,pVar );
			key.fDuration = 0.0f;

			_smart_ptr<ISound> pSound 
				= gEnv->pSoundSystem->CreateSound(key.pszFilename, 
																					FLAG_SOUND_2D|FLAG_SOUND_LOAD_SYNCHRONOUSLY);
			if (pSound)
			{
				int nLengthInMS = pSound->GetLengthMs();
				if (nLengthInMS > 0)
					key.fDuration = (float)(nLengthInMS+10)/1000.0f;
				else
					key.bLoop = true;
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	GetIEditor()->GetAnimation()->ForceAnimation();
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CSoundKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {AB2226E5-D593-49d2-B7CB-989412CAAEDE}
		static const GUID guid = 
		{ 0xab2226e5, 0xd593, 0x49d2, { 0xb7, 0xcb, 0x98, 0x94, 0x12, 0xca, 0xae, 0xde } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Sound"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CSoundKeyUIControls); };
};

REGISTER_CLASS_DESC(CSoundKeyUIControls_Class);