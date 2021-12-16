// CharacterKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"

//////////////////////////////////////////////////////////////////////////
class CCharacterKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CCharacterKeyUIControls)
public:
	CSmartVariableArray mv_table;

	CSmartVariable<CString> mv_animation;
	CSmartVariable<bool> mv_loop;
	CSmartVariable<bool> mv_unload;
	CSmartVariable<bool> mv_inplace;
	CSmartVariable<float> mv_blendTime;
	CSmartVariable<float> mv_startTime;
	CSmartVariable<float> mv_timeScale;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_animation,"Animation",IVariable::DT_ANIMATION );
		AddVariable( mv_table,mv_loop,"Loop" );
		AddVariable( mv_table,mv_unload,"Unload" );
		AddVariable( mv_table,mv_inplace,"Inplace" );
		AddVariable( mv_table,mv_blendTime,"Blend Time" );
		AddVariable( mv_table,mv_startTime,"Start Time" );
		AddVariable( mv_table,mv_timeScale,"Time Scale" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_CHARACTER)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
};
IMPLEMENT_DYNCREATE(CCharacterKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CCharacterKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_CHARACTER)
		{
			ICharacterKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			// Find editor object who owns this node.
			IEntity *entity = selectedKeys.keys[0].pNode->GetEntity();
			if (entity)
			{
				// Set the character instance as a user data so that the UI
				// can display the proper list of animations.
				ICharacterInstance *pCharacter = entity->GetCharacter(0);
				mv_animation->SetUserData(pCharacter);
			}

			mv_animation = (CString)key.m_animation;
			mv_loop = key.m_bLoop;
			mv_unload = key.m_bUnload;
			mv_inplace = key.m_bInPlace;
			mv_blendTime = key.m_blendTime;
			mv_startTime = key.m_startTime;
			mv_timeScale = key.m_speed;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CCharacterKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_CHARACTER)
		{
			ICharacterKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			if (mv_animation.GetVar() == pVar)
			{
				strncpy( key.m_animation,(CString)mv_animation,sizeof(key.m_animation) );
				key.m_animation[sizeof(key.m_animation)-1] = '\0';
			}
			SyncValue( mv_loop,key.m_bLoop,false,pVar );
			SyncValue( mv_unload,key.m_bUnload,false,pVar );
			SyncValue( mv_inplace,key.m_bInPlace,false,pVar );
			SyncValue( mv_blendTime,key.m_blendTime,false,pVar );
			SyncValue( mv_startTime,key.m_startTime,false,pVar );
			SyncValue( mv_timeScale,key.m_speed,false,pVar );

			if (strlen(key.m_animation) > 0)
			{
				IEntity *entity = selectedKey.pNode->GetEntity();
				if (entity)
				{
					ICharacterInstance* pCharacter = entity->GetCharacter(0);
					if (pCharacter)
					{
						IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
						assert (pAnimations);

						int id=pAnimations->GetAnimIDByName(key.m_animation);
						key.m_duration = pAnimations->GetDuration_sec(id) + pAnimations->GetStart( key.m_animation );
					}
				}
			}


			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}
	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CCharacterKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {E977A6F4-CEC1-4c67-8735-28721B3F6FEF}
		static const GUID guid = { 0xe977a6f4, 0xcec1, 0x4c67, { 0x87, 0x35, 0x28, 0x72, 0x1b, 0x3f, 0x6f, 0xef } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Character"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CCharacterKeyUIControls); };
};

REGISTER_CLASS_DESC(CCharacterKeyUIControls_Class);