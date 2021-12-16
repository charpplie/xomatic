// EventKeyUIControls.cpp
//

#include "stdafx.h"
#include "TrackViewKeyPropertiesDlg.h"
#include "CryEditDoc.h"
#include "Objects\EntityScript.h"
#include "mission.h"
#include "missionscript.h"

//////////////////////////////////////////////////////////////////////////
class CEventKeyUIControls : public CTrackViewKeyUIControls
{
	DECLARE_DYNCREATE(CEventKeyUIControls)
public:
	CSmartVariableArray mv_table;
	CSmartVariableArray mv_deprecated;

	CSmartVariableEnum<CString> mv_animation;
	CSmartVariableEnum<CString> mv_event;
	CSmartVariable<CString> mv_value;

	virtual void OnCreateVars()
	{
		AddVariable( mv_table,"Key Properties" );
		AddVariable( mv_table,mv_event,"Event" );
		AddVariable( mv_table,mv_value,"Value" );
		AddVariable( mv_deprecated,"Deprecated" );
		AddVariable( mv_deprecated,mv_animation,"Animation" );
	}
	bool SupportTrackType( EAnimTrackType type ) const
	{
		if (type == ATRACK_EVENT)
			return true;
		return false;
	}
	virtual bool OnKeySelectionChange( SelectedKeys &selectedKeys );
	virtual void OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys );
private:
};
IMPLEMENT_DYNCREATE(CEventKeyUIControls,CTrackViewKeyUIControls)

//////////////////////////////////////////////////////////////////////////
bool CEventKeyUIControls::OnKeySelectionChange( SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return false;

	bool bAssigned = false;
	if (selectedKeys.keys.size() == 1)
	{
		EAnimTrackType trType = selectedKeys.keys[0].pTrack->GetType();
		if (trType == ATRACK_EVENT)
		{
			mv_event.SetEnumList(NULL);
			mv_animation.SetEnumList(NULL);
			if (selectedKeys.keys[0].pNode->GetType()==ANODE_SCENE)
			{
				CMission *pMission=GetIEditor()->GetDocument()->GetCurrentMission();
				if (pMission)
				{
					CMissionScript *pScript=pMission->GetScript();
					if (pScript)
					{
						for (int i=0;i<pScript->GetEventCount();i++)
						{
							mv_event->AddEnumItem(pScript->GetEvent(i),
								pScript->GetEvent(i));
						}
					}
				}
			}else
			{
				// Find editor object who owns this node.
				IEntity *entity = selectedKeys.keys[0].pNode->GetEntity();
				if (entity)
				{
					// Add events.
					// Find EntityClass.
					CEntityScript *script = CEntityScriptRegistry::Instance()->Find( entity->GetClass()->GetName() );
					if (script)
					{
						mv_event->AddEnumItem("","");
						for (int i = 0; i < script->GetEventCount(); i++)
						{
							mv_event->AddEnumItem(script->GetEvent(i), script->GetEvent(i));
						}
					}

					// Add available animations.
					ICharacterInstance *pCharacter = entity->GetCharacter(0);
					if (pCharacter)
					{
						mv_animation->AddEnumItem("","");
						IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
						assert (pAnimations);

						uint32 numAnims		= pAnimations->GetAnimationCount();
						uint32 numMorphs	= pAnimations->numMorphTargets();
						for (int i = 0; i <(numAnims+numMorphs); ++i)
						{
							mv_animation->AddEnumItem(pAnimations->GetNameByAnimID(i),
								pAnimations->GetNameByAnimID(i));
						}
					}
				}
			}

			IEventKey key;
			selectedKeys.keys[0].pTrack->GetKey( selectedKeys.keys[0].nKey,&key );

			mv_event = key.event;
			mv_value = key.eventValue;
			mv_animation = key.animation;

			bAssigned = true;
		}
	}
	return bAssigned;
}

// Called when UI variable changes.
void CEventKeyUIControls::OnUIChange( IVariable *pVar,SelectedKeys &selectedKeys )
{
	if (!selectedKeys.bAllOfSameType)
		return;

	for (int keyIndex = 0,num = (int)selectedKeys.keys.size(); keyIndex < num; keyIndex++)
	{
		SelectedKey &selectedKey = selectedKeys.keys[keyIndex];
		EAnimTrackType trType = selectedKey.pTrack->GetType();
		if (trType == ATRACK_EVENT)
		{
			IEventKey key;
			selectedKey.pTrack->GetKey( selectedKey.nKey,&key );

			CString event,value,animation;
			event = mv_event;
			value = mv_value;
			animation = mv_animation;

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
			if (pVar == mv_animation.GetVar())
			{
				strncpy( key.animation,animation,sizeof(key.animation) );
				key.animation[sizeof(key.animation)-1] = '\0';
			}


			if (strlen(key.animation) > 0)
			{
				IEntity *entity = selectedKey.pNode->GetEntity();
				if (entity)
				{
					ICharacterInstance *pCharacter = entity->GetCharacter(0);
					if (pCharacter)
					{
						IAnimationSet* pAnimations = pCharacter->GetIAnimationSet();
						assert (pAnimations);
						int id=pAnimations->GetAnimIDByName(key.animation);
						key.duration = pAnimations->GetDuration_sec(id);
					}
				}
			}

			selectedKey.pTrack->SetKey( selectedKey.nKey,&key );
		}
	}

	RefreshTrackViewKeys();
}

//////////////////////////////////////////////////////////////////////////
class CEventKeyUIControls_Class : public CRefCountClassDesc
{
public:
	virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_TRACKVIEW_KEYUI; };
	virtual REFGUID ClassID()
	{
		// {ED5A2023-EDE1-4a47-BBE6-7D7BA0E4001D}
		static const GUID guid = 
		{ 0xed5a2023, 0xede1, 0x4a47, { 0xbb, 0xe6, 0x7d, 0x7b, 0xa0, 0xe4, 0x0, 0x1d } };
		return guid;
	}
	virtual const char* ClassName() { return "TrackView.KeyUI.Event"; };
	virtual const char* Category() { return "TrackViewKeyUI"; };
	virtual CRuntimeClass* GetRuntimeClass() { return RUNTIME_CLASS(CEventKeyUIControls); };
};

REGISTER_CLASS_DESC(CEventKeyUIControls_Class);